import { INPUT_METHODS, encodeInput, validInput, type InputMethod, type InputServiceMap } from './input-contracts.js';
import { N64_METHODS, encodeN64, validN64, type N64Method, type N64ServiceMap, type Stop, validStop } from './n64-contracts.js';
/** Shared contract for Electron routing, the inspector and the future MCP adapter. */
export const PROTOCOL = 1 as const;
export const MAX_TRANSFER = 1024;
export const METHODS = ['session.status', 'session.pause', 'session.resume', 'session.reset', 'memory.regions', 'memory.read', 'memory.write', 'execution.runFrames', 'execution.runUntilFrame', 'execution.status', 'execution.cancel', 'execution.release', 'execution.stepInstructions', ...N64_METHODS, ...INPUT_METHODS] as const;
export type ServiceMethod = typeof METHODS[number];
export type SessionMethod = Extract<ServiceMethod, `session.${string}`>;
export interface SessionIdentity { session: string; epoch: number }
export interface SessionStatus { paused: boolean; system: string; synthetic: boolean; capabilities: ServiceMethod[]; frame?: number; frame_kind?: string; at_frame_boundary?: boolean; executor?: string; operation_id?: number | null }
export interface ExecutionOperation {
  operation_id: number; kind: string; owner: string; state: 'running' | 'done'; stopped: boolean;
  stop_reason: string; epoch: number; start_epoch: number; start_frame: number; target_frame: number;
  frame: number; frame_kind: string; elapsed_frames: number; start_at_frame_boundary: boolean;
  at_frame_boundary: boolean; executor: string; slices: number; elapsed_seconds: number; executed?: number; debug_stop?: Stop;
}
export interface MemoryRegion { id: string; name: string; size: number; readable: boolean; writable: boolean; addressing: 'region-offset'; unavailableReason?: string }
export interface MemoryRead { region: string; offset: number; length: number; hex: string }
export interface MemoryWritten { region: string; offset: number; written: number }
export interface ServiceMap extends N64ServiceMap, InputServiceMap {
  'execution.stepInstructions': { params: { count: number; timeoutMs: number }; result: ExecutionOperation };
  'session.status': { params: undefined; result: SessionStatus };
  'session.pause': { params: undefined; result: SessionStatus };
  'session.resume': { params: { keepOverrides: boolean } | undefined; result: SessionStatus };
  'session.reset': { params: undefined; result: SessionStatus };
  'execution.runFrames': { params: { count: number; timeoutMs: number }; result: ExecutionOperation };
  'execution.runUntilFrame': { params: { frame: number; timeoutMs: number }; result: ExecutionOperation };
  'execution.status': { params: { operationId: number }; result: ExecutionOperation };
  'execution.cancel': { params: { operationId: number }; result: ExecutionOperation };
  'execution.release': { params: undefined; result: { released: boolean } };
  'memory.regions': { params: undefined; result: { regions: MemoryRegion[] } };
  'memory.read': { params: { region: string; offset: number; length: number }; result: MemoryRead };
  'memory.write': { params: { region: string; offset: number; hex: string }; result: MemoryWritten };
}
export type ServiceParams<M extends ServiceMethod> = ServiceMap[M]['params'];
export type ServiceResult<M extends ServiceMethod> = ServiceMap[M]['result'];
export interface SessionReply<Result = SessionStatus> extends SessionIdentity {
  protocol: typeof PROTOCOL; id: string; ok: boolean; result?: Result; error?: string;
}
export interface SessionClient {
  request<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>, owner?: string): Promise<SessionReply<ServiceResult<M>>>;
}
export class ServiceError extends Error {
  constructor(public readonly code: string) { super(code); this.name = 'ServiceError'; }
}
const token = (value: unknown): value is string => typeof value === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(value);
const integer = (value: unknown): value is number => Number.isSafeInteger(value) && (value as number) >= 0;
const hex = (value: unknown): value is string => typeof value === 'string' && /^(?:[a-fA-F0-9]{2})+$/.test(value) && value.length <= MAX_TRANSFER * 2;
const keys = (p: unknown, names: string[]): boolean => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names.sort().join(',');
export function encodeRequest<M extends ServiceMethod>(id: string, identity: SessionIdentity, method: M, params?: ServiceParams<M>, owner = 'ui'): string {
  if (!identity || typeof identity !== 'object' || !token(id) || !token(identity.session) || !integer(identity.epoch) || identity.epoch < 1 ||
      !METHODS.includes(method) || !token(owner)) throw new ServiceError('invalid_request');
  let suffix = '';
  if (method === 'session.resume' && params !== undefined) {
    const p = params as { keepOverrides: boolean };
    if (!keys(p, ['keepOverrides']) || typeof p.keepOverrides !== 'boolean') throw new ServiceError('invalid_request');
    suffix = ` ${p.keepOverrides ? 1 : 0}`;
  } else if (method.startsWith('n64.input.')) {
    const encoded = encodeInput(method as InputMethod, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method.startsWith('n64.')) {
    const encoded = encodeN64(method as N64Method, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method === 'memory.read') {
    const p = params as ServiceParams<'memory.read'>;
    if (!keys(p, ['region', 'offset', 'length']) || !token(p.region) || !integer(p.offset) || !integer(p.length) || p.length < 1 || p.length > MAX_TRANSFER) throw new ServiceError('invalid_request');
    suffix = ` ${p.region} ${p.offset} ${p.length}`;
  } else if (method === 'memory.write') {
    const p = params as ServiceParams<'memory.write'>;
    if (!keys(p, ['region', 'offset', 'hex']) || !token(p.region) || !integer(p.offset) || !hex(p.hex)) throw new ServiceError('invalid_request');
    suffix = ` ${p.region} ${p.offset} ${p.hex}`;
  } else if (method === 'execution.runFrames' || method === 'execution.runUntilFrame' || method === 'execution.stepInstructions') {
    const p = params as { count?: number; frame?: number; timeoutMs: number };
    const field = method === 'execution.runUntilFrame' ? 'frame' : 'count';
    if (!keys(p, [field, 'timeoutMs']) || !integer(p[field]) || !integer(p.timeoutMs) || p.timeoutMs < 1 || p.timeoutMs > 600000 || (field === 'count' && p.count! > 1000000)) throw new ServiceError('invalid_request');
    suffix = ` ${p[field]} ${p.timeoutMs}`;
  } else if (method === 'execution.status' || method === 'execution.cancel') {
    const p = params as { operationId: number };
    if (!keys(p, ['operationId']) || !integer(p.operationId)) throw new ServiceError('invalid_request');
    suffix = ` ${p.operationId}`;
  } else if (params !== undefined) throw new ServiceError('invalid_request');
  return `control ${PROTOCOL} ${id} ${identity.session} ${identity.epoch} ${method}${suffix} owner=${owner}\n`;
}
export function decodeReply<M extends ServiceMethod = 'session.status'>(line: string, method: M = 'session.status' as M): SessionReply<ServiceResult<M>> {
  let r: SessionReply<any>;
  try { r = JSON.parse(line); } catch { throw new ServiceError('invalid_reply'); }
  if (!r || r.protocol !== PROTOCOL || !token(r.id) || !token(r.session) || !integer(r.epoch) || r.epoch < 1 || typeof r.ok !== 'boolean') throw new ServiceError('invalid_reply');
  if (r.ok) {
    const s = r.result;
    let valid = false;
    if (method.startsWith('n64.') && !method.startsWith('n64.input.')) valid = validN64(method as N64Method, s);
    if (method.startsWith('n64.input.')) valid = validInput(s);
    if (method.startsWith('session.')) valid = !!s && typeof s.paused === 'boolean' && typeof s.system === 'string' && typeof s.synthetic === 'boolean' && Array.isArray(s.capabilities) && s.capabilities.every((m: ServiceMethod) => METHODS.includes(m));
    if (method === 'execution.release') valid = !!s && s.released === true;
    if (method.startsWith('execution.') && method !== 'execution.release') valid = !!s && integer(s.operation_id) && s.operation_id > 0 && token(s.owner) && typeof s.kind === 'string' && ['running', 'done'].includes(s.state) && s.stopped === (s.state === 'done') && typeof s.stop_reason === 'string' && integer(s.epoch) && integer(s.start_epoch) && integer(s.start_frame) && integer(s.target_frame) && integer(s.frame) && integer(s.elapsed_frames) && typeof s.start_at_frame_boundary === 'boolean' && typeof s.at_frame_boundary === 'boolean' && typeof s.frame_kind === 'string' && typeof s.executor === 'string' && integer(s.slices) && Number.isFinite(s.elapsed_seconds) && s.elapsed_seconds >= 0 && (s.executed === undefined || integer(s.executed)) && (s.debug_stop === undefined || validStop(s.debug_stop));
    if (method === 'memory.regions') valid = !!s && Array.isArray(s.regions) && s.regions.length <= 128 && s.regions.every((region: MemoryRegion) => region && token(region.id) && typeof region.name === 'string' && integer(region.size) && typeof region.readable === 'boolean' && typeof region.writable === 'boolean' && region.addressing === 'region-offset' && (region.unavailableReason === undefined || typeof region.unavailableReason === 'string'));
    if (method === 'memory.read') valid = !!s && token(s.region) && integer(s.offset) && hex(s.hex) && s.length === s.hex.length / 2;
    if (method === 'memory.write') valid = !!s && token(s.region) && integer(s.offset) && integer(s.written) && s.written > 0 && s.written <= MAX_TRANSFER;
    if (!valid || r.error !== undefined) throw new ServiceError('invalid_reply');
  } else if (typeof r.error !== 'string' || r.result !== undefined) throw new ServiceError('invalid_reply');
  return r;
}
