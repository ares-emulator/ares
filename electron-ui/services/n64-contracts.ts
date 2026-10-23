import type { ExecutionOperation } from './contracts.js';
export const GPR_NAMES = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3', 't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7', 's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7', 't8', 't9', 'k0', 'k1', 'gp', 'sp', 's8', 'ra'];
/** N64-only contracts. All architectural integers travel as lossless hex strings. */
export const N64_METHODS = ['n64.debug.registers', 'n64.debug.lastStop', 'n64.debug.breakAdd', 'n64.debug.breakRemove', 'n64.debug.breakList', 'n64.debug.watchAdd', 'n64.debug.watchRemove', 'n64.debug.watchList', 'n64.debug.clear', 'n64.memory.read', 'n64.memory.readMany', 'n64.memory.write', 'n64.memory.writeMany'] as const;
export type N64Method = typeof N64_METHODS[number];
export interface Predicate { field: 'pc' | 'gpr' | 'hi' | 'lo' | 'value'; index: number; op: 'eq' | 'ne' | 'lt' | 'le' | 'gt' | 'ge'; value: string; mask: string }
export interface Registers { pc: string; next_pc: string; delay_slot: boolean; lo: string; hi: string; gpr: string[]; cop0: (string | null)[]; fpr: string[]; fcr0: string; fcr31: string }
export interface Stop { valid: boolean; stop_reason?: string; epoch?: number; frame?: number; instructions?: number; registers?: Registers; hit?: { id: number; other_ids: number[]; pc?: string; kind?: string; access_pc?: string; instruction?: string; addr?: string; paddr?: string; size?: number; value?: string; old_value?: string | null } }
export interface Definition { id: number; addr: string; paddr: string; hits: number; condition: { field: number; index: number; op: number; value: string; mask: string }[]; length?: number; kind?: number }
interface MemoryBase { addr: string; paddr: string; view: 'rdram' | 'cpu'; cache_aware: boolean }
export interface N64Read extends MemoryBase { length: number; hex: string }
export interface N64Write extends MemoryBase { hazards: string[]; written: number; jit_invalidated: boolean; icache_invalidated: false; dcache_dirty_lines_overlapping: number; icache_lines_overlapping: number }
export interface MemoryVerification { readbacks: N64Read[]; matches: boolean[] }
export interface MemoryWrites { writes: N64Write[]; immediate_verification: MemoryVerification; side_effects_rolled_back: false; operation: ExecutionOperation | null }
export interface N64ServiceMap {
  'n64.memory.readMany': { params: { reads: { address: string; view: 'cpu' | 'rdram'; length: number }[] }; result: Omit<import('./contracts.js').MemorySample, 'reads'> & { reads: N64Read[] } };
  'n64.memory.writeMany': { params: { writes: { address: string; view: 'rdram' | 'cpu'; hex: string }[]; verifyAfterFrames: number; timeoutMs: number }; result: MemoryWrites };
  'n64.debug.registers': { params: undefined; result: Registers };
  'n64.debug.lastStop': { params: undefined; result: Stop };
  'n64.debug.breakAdd': { params: { address: string; condition: Predicate[] }; result: { id: number } };
  'n64.debug.watchAdd': { params: { address: string; length: number; kind: number; condition: Predicate[] }; result: { id: number } };
  'n64.debug.breakRemove': { params: { id: number }; result: { removed: true } };
  'n64.debug.watchRemove': { params: { id: number }; result: { removed: true } };
  'n64.debug.breakList': { params: { offset: number }; result: { definitions: Definition[]; total: number } };
  'n64.debug.watchList': { params: { offset: number }; result: { definitions: Definition[]; total: number } };
  'n64.debug.clear': { params: undefined; result: { cleared: true } };
  'n64.memory.read': { params: { address: string; view: 'rdram' | 'cpu'; length: number }; result: N64Read };
  'n64.memory.write': { params: { address: string; view: 'rdram' | 'cpu'; hex: string }; result: N64Write };
}
const integer = (n: unknown): n is number => Number.isSafeInteger(n) && (n as number) >= 0;
export const hex64 = (s: unknown): s is string => typeof s === 'string' && /^0x[0-9a-f]{1,16}$/i.test(s);
const bytes = (s: unknown): s is string => typeof s === 'string' && /^(?:[0-9a-f]{2}){1,1024}$/i.test(s);
const keys = (p: unknown, names: string[]) => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names.sort().join(',');
const fields = ['pc', 'gpr', 'hi', 'lo', 'value'];
const ops = ['eq', 'ne', 'lt', 'le', 'gt', 'ge'];
/** Returns undefined on malformed input, including extra properties. */
export function encodeN64(method: N64Method, params: unknown): string | undefined {
  const p = params as any;
  if (method === 'n64.memory.readMany') {
    if (!keys(p, ['reads']) || !Array.isArray(p.reads) || !p.reads.length || p.reads.length > 64 || p.reads.some((r: any) => !keys(r, ['address', 'view', 'length']) || !hex64(r.address) || !['cpu', 'rdram'].includes(r.view) || !integer(r.length) || r.length < 1 || r.length > 65536) || p.reads.reduce((n: number, r: any) => n + r.length, 0) > 65536) return;
    return p.reads.map((r: any) => ` ${r.address} ${r.view} ${r.length}`).join('');
  }
  if (['n64.debug.registers', 'n64.debug.lastStop', 'n64.debug.clear'].includes(method)) return p === undefined ? '' : undefined;
  if (method.endsWith('Remove')) return keys(p, ['id']) && integer(p.id) && p.id > 0 && p.id <= 0xffffffff ? ` ${p.id}` : undefined;
  if (method.endsWith('List')) return keys(p, ['offset']) && integer(p.offset) && p.offset <= 256 ? ` ${p.offset}` : undefined;
  if (method.endsWith('Add')) {
    const watch = method === 'n64.debug.watchAdd';
    if (!keys(p, watch ? ['address', 'length', 'kind', 'condition'] : ['address', 'condition']) || !hex64(p.address) || !Array.isArray(p.condition) || p.condition.length > 8 ||
      (watch && (!integer(p.length) || p.length < 1 || p.length > 65536 || ![1, 2, 3].includes(p.kind)))) return;
    let out = ` ${p.address}${watch ? ` ${p.length} ${p.kind}` : ''}`;
    for (const c of p.condition) {
      if (!keys(c, ['field', 'index', 'op', 'value', 'mask']) || !fields.includes(c.field) || (!watch && c.field === 'value') || !integer(c.index) || c.index > 31 || !ops.includes(c.op) || !hex64(c.value) || !hex64(c.mask)) return;
      out += ` ${fields.indexOf(c.field)}:${c.index}:${ops.indexOf(c.op)}:${c.value}:${c.mask}`;
    }
    return out;
  }
  if (method === 'n64.memory.writeMany') {
    if (!keys(p, ['writes', 'verifyAfterFrames', 'timeoutMs']) || !Array.isArray(p.writes) || p.writes.length < 1 || p.writes.length > 32 ||
      !integer(p.verifyAfterFrames) || p.verifyAfterFrames > 100000 || !integer(p.timeoutMs) || p.timeoutMs < 1 || p.timeoutMs > 600000) return;
    let out = ` ${p.verifyAfterFrames} ${p.timeoutMs}`, total = 0;
    for (const w of p.writes) {
      const encoded = encodeN64('n64.memory.write', w);
      if (encoded === undefined || (total += w.hex.length / 2) > 1024) return;
      out += encoded;
    }
    return out;
  }
  const write = method === 'n64.memory.write';
  if (!keys(p, ['address', 'view', write ? 'hex' : 'length']) || !hex64(p.address) || !['rdram', 'cpu'].includes(p.view) ||
    (write ? !bytes(p.hex) : !integer(p.length) || p.length < 1 || p.length > 1024)) return;
  return ` ${p.address} ${p.view} ${write ? p.hex : p.length}`;
}
export function validRegisters(s: any): s is Registers {
  return !!s && [s.pc, s.next_pc, s.lo, s.hi, s.fcr0, s.fcr31].every(hex64) && typeof s.delay_slot === 'boolean' &&
    [s.gpr, s.fpr].every(a => Array.isArray(a) && a.length === 32 && a.every(hex64)) && Array.isArray(s.cop0) && s.cop0.length === 32 && s.cop0.every((v: unknown) => v === null || hex64(v));
}
export function validStop(s: any): s is Stop {
  if (!s || typeof s.valid !== 'boolean') return false;
  if (!s.valid) return true;
  return typeof s.stop_reason === 'string' && integer(s.epoch) && integer(s.frame) && integer(s.instructions) && validRegisters(s.registers) &&
    (s.hit === undefined || (integer(s.hit.id) && Array.isArray(s.hit.other_ids) && s.hit.other_ids.every(integer) &&
      (s.hit.pc === undefined || hex64(s.hit.pc)) && (s.hit.access_pc === undefined || (hex64(s.hit.access_pc) && hex64(s.hit.addr) && hex64(s.hit.paddr) && hex64(s.hit.instruction) && integer(s.hit.size) && s.hit.size >= 1 && s.hit.size <= 8 && hex64(s.hit.value) && (s.hit.old_value === null || hex64(s.hit.old_value))))));
}
export function validN64(method: N64Method, s: any): boolean {
  if (!s) return false;
  if (method === 'n64.memory.writeMany') return Array.isArray(s.writes) && s.writes.length >= 1 && s.writes.length <= 32 &&
    s.writes.every((w: unknown) => validN64('n64.memory.write', w)) && s.writes.reduce((n: number, w: N64Write) => n + w.written, 0) <= 1024 &&
    validMemoryVerification(s.immediate_verification) && s.immediate_verification.readbacks.length === s.writes.length && s.side_effects_rolled_back === false &&
    (s.operation === null || (s.operation && s.operation.kind === 'verify_writes'));
  if (method === 'n64.debug.registers') return validRegisters(s);
  if (method === 'n64.debug.lastStop') return validStop(s);
  if (method === 'n64.debug.clear') return s.cleared === true;
  if (method.endsWith('Add')) return integer(s.id) && s.id > 0;
  if (method.endsWith('Remove')) return s.removed === true;
  if (method.endsWith('List')) return integer(s.total) && s.total <= 256 && Array.isArray(s.definitions) && s.definitions.length <= 32 && s.definitions.every((d: Definition) => integer(d.id) && hex64(d.addr) && hex64(d.paddr) && integer(d.hits) && Array.isArray(d.condition) && d.condition.length <= 8 && d.condition.every(c => integer(c.field) && c.field <= 4 && integer(c.index) && c.index <= 31 && integer(c.op) && c.op <= 5 && hex64(c.value) && hex64(c.mask)));
  if (!hex64(s.addr) || !hex64(s.paddr) || !['rdram', 'cpu'].includes(s.view) || typeof s.cache_aware !== 'boolean') return false;
  if (method === 'n64.memory.read') return bytes(s.hex) && s.length === s.hex.length / 2;
  return Array.isArray(s.hazards) && s.hazards.every((h: unknown) => ['dirty_dcache_may_overwrite', 'resident_icache_not_updated'].includes(h as string)) && integer(s.written) && s.written > 0 && s.written <= 1024 && typeof s.jit_invalidated === 'boolean' && s.icache_invalidated === false && integer(s.dcache_dirty_lines_overlapping) && integer(s.icache_lines_overlapping);
}

export function validMemoryVerification(s: any): s is MemoryVerification {
  return !!s && Array.isArray(s.readbacks) && s.readbacks.length >= 1 && s.readbacks.length <= 32 &&
    s.readbacks.every((r: unknown) => validN64('n64.memory.read', r)) && s.readbacks.reduce((n: number, r: N64Read) => n + r.length, 0) <= 1024 &&
    Array.isArray(s.matches) && s.matches.length === s.readbacks.length && s.matches.every((v: unknown) => typeof v === 'boolean');
}
