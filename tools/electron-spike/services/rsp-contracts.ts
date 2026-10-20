import type { ExecutionOperation } from './contracts.js';
import { hex64 } from './n64-contracts.js';
export const RSP_METHODS = ['n64.rsp.debug', 'n64.rsp.break', 'n64.rsp.step', 'n64.rsp.run', 'n64.rsp.stop'] as const;
export type RspMethod = typeof RSP_METHODS[number];
export interface RspDebugState { available: true; active: boolean; stopped: boolean; curPc: string; instrCount: number; stepsRemaining: number; runToBp: boolean; breakpoints: string[]; stop_reason: string; executor: 'interpreter' | 'recompiler' }
export interface RspProgress { start_pc: string; pc: string; executed: number; requested: number; hardware_halted: boolean; stop_reason: string }
export interface RspServiceMap {
  'n64.rsp.debug': { params: { enable: boolean }; result: RspDebugState };
  'n64.rsp.break': { params: { action: 'add' | 'remove' | 'clear' | 'list'; address: string | null }; result: { breakpoints: string[] } };
  'n64.rsp.step': { params: { count: number }; result: ExecutionOperation };
  'n64.rsp.run': { params: undefined; result: ExecutionOperation };
  'n64.rsp.stop': { params: undefined; result: { stopRequested: true; debug: RspDebugState } };
}
const integer = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const imem = (v: unknown) => hex64(v) && BigInt(v) <= 4095n;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
const bps = (v: any) => Array.isArray(v) && v.length <= 16 && v.every((x: unknown) => imem(x) && (BigInt(x as string) & 3n) === 0n) && new Set(v).size === v.length;
export function validRspDebug(s: any): s is RspDebugState {
  return !!s && s.available === true && typeof s.active === 'boolean' && typeof s.stopped === 'boolean' && imem(s.curPc) && integer(s.instrCount) && integer(s.stepsRemaining, 1000000) && typeof s.runToBp === 'boolean' && bps(s.breakpoints) && typeof s.stop_reason === 'string' && ['interpreter', 'recompiler'].includes(s.executor) && (!s.active || s.executor === 'interpreter');
}
export function validRspProgress(s: any): s is RspProgress {
  return !!s && imem(s.start_pc) && imem(s.pc) && integer(s.executed) && integer(s.requested, 1000000) && typeof s.hardware_halted === 'boolean' && typeof s.stop_reason === 'string';
}
export function encodeRsp(method: RspMethod, p: any): string | undefined {
  if (method === 'n64.rsp.debug') return keys(p, 'enable') && typeof p.enable === 'boolean' ? ` ${p.enable ? 1 : 0}` : undefined;
  if (method === 'n64.rsp.step') return keys(p, 'count') && integer(p.count, 1000000) && p.count > 0 ? ` ${p.count}` : undefined;
  if (method !== 'n64.rsp.break') return p === undefined ? '' : undefined;
  if (!keys(p, 'action,address') || !['list', 'clear', 'add', 'remove'].includes(p.action)) return;
  if (p.action === 'list' || p.action === 'clear') return p.address === null ? ` ${p.action}` : undefined;
  return imem(p.address) && (BigInt(p.address) & 3n) === 0n ? ` ${p.action} ${p.address}` : undefined;
}
export function validRspResult(method: RspMethod, s: any): boolean {
  if (method === 'n64.rsp.debug') return validRspDebug(s);
  if (method === 'n64.rsp.break') return !!s && bps(s.breakpoints);
  if (method === 'n64.rsp.stop') return !!s && s.stopRequested === true && validRspDebug(s.debug);
  return !!s && s.kind === (method === 'n64.rsp.step' ? 'rsp_step' : 'rsp_run') && validRspProgress(s.rsp);
}
