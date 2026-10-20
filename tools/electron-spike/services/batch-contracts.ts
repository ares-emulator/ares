import type { ScreenshotParams } from './screenshot-contracts.js';
import { validScreenshotRequest } from './screenshot-contracts.js';
import { encodeN64 } from './n64-contracts.js';
export const NATIVE_BATCH_METHODS = ['execution.batchBegin', 'execution.batchCheck'] as const;
export type BatchStep = { kind: 'state_load'; path: string }
  | { kind: 'write'; address: string; hex: string; view: 'cpu' | 'rdram' }
  | { kind: 'read'; address: string; length: number; view: 'cpu' | 'rdram' }
  | { kind: 'run'; n: number }
  | ({ kind: 'screenshot' } & ScreenshotParams);
export interface BatchParams { steps: BatchStep[]; timeoutMs: number; stopOnError: boolean }
export interface BatchStepResult { step: number; kind: BatchStep['kind']; ok: boolean; epoch: number; frame: number; data?: unknown; error?: string }
export interface BatchResult { batch_id: number; state: 'done'; stopped: true; stop_reason: string; start_epoch: number; epoch: number; frame: number; steps_completed: number; steps_attempted: number; results: BatchStepResult[]; side_effects_rolled_back: false }
export interface BatchServiceMap {
  'execution.batchBegin': { params: { timeoutMs: number }; result: { batch_id: number } };
  'execution.batchCheck': { params: { batchId: number }; result: { active: true } };
  'experiment.batch': { params: BatchParams; result: BatchResult };
}
const integer = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function encodeBatch(method: string, p: any): string | undefined {
  if (method === 'execution.batchBegin') return keys(p, 'timeoutMs') && integer(p.timeoutMs, 20000) && p.timeoutMs > 0 ? ` ${p.timeoutMs}` : undefined;
  if (method === 'execution.batchCheck') return keys(p, 'batchId') && integer(p.batchId) && p.batchId > 0 ? ` ${p.batchId}` : undefined;
  if (!keys(p, 'steps,stopOnError,timeoutMs') || !integer(p.timeoutMs, 20000) || p.timeoutMs < 1 || typeof p.stopOnError !== 'boolean' || !Array.isArray(p.steps) || !p.steps.length || p.steps.length > 32) return;
  let bytes = 0, frames = 0, artifacts = 0;
  for (const s of p.steps) {
    if (s?.kind === 'write' || s?.kind === 'read') {
      const { kind, ...memory } = s;
      if (encodeN64(kind === 'write' ? 'n64.memory.write' : 'n64.memory.read', memory) === undefined) return;
      bytes += kind === 'write' ? s.hex.length / 2 : s.length;
    } else if (s?.kind === 'run') {
      if (!keys(s, 'kind,n') || !integer(s.n, 1000000)) return;
      frames += s.n;
    } else if (s?.kind === 'state_load') {
      if (!keys(s, 'kind,path') || typeof s.path !== 'string' || !s.path.length || s.path.length > 4096 || s.path.includes('\0')) return;
    } else if (s?.kind === 'screenshot') {
      const { kind, ...capture } = s;
      if (!validScreenshotRequest('artifact.screenshot', capture) || capture.inline || !capture.path || capture.expectedFrame !== undefined) return;
      artifacts++;
    } else return;
  }
  if (bytes > 16384 || frames > 1000000 || artifacts > 8 || new TextEncoder().encode(JSON.stringify(p)).byteLength > 48000) return;
  return ''; // main-only; step data never enters the native control line
}
export function validBatchResult(s: any): s is BatchResult {
  return !!s && integer(s.batch_id) && s.batch_id > 0 && s.state === 'done' && s.stopped === true && typeof s.stop_reason === 'string' &&
    integer(s.start_epoch) && s.start_epoch > 0 && integer(s.epoch) && s.epoch > 0 && integer(s.frame) && s.side_effects_rolled_back === false &&
    Array.isArray(s.results) && s.results.length <= 32 && s.steps_attempted === s.results.length && s.steps_completed === s.results.filter((r: any) => r?.ok === true).length && s.results.every((r: any, i: number) => !!r && r.step === i &&
      ['state_load', 'write', 'read', 'run', 'screenshot'].includes(r.kind) && typeof r.ok === 'boolean' && integer(r.epoch) && integer(r.frame) &&
      (r.ok ? r.data !== undefined && r.error === undefined : typeof r.error === 'string')) && new TextEncoder().encode(JSON.stringify(s)).byteLength <= 49152;
}
