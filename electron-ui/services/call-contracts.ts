import type { ExecutionOperation } from './contracts.js';
import { hex64, validStop, type Stop } from './n64-contracts.js';
export const CALL_METHODS = ['n64.cpu.call', 'n64.cpu.callRecover'] as const;
export interface CallerSnapshot { pc: string; next_pc: string; delay_slot: boolean; lo: string; hi: string; gpr: string[]; cop0: (string | null)[]; fpr: string[]; fcr0: string; fcr31: string }
export interface CallProgress { entry: string; return_gate: string; pc: string; completion_pc: string | null; v0: string | null; v1: string | null; exception_code: number | null; caller_context_restored: boolean; recovery_available: boolean; memory_device_effects_retained: true; caller_snapshot: CallerSnapshot | null; completion_stop: Stop }
export interface CallServiceMap {
  'n64.cpu.call': { params: { address: string; args: string[]; maxInstructions: number; maxFrames: number; timeoutMs: number }; result: ExecutionOperation };
  'n64.cpu.callRecover': { params: { restore: boolean }; result: { restored: boolean; memory_device_effects_retained: true; pc: string; epoch: number; frame: number } };
}
const n = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const keys = (p: any, expected: string) => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === expected;
export function encodeCall(method: string, p: any): string | undefined {
  if (method === 'n64.cpu.callRecover') return keys(p, 'restore') && typeof p.restore === 'boolean' ? ` ${p.restore ? 1 : 0}` : undefined;
  return keys(p, 'address,args,maxFrames,maxInstructions,timeoutMs') && hex64(p.address) && Array.isArray(p.args) && p.args.length <= 4 && p.args.every(hex64) && n(p.maxInstructions, 1000000) && p.maxInstructions > 0 && n(p.maxFrames, 10000) && p.maxFrames > 0 && n(p.timeoutMs, 600000) && p.timeoutMs > 0 ? ` ${p.address} ${p.maxInstructions} ${p.maxFrames} ${p.timeoutMs}${p.args.map((x: string) => ` ${x}`).join('')}` : undefined;
}
const bank = (a: any, nullable = false) => Array.isArray(a) && a.length === 32 && a.every(x => nullable && x === null || hex64(x));
export function validCaller(s: any): s is CallerSnapshot {
  return !!s && ['pc', 'next_pc', 'lo', 'hi', 'fcr0', 'fcr31'].every(k => hex64(s[k])) && typeof s.delay_slot === 'boolean' && bank(s.gpr) && bank(s.cop0, true) && bank(s.fpr);
}
export function validCallProgress(s: any): s is CallProgress {
  if (!s) return false;
  const pending = s.completion_pc === null;
  if (pending ? s.caller_context_restored || s.recovery_available || s.caller_snapshot !== null || s.v0 !== null || s.v1 !== null || s.exception_code !== null : !s.caller_context_restored && (!s.recovery_available || s.caller_snapshot === null || s.v0 !== null || s.v1 !== null)) return false;
  return !!s && ['entry', 'return_gate', 'pc'].every(k => hex64(s[k])) && ['completion_pc', 'v0', 'v1'].every(k => s[k] === null || hex64(s[k])) && (s.exception_code === null || n(s.exception_code, 256)) && typeof s.caller_context_restored === 'boolean' && typeof s.recovery_available === 'boolean' && s.memory_device_effects_retained === true && (s.caller_snapshot === null || validCaller(s.caller_snapshot)) && validStop(s.completion_stop) && (!s.caller_context_restored || !s.recovery_available && s.v0 !== null && s.v1 !== null && s.caller_snapshot === null) && (!s.recovery_available || s.caller_snapshot !== null);
}
export function validCallRecovery(s: any) { return !!s && typeof s.restored === 'boolean' && s.memory_device_effects_retained === true && hex64(s.pc) && n(s.epoch) && s.epoch > 0 && n(s.frame); }
