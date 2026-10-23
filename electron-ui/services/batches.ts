import { randomUUID } from 'node:crypto';
import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply, type ExecutionOperation } from './contracts.js';
import type { BatchParams, BatchResult, BatchStepResult } from './batch-contracts.js';
const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || r.result === undefined) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
const sleep = () => new Promise(resolve => setTimeout(resolve, 5));
/** Main orchestrates file work; the native reservation owns guest exclusivity. */
export class Batches {
  private jobs = new Map<string, { child: string; token?: string; cancel: (reason: string) => void }>();
  release(owner: string) { this.interrupt(owner, 'disconnected'); }
  cancel(owner:string,token:string) { const job=this.jobs.get(owner);if(!job || job.token!==token)return false;job.cancel('cancelled');return true; }
  child(owner: string) { return this.jobs.get(owner)?.child; }
  interrupt(owner: string, reason: string) { this.jobs.get(owner)?.cancel(reason); }
  close() { for (const job of this.jobs.values()) job.cancel('native_host_disconnected'); }
  async run(native: SessionClient & { identity: SessionIdentity }, invoke: SessionClient, starting: SessionIdentity,
    p: BatchParams, owner: string, current: () => boolean, releaseArtifacts: (owner: string) => void): Promise<BatchResult> {
    if (this.jobs.size) throw new ServiceError('busy');
    const child = `batch-${randomUUID()}`, deadline = performance.now() + p.timeoutMs;
    let cancelled = '', release: Promise<unknown> | undefined;
    const revoke = () => { releaseArtifacts(child); release ??= native.request('execution.release', undefined, undefined, child).catch(() => {}); };
    const job = { child, token:p.cancellationToken, cancel: (reason: string) => { cancelled ||= reason; revoke(); } };
    this.jobs.set(owner, job);
    const timer = setTimeout(() => job.cancel('timeout'), p.timeoutMs);
    let identity = { ...starting }, frame = 0, batchId: number | undefined;
    const results: BatchStepResult[] = [];
    const guard = () => {
      if (cancelled) throw new ServiceError(cancelled);
      if (!current()) throw new ServiceError('native_host_disconnected');
      if (performance.now() >= deadline) throw new ServiceError('timeout');
      if (native.identity.session !== identity.session || native.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
    };
    let reason = 'batch_done';
    try {
      guard();
      batchId = unwrap(await native.request('execution.batchBegin', identity, { timeoutMs: Math.max(1, Math.ceil(deadline - performance.now())) }, child)).batch_id;
      const status = unwrap(await native.request('session.status', identity, undefined, child)); frame = status.frame ?? 0;
      for (const [index, step] of p.steps.entries()) {
        guard(); unwrap(await native.request('execution.batchCheck', identity, { batchId }, child));
        // Reserve the largest bounded per-step result before performing effects.
        if (Buffer.byteLength(JSON.stringify(results)) > 114000) { reason = 'result_limit'; break; }
        let record: BatchStepResult;
        let runReason = '';
        try {
          let data: any;
          if (step.kind === 'state_load') {
            const r = await invoke.request('state.load', identity, { path: step.path, resume: false, keepOverrides: false }, child);
            data = unwrap(r); identity = { session: r.session, epoch: r.epoch }; frame = data.frame;
          } else if (step.kind === 'write') {
            const { kind, ...params } = step; data = unwrap(await invoke.request('n64.memory.write', identity, params, child));
          } else if (step.kind === 'read') {
            const { kind, ...params } = step; data = unwrap(await invoke.request('n64.memory.read', identity, params, child));
          } else if (step.kind === 'screenshot') {
            const { kind, ...params } = step; data = unwrap(await invoke.request('artifact.screenshot', identity, params, child));
          } else {
            let op: ExecutionOperation = unwrap(await invoke.request('execution.runFrames', identity, { count: step.n, timeoutMs: Math.max(1, Math.ceil(deadline - performance.now())) }, child));
            // On abort, native release freezes the partial run before this poll.
            while (op.state === 'running') { await sleep(); op = unwrap(await native.request('execution.status', identity, { operationId: op.operation_id }, child)); }
            data = op; frame = op.frame; if (op.stop_reason !== 'frames_done') runReason = op.stop_reason;
          }
          record = { step: index, kind: step.kind, ok: true, epoch: identity.epoch, frame, data };
          if (Buffer.byteLength(JSON.stringify(record)) > 16000) {
            record.data = { result_omitted: true, path: data.path, operation_id: data.operation_id, stop_reason: data.stop_reason };
            runReason ||= 'result_limit';
          }
        } catch (e) {
          record = { step: index, kind: step.kind, ok: false, epoch: identity.epoch, frame, error: e instanceof ServiceError ? e.code : 'step_failed' };
        }
        // Record committed files/loads and completed runs even if cancellation
        // arrived between their native commit and the JavaScript continuation.
        results.push(record);
        if (cancelled) { reason = cancelled; break; }
        if (runReason) { reason = runReason; break; }
        if (!record.ok && p.stopOnError) { reason = 'step_error'; break; }
      }
    } catch (e) {
      if (batchId === undefined) throw e; // no batch acquired and no effects
      reason = e instanceof ServiceError ? e.code : 'batch_failed';
    } finally {
      clearTimeout(timer); revoke(); await release;
      if (this.jobs.get(owner) === job) this.jobs.delete(owner);
    }
    return { batch_id: batchId!, state: 'done', stopped: true, stop_reason: reason, start_epoch: starting.epoch,
      epoch: identity.epoch, frame, steps_attempted: results.length, steps_completed: results.filter(r => r.ok).length, results, side_effects_rolled_back: false };
  }
}
