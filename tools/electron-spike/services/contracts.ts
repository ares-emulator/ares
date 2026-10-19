/** Shared contract for Electron routing, the inspector and the future MCP adapter. */
export const PROTOCOL = 1 as const;
export const MAX_TRANSFER = 1024;
export const METHODS = ['session.status', 'session.pause', 'session.resume', 'session.reset', 'memory.regions', 'memory.read', 'memory.write'] as const;
export type ServiceMethod = typeof METHODS[number];
export type SessionMethod = Extract<ServiceMethod, `session.${string}`>;
export interface SessionIdentity { session: string; epoch: number }
export interface SessionStatus { paused: boolean; system: string; synthetic: boolean; capabilities: ServiceMethod[] }
export interface MemoryRegion { id: string; name: string; size: number; readable: boolean; writable: boolean; addressing: 'region-offset'; unavailableReason?: string }
export interface MemoryRead { region: string; offset: number; length: number; hex: string }
export interface MemoryWritten { region: string; offset: number; written: number }
export interface ServiceMap {
  'session.status': { params: undefined; result: SessionStatus };
  'session.pause': { params: undefined; result: SessionStatus };
  'session.resume': { params: undefined; result: SessionStatus };
  'session.reset': { params: undefined; result: SessionStatus };
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
  request<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>): Promise<SessionReply<ServiceResult<M>>>;
}
export class ServiceError extends Error {
  constructor(public readonly code: string) { super(code); this.name = 'ServiceError'; }
}
const token = (value: unknown): value is string => typeof value === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(value);
const integer = (value: unknown): value is number => Number.isSafeInteger(value) && (value as number) >= 0;
const hex = (value: unknown): value is string => typeof value === 'string' && /^(?:[a-fA-F0-9]{2})+$/.test(value) && value.length <= MAX_TRANSFER * 2;
const keys = (p: unknown, names: string[]): boolean => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names.sort().join(',');
export function encodeRequest<M extends ServiceMethod>(id: string, identity: SessionIdentity, method: M, params?: ServiceParams<M>): string {
  if (!identity || typeof identity !== 'object' || !token(id) || !token(identity.session) || !integer(identity.epoch) || identity.epoch < 1 ||
      !METHODS.includes(method)) throw new ServiceError('invalid_request');
  let suffix = '';
  if (method === 'memory.read') {
    const p = params as ServiceParams<'memory.read'>;
    if (!keys(p, ['region', 'offset', 'length']) || !token(p.region) || !integer(p.offset) || !integer(p.length) || p.length < 1 || p.length > MAX_TRANSFER) throw new ServiceError('invalid_request');
    suffix = ` ${p.region} ${p.offset} ${p.length}`;
  } else if (method === 'memory.write') {
    const p = params as ServiceParams<'memory.write'>;
    if (!keys(p, ['region', 'offset', 'hex']) || !token(p.region) || !integer(p.offset) || !hex(p.hex)) throw new ServiceError('invalid_request');
    suffix = ` ${p.region} ${p.offset} ${p.hex}`;
  } else if (params !== undefined) throw new ServiceError('invalid_request');
  return `control ${PROTOCOL} ${id} ${identity.session} ${identity.epoch} ${method}${suffix}\n`;
}
export function decodeReply<M extends ServiceMethod = 'session.status'>(line: string, method: M = 'session.status' as M): SessionReply<ServiceResult<M>> {
  let r: SessionReply<any>;
  try { r = JSON.parse(line); } catch { throw new ServiceError('invalid_reply'); }
  if (!r || r.protocol !== PROTOCOL || !token(r.id) || !token(r.session) || !integer(r.epoch) || r.epoch < 1 || typeof r.ok !== 'boolean') throw new ServiceError('invalid_reply');
  if (r.ok) {
    const s = r.result;
    let valid = false;
    if (method.startsWith('session.')) valid = !!s && typeof s.paused === 'boolean' && typeof s.system === 'string' && typeof s.synthetic === 'boolean' && Array.isArray(s.capabilities) && s.capabilities.every((m: ServiceMethod) => METHODS.includes(m));
    if (method === 'memory.regions') valid = !!s && Array.isArray(s.regions) && s.regions.length <= 128 && s.regions.every((region: MemoryRegion) => region && token(region.id) && typeof region.name === 'string' && integer(region.size) && typeof region.readable === 'boolean' && typeof region.writable === 'boolean' && region.addressing === 'region-offset' && (region.unavailableReason === undefined || typeof region.unavailableReason === 'string'));
    if (method === 'memory.read') valid = !!s && token(s.region) && integer(s.offset) && hex(s.hex) && s.length === s.hex.length / 2;
    if (method === 'memory.write') valid = !!s && token(s.region) && integer(s.offset) && integer(s.written) && s.written > 0 && s.written <= MAX_TRANSFER;
    if (!valid || r.error !== undefined) throw new ServiceError('invalid_reply');
  } else if (typeof r.error !== 'string' || r.result !== undefined) throw new ServiceError('invalid_reply');
  return r;
}
