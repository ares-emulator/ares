import { hex64 } from './n64-contracts.js';
export interface DmaRecord {
  id: number; epoch: number; frame: number; cart_address: string; dram_address: string; requested_length: number;
  from_rom: boolean; rom_offset: string | null; cpu_pc: string; bytes_attempted: number; bytes_written: number | null;
  writes_confirmed: boolean; ranges_truncated: boolean; ranges: { dram_address: string; length: number }[]; code_candidate: boolean;
}
export interface DmaLog { entries: DmaRecord[]; next_since_id: number; more: boolean; capacity: 4096; dropped: number; epoch: number; history_complete: boolean; recording_from_epoch_start: boolean; enabled_at_frame: number; code_range: { start: string; end: string }; note: string }
export interface DmaServiceMap { 'n64.dma.log': { params: { sinceId: number; limit: number; codeStart: number; codeEnd: number }; result: DmaLog } }
const integer = (n: unknown, max = Number.MAX_SAFE_INTEGER): n is number => Number.isSafeInteger(n) && (n as number) >= 0 && (n as number) <= max;
export function encodeDma(p: any): string | undefined {
  if (!p || typeof p !== 'object' || Object.keys(p).sort().join(',') !== 'codeEnd,codeStart,limit,sinceId' || !integer(p.sinceId) || !integer(p.limit, 16) || !p.limit || !integer(p.codeStart, 0xffffffff) || !integer(p.codeEnd, 0x100000000) || p.codeEnd <= p.codeStart) return;
  return ` ${p.sinceId} ${p.limit} ${p.codeStart} ${p.codeEnd}`;
}
export function validDma(s: any): s is DmaLog {
  if (!s || !Array.isArray(s.entries) || s.entries.length > 16 || !integer(s.next_since_id) || typeof s.more !== 'boolean' || (s.more && !s.entries.length) || s.capacity !== 4096 || !integer(s.dropped) || !integer(s.epoch) || !s.epoch || typeof s.history_complete !== 'boolean' || typeof s.recording_from_epoch_start !== 'boolean' || !integer(s.enabled_at_frame) || !s.code_range || !hex64(s.code_range.start) || !hex64(s.code_range.end) || typeof s.note !== 'string') return false;
  let previous = 0;
  return s.entries.every((r: DmaRecord) => {
    if (!r) return false;
    const valid = !!r && integer(r.id) && r.id > previous && r.epoch === s.epoch && integer(r.frame) && [r.cart_address, r.dram_address, r.cpu_pc].every(hex64) && integer(r.requested_length, 0x1000000) && r.requested_length > 0 && typeof r.from_rom === 'boolean' && (r.from_rom ? hex64(r.rom_offset) : r.rom_offset === null) && integer(r.bytes_attempted) && typeof r.writes_confirmed === 'boolean' && (r.writes_confirmed ? integer(r.bytes_written) && r.bytes_written! <= r.bytes_attempted : r.bytes_written === null) && typeof r.ranges_truncated === 'boolean' && typeof r.code_candidate === 'boolean' && Array.isArray(r.ranges) && r.ranges.length <= 64 && r.ranges.every(range => hex64(range.dram_address) && integer(range.length) && range.length > 0);
    previous = r.id; return valid;
  }) && (!s.entries.length || s.next_since_id === previous);
}
