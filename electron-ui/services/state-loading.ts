import path from 'node:path';
import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from './contracts.js';
import { readStateFile, readStateMetadata, stateHeader } from './state-inspection.js';
import type { StateServiceMap } from './state-contracts.js';
export class StateLoading {
  private jobs = new Map<string, AbortController>();
  release(owner: string) { this.jobs.get(owner)?.abort(); }
  close() { for (const job of this.jobs.values()) job.abort(); }
  async load(client: SessionClient & { identity: SessionIdentity }, identity: SessionIdentity, p: { path: string; resume: boolean; keepOverrides: boolean }, owner: string, current: () => boolean) {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(), deadline = performance.now() + 25000; this.jobs.set(owner, job);
    const guard = () => {
      if (!current() || job.signal.aborted) throw new ServiceError('artifact_cancelled');
      if (client.identity.session !== identity.session || client.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('state_timeout');
    };
    const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
    let uploadId: number | undefined;
    try {
      guard(); const ticket = unwrap(await client.request('n64.state.loadBegin', identity, undefined, owner)); uploadId = ticket.upload_id;
      const file = path.resolve(p.path); if (file.length > 4096) throw new ServiceError('invalid_path');
      const data = await readStateFile(file, 67108864, guard), format = unwrap(await client.request('n64.state.format', identity, undefined, owner));
      const info = stateHeader(file, data, format);
      if (!info.signature_valid) throw new ServiceError('bad_signature');
      if (!info.version_supported) throw new ServiceError('unsupported_version');
      const meta = await readStateMetadata(file, data.bytes, info.version, guard);
      if (meta.present && !meta.value) throw new ServiceError('state_metadata_mismatch');
      let offset = 0, size = 0;
      const layout = meta.value?.rdram_layout;
      if (layout !== undefined) {
        if (!layout || layout.version !== info.version || layout.storage !== 'lsb32' || !Number.isSafeInteger(layout.offset) || layout.offset < 533 || !Number.isSafeInteger(layout.size) || layout.size <= 0 || layout.size > 8388608 || layout.size % 4 || layout.offset > data.bytes.length || layout.size > data.bytes.length - layout.offset) throw new ServiceError('invalid_layout');
        offset = layout.offset; size = layout.size;
      }
      guard(); unwrap(await client.request('n64.state.loadPrepare', identity, { uploadId, bytes: data.bytes.length, offset, size, romHash: meta.value?.rom_sha256 ?? null }, owner));
      for (let at = 0; at < data.bytes.length;) {
        guard(); const starts = Array.from({ length: Math.min(4, Math.ceil((data.bytes.length - at) / 1800)) }, (_, i) => at + i * 1800);
        const replies = await Promise.allSettled(starts.map(async start => unwrap(await client.request('n64.state.loadWrite', identity, { uploadId: ticket.upload_id, offset: start, hex: data.bytes.subarray(start, Math.min(start + 1800, data.bytes.length)).toString('hex') }, owner))));
        for (let n = 0; n < replies.length; n++) {
          const reply = replies[n]; if (reply.status === 'rejected') throw reply.reason;
          if (reply.value.written !== Math.min(1800, data.bytes.length - starts[n])) throw new ServiceError('invalid_reply');
          at += reply.value.written;
        }
      }
      guard(); const result: StateServiceMap['n64.state.loadCommit']['result'] = unwrap(await client.request('n64.state.loadCommit', identity, { uploadId, resume: p.resume, keepOverrides: p.keepOverrides }, owner));
      // Commit starts a new epoch. Never apply the old-epoch guard after it.
      uploadId = undefined; return { ...result, path: file };
    } catch (error) { if (error instanceof ServiceError) throw error; throw new ServiceError('state_read_failed'); }
    finally {
      if (uploadId !== undefined) await client.request('n64.state.loadAbort', identity, { uploadId }, owner).catch(() => {});
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
  }
}
