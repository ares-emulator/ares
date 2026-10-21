import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from './contracts.js';
import type { DumpParams, DumpResult } from './inspection-contracts.js';
import { publishArtifact } from './artifact-writer.js';
export class MemoryDumps {
  private jobs = new Map<string, AbortController>();
  release(owner: string) { this.jobs.get(owner)?.abort(); }
  close() { for (const job of this.jobs.values()) job.abort(); }
  async save(client: SessionClient & { identity: SessionIdentity }, identity: SessionIdentity, p: DumpParams, owner: string, current: () => boolean): Promise<DumpResult> {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(), deadline = performance.now() + 25000; this.jobs.set(owner, job);
    const guard = () => {
      if (!current() || job.signal.aborted) throw new ServiceError('artifact_cancelled');
      if (client.identity.session !== identity.session || client.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('artifact_timeout');
    };
    const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
    let snapshotId: number | undefined;
    try {
      guard();
      const { path, overwrite, ...request } = p;
      const snapshot = unwrap(await client.request('n64.dump.begin', identity, request, owner)); snapshotId = snapshot.snapshot_id; guard();
      if (snapshot.epoch !== identity.epoch || (!p.all && snapshot.bytes !== p.length) || (p.all && p.expectSize !== null && snapshot.bytes !== p.expectSize)) throw new ServiceError('invalid_reply');
      const artifact = await publishArtifact(path, overwrite, async write => {
        for (let offset = 0; offset < snapshot.bytes;) {
          guard(); const starts = Array.from({ length: Math.min(4, Math.ceil((snapshot.bytes - offset) / 16384)) }, (_, i) => offset + i * 16384);
          const settled = await Promise.allSettled(starts.map(async start => unwrap(await client.request('n64.dump.read', identity, { snapshotId: snapshot.snapshot_id, offset: start, length: Math.min(16384, snapshot.bytes - start) }, owner))));
          for (let i = 0; i < settled.length; ++i) {
            const page = settled[i]; if (page.status === 'rejected') throw page.reason;
            const r = page.value, length = Math.min(16384, snapshot.bytes - starts[i]);
            if (r.snapshot_id !== snapshot.snapshot_id || r.offset !== starts[i] || r.length !== length || r.hex.length !== length * 2 || !/^[a-f0-9]+$/.test(r.hex)) throw new ServiceError('invalid_reply');
            await write(Buffer.from(r.hex, 'hex')); offset += length;
          }
        }
      }, guard, 8388608);
      const { snapshot_id, ...metadata } = snapshot;
      return { ...metadata, ...artifact, size: artifact.bytes, length: artifact.bytes, byte_order: 'guest big-endian bytes; no header' };
    } finally {
      if (snapshotId !== undefined) await client.request('n64.dump.release', identity, { snapshotId }, owner).catch(() => {});
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
  }
}
