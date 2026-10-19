import type { ExecutionOperation } from './contracts.js';
import { hex64 } from './n64-contracts.js';
export const TRACE_METHODS = ['n64.trace.start', 'n64.trace.page'] as const;
export interface TraceRecord { pc: string; frame: number; count?: number; sequence?: number; target?: string; kind?: number; link_register?: number; link_value?: string; sp?: string; registers?: Record<string, string>; f12?: string; fr?: boolean }
export interface TracePage {
  operation_id: number; kind: 'pc' | 'calls'; epoch: number; start_frame: number; end_frame: number; stop_reason: string;
  record_count: number; storage_record_limit: number; offset: number; next_offset: number | null; records: TraceRecord[];
  complete: boolean; truncated: boolean; dropped_records: number; overflow_observations: number;
  unique_pcs: number; counted_instructions: number; frames_traced: number;
  traced_instructions: number; instructions_observed: number; calls_observed: number; excluded_callsite: number; excluded_target: number;
}
export interface TraceServiceMap {
  'n64.trace.start': { params: { kind: 'pc' | 'calls'; startFrame: number; frames: number; low: string; high: string; targetLow: string; targetHigh: string; maxRecords: number; registerMask: number; f12: boolean; timeoutMs: number }; result: { operation: ExecutionOperation; storage_record_limit: number } };
  'n64.trace.page': { params: { operationId: number; offset: number; limit: number }; result: TracePage };
}
const integer = (n: unknown, max = Number.MAX_SAFE_INTEGER): n is number => Number.isSafeInteger(n) && (n as number) >= 0 && (n as number) <= max;
const keys = (p: unknown, names: string[]) => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names.sort().join(',');
export function encodeTrace(method: string, params: unknown): string | undefined {
  const p = params as any;
  if (method === 'n64.trace.page') return keys(p, ['operationId', 'offset', 'limit']) && integer(p.operationId) && p.operationId > 0 && integer(p.offset) && integer(p.limit, 64) && p.limit > 0 ? ` ${p.operationId} ${p.offset} ${p.limit}` : undefined;
  if (!keys(p, ['kind', 'startFrame', 'frames', 'low', 'high', 'targetLow', 'targetHigh', 'maxRecords', 'registerMask', 'f12', 'timeoutMs']) || !['pc', 'calls'].includes(p.kind) || !integer(p.startFrame) || !integer(p.frames, 100000) || !p.frames || p.startFrame > Number.MAX_SAFE_INTEGER - p.frames || ![p.low, p.high, p.targetLow, p.targetHigh].every(hex64) || !integer(p.maxRecords, 2000000) || !p.maxRecords || !integer(p.registerMask, 0xffffffff) || typeof p.f12 !== 'boolean' || !integer(p.timeoutMs, 600000) || !p.timeoutMs) return;
  if (BigInt(p.low) >= BigInt(p.high) || BigInt(p.targetLow) >= BigInt(p.targetHigh)) return;
  return ` ${p.kind} ${p.startFrame} ${p.frames} ${p.low} ${p.high} ${p.targetLow} ${p.targetHigh} ${p.maxRecords} ${p.registerMask} ${p.f12 ? 1 : 0} ${p.timeoutMs}`;
}
export function validTracePage(s: any): s is TracePage {
  if (!s || !integer(s.operation_id) || !s.operation_id || !['pc', 'calls'].includes(s.kind) || !integer(s.epoch) || !s.epoch || !integer(s.start_frame) || !integer(s.end_frame) || typeof s.stop_reason !== 'string' || !integer(s.record_count, 262144) || !integer(s.storage_record_limit, 262144) || !s.storage_record_limit || !integer(s.offset) || s.offset > s.record_count || s.record_count > s.storage_record_limit || typeof s.complete !== 'boolean' || typeof s.truncated !== 'boolean' || !['unique_pcs', 'counted_instructions', 'frames_traced', 'dropped_records', 'overflow_observations', 'traced_instructions', 'instructions_observed', 'calls_observed', 'excluded_callsite', 'excluded_target'].every(k => integer(s[k]))) return false;
  if (!Array.isArray(s.records) || s.records.length > (s.kind === 'calls' ? 32 : 64) || s.offset + s.records.length > s.record_count || s.next_offset !== (s.offset + s.records.length < s.record_count ? s.offset + s.records.length : null) || (s.offset < s.record_count && s.records.length === 0)) return false;
  return s.records.every((r: TraceRecord) => !!r && hex64(r.pc) && integer(r.frame) && (s.kind === 'pc' ? integer(r.count) && r.count! > 0 : integer(r.sequence) && hex64(r.target) && integer(r.kind, 2) && integer(r.link_register, 31) && hex64(r.link_value) && hex64(r.sp) && !!r.registers && typeof r.registers === 'object' && !Array.isArray(r.registers) && Object.entries(r.registers).every(([k, v]) => /^(?:[0-9]|[12][0-9]|3[01])$/.test(k) && hex64(v)) && (r.f12 === undefined || (hex64(r.f12) && typeof r.fr === 'boolean'))));
}
