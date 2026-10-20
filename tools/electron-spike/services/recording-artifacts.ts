import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from './contracts.js';
import { publishArtifact } from './artifact-writer.js';
import { aviParts } from './avi.js';
import type { RecordingServiceMap } from './recording-contracts.js';
export class RecordingArtifacts {
  private jobs = new Map<string, AbortController>();
  release(owner: string) { this.jobs.get(owner)?.abort(); }
  close() { for (const job of this.jobs.values()) job.abort(); }
  async stop(client: SessionClient & { identity: SessionIdentity }, identity: SessionIdentity, path: string, overwrite: boolean, owner: string, current: () => boolean) {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(), deadline = performance.now() + 25000; this.jobs.set(owner, job);
    const guard = () => {
      if (job.signal.aborted || !current()) throw new ServiceError('artifact_cancelled');
      if (client.identity.session !== identity.session || client.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('artifact_timeout');
    };
    const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
    let snapshotId: number | undefined, committed = false;
    try {
      guard(); const snapshot = unwrap(await client.request('video.record.begin', identity, undefined, owner)); snapshotId = snapshot.snapshot_id;
      const { snapshot_id, ...metadata } = snapshot, avi = aviParts(snapshot);
      const artifact = await publishArtifact(path, overwrite, async write => {
        await write(avi.header);
        for (let frame = 0; frame < snapshot.frames; frame++) {
          await write(avi.chunk);
          for (let offset = frame * avi.frameBytes; offset < (frame + 1) * avi.frameBytes;) {
            guard();
            const length = Math.min(16384, (frame + 1) * avi.frameBytes - offset);
            const page: RecordingServiceMap['video.record.read']['result'] = unwrap(await client.request('video.record.read', identity, { snapshotId: snapshot_id, offset, length }, owner));
            if (page.snapshot_id !== snapshot_id || page.offset !== offset || page.length !== length) throw new ServiceError('invalid_reply');
            const bytes = Buffer.from(page.hex, 'hex'); if (bytes.length !== length) throw new ServiceError('invalid_reply');
            await write(bytes); offset += length;
          }
        }
        await write(avi.index);
        unwrap(await client.request('video.record.status', identity, undefined, owner));
      }, guard, 270000000, () => { committed = true; });
      const result = { ...metadata, ...artifact, available: false, size: artifact.bytes, duration_seconds: snapshot.frames / snapshot.fps };
      try { unwrap(await client.request('video.record.release', identity, { snapshotId, consume: true }, owner)); snapshotId = undefined; return result; }
      catch (error) { return { ...result, cleanup_error: error instanceof ServiceError ? error.code : 'release_failed' }; }
    } finally {
      if (snapshotId !== undefined) await client.request('video.record.release', identity, { snapshotId, consume: committed }, owner).catch(() => {});
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
  }
}
