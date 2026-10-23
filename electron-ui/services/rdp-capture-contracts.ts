import type { ExecutionOperation } from './contracts.js';
import type { PublishedArtifact, TraceExportParams } from './artifact-contracts.js';
import { hex64 } from './n64-contracts.js';
export const RDP_CAPTURE_METHODS = ['n64.rdp.captureStart', 'n64.rdp.capturePage'] as const;
export type CaptureKind = 'words' | 'batches' | 'tasks';
export interface CaptureWord { value: string; address: string; batch: number }
export interface CaptureBatch { id: number; source: 'DMEM' | 'RDRAM'; dpc_start: string; current: string; end: string; words: number; first_word: number; captured_words: number; cpu_pc: string; rsp_pc: string; rsp_running: boolean; task: number }
export interface CaptureTask { id: number; cpu_pc: string; header: string; dmem_hash_xxh64: string; imem_hash_xxh64: string; dmem: string | null }
export interface CapturePoint { pc: string; frame: number; epoch: number; vi: Record<string, number | string>; dpc: Record<string, number | string> }
export interface CapturePage {
  rom: {sha256: string; size: number; title?: string; id?: string}; renderer: string; start: CapturePoint; end: CapturePoint; rdram_size: number;
  operation_id: number; epoch: number; frame: number; kind: CaptureKind; stop_reason: string; complete: boolean; truncated: boolean;
  dropped_words: number; dropped_batches: number; dropped_tasks: number; batches_outside_interval: number;
  word_count: number; batch_count: number; task_count: number; storage_word_limit: number; storage_batch_limit: 512; storage_task_limit: 64; dmem_snapshots: boolean;
  offset: number; total: number; records: (CaptureWord | CaptureBatch | CaptureTask)[]; next_offset: number | null;
}
export interface RdpCaptureServiceMap {
  'n64.rdp.captureStart': { params: { maxWords: number; dmem: boolean; timeoutMs: number }; result: ExecutionOperation };
  'n64.rdp.capturePage': { params: { operationId: number; kind: CaptureKind; offset: number; limit: number }; result: CapturePage };
  'artifact.rdpExport': { params: TraceExportParams; result: PublishedArtifact };
}
const n = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const keys = (p: any, expected: string) => p && Object.keys(p).sort().join(',') === expected;
export function encodeRdpCapture(method: string, p: any): string | undefined {
  if (method === 'artifact.rdpExport') return keys(p, 'operationId,overwrite,path') && n(p.operationId) && p.operationId > 0 && typeof p.path === 'string' && p.path.length > 0 && p.path.length <= 4096 && !p.path.includes('\0') && typeof p.overwrite === 'boolean' ? '' : undefined;
  if (method === 'n64.rdp.captureStart') return keys(p, 'dmem,maxWords,timeoutMs') && n(p.maxWords, 65536) && p.maxWords > 0 && typeof p.dmem === 'boolean' && n(p.timeoutMs, 600000) && p.timeoutMs > 0 ? ` ${p.maxWords} ${p.dmem ? 1 : 0} ${p.timeoutMs}` : undefined;
  return keys(p, 'kind,limit,offset,operationId') && n(p.operationId) && p.operationId > 0 && ['words', 'batches', 'tasks'].includes(p.kind) && n(p.offset) && n(p.limit, 64) && p.limit > 0 ? ` ${p.operationId} ${p.kind} ${p.offset} ${p.limit}` : undefined;
}
export function validCapturePage(s: any): s is CapturePage {
  if (!s || !n(s.operation_id) || !s.operation_id || !n(s.epoch) || !s.epoch || !n(s.frame) || !['words', 'batches', 'tasks'].includes(s.kind) || typeof s.stop_reason !== 'string' || typeof s.complete !== 'boolean' || typeof s.truncated !== 'boolean' || typeof s.dmem_snapshots !== 'boolean') return false;
  const point = (v: any) => !!v && hex64(v.pc) && n(v.frame) && v.epoch === s.epoch && v.vi && typeof v.vi === 'object' && hex64(v.vi.VI_ORIGIN) && v.dpc && typeof v.dpc === 'object' && hex64(v.dpc.DPC_CURRENT);
  if (!s.rom || !/^[a-f0-9]{64}$/.test(s.rom.sha256) || !n(s.rom.size) || typeof s.renderer !== 'string' || !point(s.start) || !point(s.end) || !n(s.rdram_size, 8*1024*1024) || !s.rdram_size) return false;
  for (const key of ['dropped_words', 'dropped_batches', 'dropped_tasks', 'batches_outside_interval', 'word_count', 'batch_count', 'task_count', 'storage_word_limit', 'offset', 'total']) if (!n(s[key])) return false;
  if (!s.storage_word_limit || s.storage_word_limit > 65536 || s.word_count > s.storage_word_limit || s.batch_count > 512 || s.task_count > 64 || s.storage_batch_limit !== 512 || s.storage_task_limit !== 64 || !Array.isArray(s.records)) return false;
  const total = s.kind === 'words' ? s.word_count : s.kind === 'batches' ? s.batch_count : s.task_count;
  const end = s.offset + s.records.length;
  if (s.total !== total || s.offset > total || end > total || s.records.length > (s.kind === 'words' ? 64 : s.kind === 'batches' ? 16 : 1) || (end < total ? s.next_offset !== end || end === s.offset : s.next_offset !== null)) return false;
  return s.records.every((r: any, i: number) => {
    if (!r || typeof r !== 'object') return false;
    if (s.kind === 'words') return hex64(r.value) && hex64(r.address) && BigInt(r.address) <= 0xffffffn && n(r.batch) && r.batch < s.batch_count;
    if (s.kind === 'batches') return r.id === s.offset + i && ['DMEM', 'RDRAM'].includes(r.source) && ['dpc_start', 'current', 'end', 'cpu_pc', 'rsp_pc'].every(k => hex64(r[k])) && n(r.words) && n(r.first_word) && n(r.captured_words) && r.captured_words <= r.words && r.first_word + r.captured_words <= s.word_count && typeof r.rsp_running === 'boolean' && Number.isInteger(r.task) && r.task >= -1 && r.task < s.task_count;
    return r.id === s.offset + i && hex64(r.cpu_pc) && hex64(r.dmem_hash_xxh64) && hex64(r.imem_hash_xxh64) && typeof r.header === 'string' && /^[a-f0-9]{128}$/.test(r.header) && (s.dmem_snapshots ? typeof r.dmem === 'string' && /^[a-f0-9]{8192}$/.test(r.dmem) : r.dmem === null);
  });
}
