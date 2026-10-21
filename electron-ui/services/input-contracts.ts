import type { ExecutionOperation } from './contracts.js';
/** N64 four-port input ownership. Mutations replace the complete pending state. */
export const INPUT_METHODS = ['n64.input.set', 'n64.input.release', 'n64.input.state', 'n64.input.hostMode', 'n64.input.tap'] as const;
export type InputMethod = typeof INPUT_METHODS[number];
export interface PortInput { overridden: boolean; owner: string | null; buttons: number; x: number; y: number; expires_frame: number | null }
export interface InputState { ports: { port: number; pending: PortInput; latched: PortInput }[]; epoch: number; frame: number; neutral_during_operations: boolean; operation_running: boolean; human_takeovers: number }
export interface InputServiceMap {
  'n64.input.tap': { params: { port: number; button: number; frames: number; timeoutMs: number; keepOverride: boolean }; result: ExecutionOperation };
  'n64.input.set': { params: { port: number; buttons: number; x: number; y: number; timeoutFrames: number; timeoutMs: number }; result: InputState };
  'n64.input.release': { params: { port: number }; result: InputState };
  'n64.input.state': { params: undefined; result: InputState };
  'n64.input.hostMode': { params: { neutral: boolean }; result: InputState };
}
const integer = (n: unknown, min: number, max: number): n is number => Number.isSafeInteger(n) && (n as number) >= min && (n as number) <= max;
const keys = (p: unknown, names: string[]) => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names.sort().join(',');
export function encodeInput(method: InputMethod, params: unknown): string | undefined {
  const p = params as any;
  if (method === 'n64.input.tap') return keys(p, ['port', 'button', 'frames', 'timeoutMs', 'keepOverride']) && integer(p.port, 1, 4) && integer(p.button, 0, 13) && integer(p.frames, 1, 100000) && integer(p.timeoutMs, 1, 600000) && typeof p.keepOverride === 'boolean' ? ` ${p.port} ${p.button} ${p.frames} ${p.timeoutMs} ${+p.keepOverride}` : undefined;
  if (method === 'n64.input.state') return p === undefined ? '' : undefined;
  if (method === 'n64.input.hostMode') return keys(p, ['neutral']) && typeof p.neutral === 'boolean' ? ` ${p.neutral ? 1 : 0}` : undefined;
  if (method === 'n64.input.release') return keys(p, ['port']) && integer(p.port, 0, 4) ? ` ${p.port}` : undefined;
  if (!keys(p, ['port', 'buttons', 'x', 'y', 'timeoutFrames', 'timeoutMs']) || !integer(p.port, 1, 4) || !integer(p.buttons, 0, 0x3fff) || !integer(p.x, -32767, 32767) || !integer(p.y, -32767, 32767) || !integer(p.timeoutFrames, 0, 100000) || !integer(p.timeoutMs, 0, 3600000)) return;
  return ` ${p.port} ${p.buttons} ${p.x} ${p.y} ${p.timeoutFrames} ${p.timeoutMs}`;
}
export function validInput(s: any): s is InputState {
  const port = (p: any) => !!p && typeof p.overridden === 'boolean' && (p.overridden ? typeof p.owner === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(p.owner) : p.owner === null) && integer(p.buttons, 0, 0x3fff) && integer(p.x, -32767, 32767) && integer(p.y, -32767, 32767) && (p.expires_frame === null || integer(p.expires_frame, 0, Number.MAX_SAFE_INTEGER));
  return !!s && Array.isArray(s.ports) && s.ports.length === 4 && s.ports.every((p: any, i: number) => p.port === i + 1 && port(p.pending) && port(p.latched)) && integer(s.epoch, 1, Number.MAX_SAFE_INTEGER) && integer(s.frame, 0, Number.MAX_SAFE_INTEGER) && typeof s.neutral_during_operations === 'boolean' && typeof s.operation_running === 'boolean' && integer(s.human_takeovers, 0, Number.MAX_SAFE_INTEGER);
}
