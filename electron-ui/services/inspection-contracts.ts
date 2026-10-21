import { hex64 } from './n64-contracts.js';
export const INSPECTION_METHODS = ['n64.render.probe', 'n64.dump.begin', 'n64.dump.read', 'n64.dump.release', 'n64.cpu.stack'] as const;
export interface DumpParams { path: string; overwrite: boolean; address: string; length: number; view: 'cpu' | 'rdram'; expectSize: number | null; all: boolean }
export interface DumpSnapshot { snapshot_id: number; bytes: number; epoch: number; frame: number; addr: string; paddr: string; view: 'cpu' | 'rdram'; cache_aware: boolean; stopped: boolean }
export interface DumpResult extends Omit<DumpSnapshot, 'snapshot_id'> { path: string; sha256: string; size: number; length: number; byte_order: string }
export interface Probe { viActive: boolean; viOrigin: string; dpcClock: string; epoch: number; frame: number; paused: boolean; presented_frame: number | null; vps: number | null; originChanged: boolean | null; dpcAdvanced: boolean | null; frameAdvanced: boolean | null; rendering: boolean | null }
export interface StackFrame { pc: string; sp: string; return_address: string | null; callsite: string | null; function_start_guess: string; frame_size: number; scan_bounds: { start: string; end: string }; sp_adjustment: number; ra_save_width: number; ra_source: string; epilogue_seen: boolean; function_hint_used: boolean; ra_slot_offset: number | null; method: 'live_registers' | 'prologue_analysis'; confidence: 'low' | 'medium' | 'high' }
export interface StackResult { frames: StackFrame[]; stop_reason: string; heuristic: true; sp: string; raw_sp_window: string; assumptions: string[]; epoch: number; frame: number }
export interface InspectionServiceMap {
  'n64.cpu.stack': { params: { functionStart: string | null }; result: StackResult };
  'n64.render.probe': { params: undefined; result: Probe };
  'n64.dump.begin': { params: Omit<DumpParams, 'path' | 'overwrite'>; result: DumpSnapshot };
  'n64.dump.read': { params: { snapshotId: number; offset: number; length: number }; result: { snapshot_id: number; offset: number; length: number; hex: string } };
  'n64.dump.release': { params: { snapshotId: number }; result: { released: true } };
  'artifact.dump': { params: DumpParams; result: DumpResult };
}
const n = (x: unknown, max = Number.MAX_SAFE_INTEGER): x is number => Number.isSafeInteger(x) && (x as number) >= 0 && (x as number) <= max;
const keys = (p: any, names: string) => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
function validBegin(p: any) {
  return hex64(p.address) && n(p.length, 1048576) && ['cpu', 'rdram'].includes(p.view) && typeof p.all === 'boolean' &&
    (p.all ? BigInt(p.address) === 0n && p.length === 0 && p.view === 'rdram' && (p.expectSize === null || n(p.expectSize, 8388608) && p.expectSize > 0) : p.length > 0 && p.expectSize === null);
}
export function encodeInspection(method: string, p: any): string | undefined {
  if (method === 'n64.cpu.stack') return keys(p, 'functionStart') && (p.functionStart === null || hex64(p.functionStart)) ? (p.functionStart === null ? '' : ` ${p.functionStart}`) : undefined;
  if (method === 'n64.render.probe') return p === undefined ? '' : undefined;
  if (method === 'artifact.dump') return keys(p, 'address,all,expectSize,length,overwrite,path,view') && validBegin(p) && typeof p.path === 'string' && p.path.length > 0 && p.path.length <= 4096 && !p.path.includes('\0') && typeof p.overwrite === 'boolean' ? '' : undefined;
  if (method === 'n64.dump.begin') return keys(p, 'address,all,expectSize,length,view') && validBegin(p) ? ` ${p.all ? 'all' : p.view} ${p.address} ${p.length} ${p.expectSize ?? 0}` : undefined;
  if (method === 'n64.dump.release') return keys(p, 'snapshotId') && n(p.snapshotId) && p.snapshotId > 0 ? ` ${p.snapshotId}` : undefined;
  return keys(p, 'length,offset,snapshotId') && n(p.snapshotId) && p.snapshotId > 0 && n(p.offset, 8388608) && n(p.length, 16384) && p.length > 0 ? ` ${p.snapshotId} ${p.offset} ${p.length}` : undefined;
}
function snapshot(s: any) { return !!s && n(s.bytes, 8388608) && s.bytes > 0 && n(s.epoch) && s.epoch > 0 && n(s.frame) && hex64(s.addr) && hex64(s.paddr) && BigInt(s.paddr) + BigInt(s.bytes) <= 8388608n && ['cpu', 'rdram'].includes(s.view) && typeof s.cache_aware === 'boolean' && typeof s.stopped === 'boolean'; }
export function validInspection(method: string, s: any): boolean {
  if (method === 'n64.cpu.stack') return !!s && s.heuristic === true && n(s.epoch) && s.epoch > 0 && n(s.frame) && hex64(s.sp) && typeof s.raw_sp_window === 'string' && /^(?:[a-f0-9]{2}){0,256}$/.test(s.raw_sp_window) && typeof s.stop_reason === 'string' && Array.isArray(s.assumptions) && s.assumptions.length <= 8 && s.assumptions.every((x: unknown) => typeof x === 'string') && Array.isArray(s.frames) && s.frames.length <= 8 && s.frames.every((f: any) => !!f && ['pc', 'sp', 'function_start_guess'].every(k => hex64(f[k])) && (f.return_address === null || hex64(f.return_address)) && (f.callsite === null || hex64(f.callsite)) && n(f.frame_size, 32768) && f.sp_adjustment === -f.frame_size && f.scan_bounds && hex64(f.scan_bounds.start) && hex64(f.scan_bounds.end) && [0, 4, 8].includes(f.ra_save_width) && typeof f.ra_source === 'string' && typeof f.epilogue_seen === 'boolean' && typeof f.function_hint_used === 'boolean' && (f.ra_slot_offset === null || n(f.ra_slot_offset, 32767)) && ['live_registers', 'prologue_analysis'].includes(f.method) && ['low', 'medium', 'high'].includes(f.confidence));
  if (method === 'n64.render.probe') return !!s && typeof s.viActive === 'boolean' && hex64(s.viOrigin) && hex64(s.dpcClock) && n(s.epoch) && s.epoch > 0 && n(s.frame) && typeof s.paused === 'boolean' && (s.presented_frame === null || n(s.presented_frame)) && (s.vps === null || typeof s.vps === 'number' && Number.isFinite(s.vps) && s.vps >= 0) && ['originChanged', 'dpcAdvanced', 'frameAdvanced', 'rendering'].every(k => s[k] === null || typeof s[k] === 'boolean');
  if (method === 'n64.dump.begin') return snapshot(s) && n(s.snapshot_id) && s.snapshot_id > 0;
  if (method === 'n64.dump.release') return !!s && s.released === true;
  if (method === 'artifact.dump') return snapshot(s) && s.size === s.bytes && s.length === s.bytes && typeof s.path === 'string' && s.path.length > 0 && typeof s.sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.sha256) && typeof s.byte_order === 'string';
  return !!s && n(s.snapshot_id) && s.snapshot_id > 0 && n(s.offset, 8388608) && n(s.length, 16384) && s.length > 0 && typeof s.hex === 'string' && s.hex.length === s.length * 2 && /^[a-f0-9]+$/.test(s.hex);
}
