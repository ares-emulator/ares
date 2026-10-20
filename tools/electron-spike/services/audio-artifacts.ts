import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from './contracts.js';
import type { AudioServiceMap } from './audio-contracts.js';
import { publishArtifact } from './artifact-writer.js';

export class AudioArtifacts {
  private jobs = new Map<string, AbortController>();
  release(owner: string) { this.jobs.get(owner)?.abort(); }
  close() { for (const job of this.jobs.values()) job.abort(); }
  async save(client: SessionClient & { identity: SessionIdentity }, identity: SessionIdentity, path: string, owner: string, current: () => boolean) {
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
      guard(); const snapshot = unwrap(await client.request('audio.begin', identity, undefined, owner)); snapshotId = snapshot.snapshot_id;
      const wav = Buffer.alloc(44 + snapshot.bytes);
      wav.write('RIFF'); wav.writeUInt32LE(wav.length - 8, 4); wav.write('WAVEfmt ', 8);
      wav.writeUInt32LE(16, 16); wav.writeUInt16LE(1, 20); wav.writeUInt16LE(2, 22);
      wav.writeUInt32LE(snapshot.rate, 24); wav.writeUInt32LE(snapshot.rate * 4, 28);
      wav.writeUInt16LE(4, 32); wav.writeUInt16LE(16, 34); wav.write('data', 36); wav.writeUInt32LE(snapshot.bytes, 40);
      for (let offset = 0; offset < snapshot.bytes;) {
        guard();
        const length = Math.min(16384, snapshot.bytes - offset);
        const page: AudioServiceMap['audio.read']['result'] = unwrap(await client.request('audio.read', identity, { snapshotId, offset, length }, owner));
        if (page.snapshot_id !== snapshotId || page.offset !== offset || page.length !== length) throw new ServiceError('invalid_reply');
        const bytes = Buffer.from(page.hex, 'hex'); if (bytes.length !== length) throw new ServiceError('invalid_reply');
        bytes.copy(wav, 44 + offset); offset += length;
      }
      unwrap(await client.request('audio.status', identity, undefined, owner)); guard();
      // The reference audio_save replaces an existing destination. Preserve that
      // behavior, with atomic replacement and no partial file on failure.
      const artifact = await publishArtifact(path, true, write => write(wav), guard, 768044, () => { committed = true; });
      const result = { ...artifact, frames: snapshot.frames, rate: snapshot.rate, seconds: snapshot.frames / snapshot.rate, epoch: identity.epoch };
      try {
        unwrap(await client.request('audio.release', identity, { snapshotId, consume: true }, owner)); snapshotId = undefined;
        return result;
      } catch (error) {
        // A saved file is a completed side effect even if cleanup loses its epoch.
        return { ...result, cleanup_error: error instanceof ServiceError ? error.code : 'release_failed' };
      }
    } finally {
      if (snapshotId !== undefined) await client.request('audio.release', identity, { snapshotId, consume: committed }, owner).catch(() => {});
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
  }
}
