import { validRspDebug, type RspDebugState } from './rsp-contracts.js';
import { GPR_NAMES, hex64 } from './n64-contracts.js';
export const MACHINE_METHODS = ['n64.cpu.setRegisters', 'n64.cpu.disassemble', 'n64.rsp.registers', 'n64.rdp.state', 'n64.peripheral.registers'] as const;
export type MachineMethod = typeof MACHINE_METHODS[number];
export interface RegisterChanges { gpr?: Record<string, string>; pc?: string; hi?: string; lo?: string; llbit?: number; status?: string }
export interface RegisterMutation { gpr: { index: number; value: string }[]; pc: string | null; hi: string | null; lo: string | null; llbit: number | null; status: string | null }
export interface MutationResult { changed: RegisterChanges; pc: string; epoch: number; frame: number; jit_invalidated: boolean }
export interface Instruction { addr: string; word: string; mnemonic: string; operands: string; target?: string; target_source?: 'current_register_context'; target_register?: string }
export interface Disassembly { instructions: Instruction[]; view: 'cpu' | 'rdram'; epoch: number; frame: number }
export interface RspRegisters { gpr: string[]; pc: string; status: Record<string, number | number[]>; vu: number[][]; acch: number[]; accm: number[]; accl: number[]; dma: Record<string, number | boolean | string>; debug: RspDebugState; curInstrHex: string; curInstr: string }
export type RdpState = Record<'DPC_START' | 'DPC_END' | 'DPC_CURRENT' | 'DPC_CLOCK' | 'PIPE_BUSY' | 'TMEM_BUSY' | 'BUFFER_BUSY', string> & { ready: number; freeze: number; crashed: number; flush: number; source: 'DMEM' | 'RDRAM' };
export type PeripheralRegisters = Record<'vi' | 'ai' | 'pi' | 'si' | 'ri', Record<string, number | string>>;
export interface MachineServiceMap {
  'n64.cpu.setRegisters': { params: RegisterMutation; result: MutationResult };
  'n64.cpu.disassemble': { params: { address: string; count: number; view: 'cpu' | 'rdram' }; result: Disassembly };
  'n64.rsp.registers': { params: undefined; result: RspRegisters };
  'n64.rdp.state': { params: undefined; result: RdpState };
  'n64.peripheral.registers': { params: undefined; result: PeripheralRegisters };
}
const integer = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const hex32 = (v: unknown): v is string => hex64(v) && BigInt(v) <= 0xffffffffn;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function registerIndex(name: string): number {
  if (name === 'fp') return 30;
  if (/^[0-9]{1,2}$/.test(name)) return Number(name) <= 31 ? Number(name) : -1;
  return GPR_NAMES.indexOf(name);
}
export function encodeMachine(method: MachineMethod, p: any): string | undefined {
  if (method === 'n64.cpu.disassemble') return keys(p, 'address,count,view') && hex64(p.address) && integer(p.count, 128) && p.count > 0 && ['cpu', 'rdram'].includes(p.view) ? ` ${p.address} ${p.count} ${p.view}` : undefined;
  if (method !== 'n64.cpu.setRegisters') return p === undefined ? '' : undefined;
  if (!keys(p, 'gpr,hi,llbit,lo,pc,status') || !Array.isArray(p.gpr) || p.gpr.length > 31 ||
    ![p.pc, p.hi, p.lo].every(v => v === null || hex64(v)) || (p.status !== null && !hex32(p.status)) || (p.llbit !== null && p.llbit !== 0 && p.llbit !== 1)) return;
  const seen = new Set<number>(); let out = '';
  for (const r of p.gpr) {
    if (!keys(r, 'index,value') || !integer(r.index, 31) || !r.index || seen.has(r.index) || !hex64(r.value)) return;
    seen.add(r.index); out += ` gpr:${r.index}:${r.value}`;
  }
  for (const field of ['pc', 'hi', 'lo', 'status', 'llbit']) if (p[field] !== null) out += ` ${field}:${field === 'llbit' ? `0x${p[field]}` : p[field]}`;
  return out;
}
const numericFields = (s: any, fields: string[], max = 0xffffffff) => !!s && fields.every(f => integer(s[f], max));
const hexFields = (s: any, fields: string[]) => !!s && fields.every(f => hex32(s[f]));
const lane = (a: any) => Array.isArray(a) && a.length === 8 && a.every(v => integer(v, 65535));
const peripheralFields = {
  vi: { hex: ['VI_ORIGIN'], number: ['VI_CTRL.colorDepth', 'VI_CTRL.serrate', 'VI_CTRL.antialias', 'VI_WIDTH', 'VI_V_CURRENT', 'VI_X_SCALE', 'VI_Y_SCALE', 'VI_H_START', 'VI_H_END', 'VI_V_START', 'VI_V_END', 'field'] },
  ai: { hex: ['AI_DRAM_ADDR'], number: ['AI_LENGTH', 'AI_DMA_COUNT', 'AI_DACRATE', 'AI_BITRATE', 'AI_ENABLE'] },
  pi: { hex: ['PI_DRAM_ADDR', 'PI_CART_ADDR'], number: ['PI_RD_LEN', 'PI_WR_LEN', 'PI_STATUS.dmaBusy', 'PI_STATUS.ioBusy', 'PI_STATUS.error', 'PI_STATUS.interrupt'] },
  si: { hex: ['SI_DRAM_ADDR', 'SI_PIF_RD_ADDR', 'SI_PIF_WR_ADDR'], number: ['SI_STATUS.dmaBusy', 'SI_STATUS.ioBusy', 'SI_STATUS.interrupt'] },
  ri: { hex: ['RI_MODE', 'RI_CONFIG', 'RI_SELECT', 'RI_REFRESH', 'RI_LATENCY'], number: [] }
};
export function validMachine(method: MachineMethod, s: any): boolean {
  if (!s) return false;
  if (method === 'n64.rdp.state') return hexFields(s, ['DPC_START', 'DPC_END', 'DPC_CURRENT', 'DPC_CLOCK', 'PIPE_BUSY', 'TMEM_BUSY', 'BUFFER_BUSY']) && numericFields(s, ['ready', 'freeze', 'crashed', 'flush'], 1) && ['DMEM', 'RDRAM'].includes(s.source);
  if (method === 'n64.peripheral.registers') return Object.entries(peripheralFields).every(([group, fields]) => hexFields(s[group], fields.hex) && numericFields(s[group], fields.number));
  if (method === 'n64.rsp.registers') return hex32(s.pc) && Array.isArray(s.gpr) && s.gpr.length === 32 && s.gpr.every(hex32) &&
    numericFields(s.status, ['halted', 'broken', 'full', 'singleStep', 'interruptOnBreak', 'semaphore'], 1) && Array.isArray(s.status.signal) && s.status.signal.length === 8 && s.status.signal.every((v: unknown) => integer(v, 1)) &&
    Array.isArray(s.vu) && s.vu.length === 32 && s.vu.every(lane) && [s.acch, s.accm, s.accl].every(lane) && validRspDebug(s.debug) && hex32(s.curInstrHex) && typeof s.curInstr === 'string' && s.curInstr.length <= 2048 &&
    typeof s.dma?.busy === 'boolean' && typeof s.dma.full === 'boolean' && typeof s.dma.curOriginCpu === 'boolean' && hex64(s.dma.curOriginPc) &&
    numericFields(s.dma, ['busyR', 'busyW', 'fullR', 'fullW', 'curRegion', 'pendRegion'], 1) && numericFields(s.dma, ['curLen', 'curCount', 'pendLen']) && hexFields(s.dma, ['curDram', 'curPbus', 'pendDram', 'pendPbus']) && typeof s.dma.clock === 'string' && /^-?\d{1,19}$/.test(s.dma.clock);
  if (!integer(s.epoch) || s.epoch < 1 || !integer(s.frame)) return false;
  if (method === 'n64.cpu.disassemble') return ['cpu', 'rdram'].includes(s.view) && Array.isArray(s.instructions) && s.instructions.length >= 1 && s.instructions.length <= 128 && s.instructions.every((r: any) => !!r && hex64(r.addr) && hex32(r.word) && typeof r.mnemonic === 'string' && r.mnemonic.length <= 128 && typeof r.operands === 'string' && r.operands.length <= 2048 &&
    (r.target === undefined || hex64(r.target)) && (r.target_source === undefined || r.target_source === 'current_register_context' && GPR_NAMES.includes(r.target_register)));
  return hex64(s.pc) && typeof s.jit_invalidated === 'boolean' && !!s.changed && typeof s.changed === 'object' && Object.keys(s.changed).every(k => ['gpr', 'pc', 'hi', 'lo', 'llbit', 'status'].includes(k)) &&
    Object.entries(s.changed).every(([k, v]) => k === 'gpr' ? !!v && typeof v === 'object' && Object.entries(v).length <= 31 && Object.entries(v).every(([name, value]) => registerIndex(name) > 0 && hex64(value)) : k === 'llbit' ? v === 0 || v === 1 : k === 'status' ? hex32(v) : hex64(v));
}
