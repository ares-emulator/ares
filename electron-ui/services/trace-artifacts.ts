import { performance } from 'node:perf_hooks';
import type { NativeSessionClient } from './native-client.js';
import { ServiceError, type SessionIdentity } from './contracts.js';
import type { TraceExportParams, PublishedArtifact } from './artifact-contracts.js';
import { publishArtifact } from './artifact-writer.js';
import { traceEntries } from './trace-format.js';
import type { TracePage } from './trace-contracts.js';

/** Main owns filesystem policy and bounded jobs; native owns immutable trace pages. */
export class TraceArtifacts {
  private jobs = new Map<string, AbortController>();
  release(owner: string): void { this.jobs.get(owner)?.abort(); }
  close(): void { for (const job of this.jobs.values()) job.abort(); }
  async export(client: NativeSessionClient, identity: SessionIdentity, p: TraceExportParams, owner: string, current: () => boolean): Promise<PublishedArtifact> {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(); this.jobs.set(owner, job);
    const deadline = performance.now() + 25000;
    const guard = () => {
      if (job.signal.aborted || !current()) throw new ServiceError('artifact_cancelled');
      if (client.identity.epoch !== identity.epoch || client.identity.session !== identity.session) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('artifact_timeout');
    };
    const page = async (offset: number): Promise<TracePage> => {
      guard(); const reply = await client.request('n64.trace.page', identity, { operationId: p.operationId, offset, limit: 64 }, owner);
      if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply');
      guard(); return reply.result;
    };
    try {
      const first = await page(0);
      const { records, offset, next_offset, ...summary } = first;
      const kind = first.kind === 'pc' ? 'trace_pc' : 'trace_calls';
      const size = first.kind === 'pc' ? 64 : 32;
      const published = await publishArtifact(p.path, p.overwrite, async write => {
        await write(JSON.stringify({ version: 1, kind, summary: { ...summary, kind, trace_kind: first.kind, session: identity.session, interval: { epoch: identity.epoch, start_frame: first.start_frame, end_frame: first.end_frame }, total_entries: first.record_count, record_capacity: first.storage_record_limit } }).slice(0, -1) + ',"entries":[');
        let emitted = 0;
        const append = async (result: TracePage) => {
          if (result.operation_id !== first.operation_id || result.epoch !== first.epoch || result.kind !== first.kind || result.record_count !== first.record_count || result.offset !== emitted) throw new ServiceError('stale_trace');
          const text = traceEntries(result).map(entry => JSON.stringify(entry)).join(',');
          if (text) await write((emitted ? ',' : '') + text);
          emitted += result.records.length;
        };
        await append(first);
        while (emitted < first.record_count) {
          // Four bounded native pages per batch; avoid retaining the full capture.
          const offsets = Array.from({ length: Math.min(4, Math.ceil((first.record_count - emitted) / size)) }, (_, i) => emitted + i * size);
          const settled = await Promise.allSettled(offsets.map(page));
          for (const result of settled) {
            if (result.status === 'rejected') throw result.reason;
            await append(result.value);
          }
        }
        await write(']}\n');
        // A newer capture must not silently replace the one being exported.
        await page(0);
      }, guard);
      return { ...published, ...identity, operation_id: p.operationId, entries: first.record_count, format: 'ares-trace-v1' };
    } finally { if (this.jobs.get(owner) === job) this.jobs.delete(owner); }
  }
}
