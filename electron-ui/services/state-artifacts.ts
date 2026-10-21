import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from './contracts.js';
import type { StateSaveResult, StateServiceMap } from './state-contracts.js';
import { publishArtifact } from './artifact-writer.js';
export class StateArtifacts {
  private jobs = new Map<string, AbortController>();
  release(owner: string) { this.jobs.get(owner)?.abort(); }
  close() { for (const job of this.jobs.values()) job.abort(); }
  async save(client: SessionClient & { identity: SessionIdentity }, identity: SessionIdentity, p: { path: string; overwrite: boolean; metadata: boolean }, owner: string, current: () => boolean): Promise<StateSaveResult> {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(), deadline = performance.now() + 25000; this.jobs.set(owner, job);
    const guard = () => {
      if (!current() || job.signal.aborted) throw new ServiceError('artifact_cancelled');
      if (client.identity.session !== identity.session || client.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('state_timeout');
    };
    const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
    let snapshotId: number | undefined;
    try {
      guard(); const snapshot = unwrap(await client.request('n64.state.begin', identity, undefined, owner)); snapshotId = snapshot.snapshot_id;
      const artifact = await publishArtifact(p.path, p.overwrite, async write => {
        for (let offset = 0; offset < snapshot.bytes;) {
          guard();
          const starts = Array.from({ length: Math.min(4, Math.ceil((snapshot.bytes - offset) / 16384)) }, (_, i) => offset + i * 16384);
          const pages = await Promise.allSettled(starts.map(async start => unwrap(await client.request('n64.state.read', identity, { snapshotId: snapshot.snapshot_id, offset: start, length: Math.min(16384, snapshot.bytes - start) }, owner))));
          for (let n = 0; n < pages.length; n++) {
            const page = pages[n]; if (page.status === 'rejected') throw page.reason;
            const data: StateServiceMap['n64.state.read']['result'] = page.value;
            if (data.snapshot_id !== snapshot.snapshot_id || data.offset !== starts[n] || data.length !== Math.min(16384, snapshot.bytes - starts[n])) throw new ServiceError('invalid_reply');
            const bytes = Buffer.from(data.hex, 'hex'); if (bytes.length !== data.length) throw new ServiceError('invalid_reply');
            await write(bytes); offset += bytes.length;
          }
        }
      }, guard);
      const result: StateSaveResult = { ...artifact, size: artifact.bytes, version: snapshot.version, epoch: snapshot.epoch, frame: snapshot.frame, pc: snapshot.pc, stopped: true, mid_frame_cpu_stop: snapshot.mid_frame_cpu_stop };
      if (p.metadata) {
        const metadata = { metadata_version: 1, state_size: artifact.bytes, state_sha256: artifact.sha256, serializer_version: snapshot.version, epoch: snapshot.epoch, frame: snapshot.frame, rom_sha256: snapshot.rom_sha256,
          rdram_layout: { version: snapshot.version, offset: snapshot.offset, size: snapshot.size, storage: snapshot.storage } };
        try { result.metadata_path = (await publishArtifact(artifact.path + '.json', p.overwrite, write => write(JSON.stringify(metadata)), guard, 16384)).path; }
        catch (error) { result.metadata_error = error instanceof ServiceError ? error.code : 'write_failed'; }
      }
      return result;
    } finally {
      if (snapshotId !== undefined) await client.request('n64.state.release', identity, { snapshotId }, owner).catch(() => {});
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
  }
}
