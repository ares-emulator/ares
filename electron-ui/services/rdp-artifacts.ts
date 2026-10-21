import { performance } from 'node:perf_hooks';
import type { NativeSessionClient } from './native-client.js';
import { ServiceError, type SessionIdentity } from './contracts.js';
import type { TraceExportParams, PublishedArtifact } from './artifact-contracts.js';
import type { CaptureKind } from './rdp-capture-contracts.js';
import { publishArtifact } from './artifact-writer.js';
export class RdpArtifacts {
  private jobs = new Map<string, AbortController>();
  release(owner: string): void { this.jobs.get(owner)?.abort(); }
  close(): void { for (const job of this.jobs.values()) job.abort(); }
  async export(client: NativeSessionClient, identity: SessionIdentity, p: TraceExportParams, owner: string, current: () => boolean): Promise<PublishedArtifact> {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(); this.jobs.set(owner, job); const deadline = performance.now() + 25000;
    const guard = () => {
      if (job.signal.aborted || !current()) throw new ServiceError('artifact_cancelled');
      if (client.identity.epoch !== identity.epoch || client.identity.session !== identity.session) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('artifact_timeout');
    };
    const page = async (kind: CaptureKind, offset: number) => {
      guard(); const r = await client.request('n64.rdp.capturePage', identity, { operationId: p.operationId, kind, offset, limit: 64 }, owner); guard();
      if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply');
      if (r.result.operation_id !== p.operationId || r.result.epoch !== identity.epoch || r.result.kind !== kind || r.result.offset !== offset) throw new ServiceError('stale_capture');
      return r.result;
    };
    try {
      const first = await page('words', 0);
      const { records, offset, total, next_offset, kind, ...summary } = first;
      const published = await publishArtifact(p.path, p.overwrite, async write => {
        await write(JSON.stringify({ version: 1, kind: 'rdp_capture', session: identity.session, summary }).slice(0, -1));
        for (const section of ['words', 'batches', 'tasks'] as const) {
          await write(`,"${section}":[`); let emitted = 0;
          let result = section === 'words' ? first : await page(section, 0);
          for (;;) {
            if (result.word_count !== first.word_count || result.batch_count !== first.batch_count || result.task_count !== first.task_count || result.frame !== first.frame || result.offset !== emitted) throw new ServiceError('stale_capture');
            if (result.records.length) await write((emitted ? ',' : '') + result.records.map(r => JSON.stringify(r)).join(','));
            emitted += result.records.length;
            if (result.next_offset === null) break;
            result = await page(section, result.next_offset);
          }
          await write(']');
        }
        await write('}\n'); await page('words', 0);
      }, guard);
      return { ...published, ...identity, operation_id: p.operationId, entries: first.word_count, format: 'ares-rdp-capture-v1' };
    } finally { if (this.jobs.get(owner) === job) this.jobs.delete(owner); }
  }
}
