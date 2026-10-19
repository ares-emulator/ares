/** Shared contract for Electron routing, the inspector and the future MCP adapter. */
export const PROTOCOL = 1 as const;
export const METHODS = ['session.status', 'session.pause', 'session.resume', 'session.reset'] as const;
export type SessionMethod = typeof METHODS[number];
export interface SessionIdentity { session: string; epoch: number }
export interface SessionStatus {
  paused: boolean;
  system: string;
  synthetic: boolean;
  capabilities: SessionMethod[];
}
export interface SessionReply extends SessionIdentity {
  protocol: typeof PROTOCOL;
  id: string;
  ok: boolean;
  result?: SessionStatus;
  error?: string;
}
export interface SessionClient {
  request(method: SessionMethod, identity?: SessionIdentity): Promise<SessionReply>;
}
export class ServiceError extends Error {
  constructor(public readonly code: string) { super(code); this.name = 'ServiceError'; }
}
const token = (value: unknown): value is string => typeof value === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(value);
export function encodeRequest(id: string, identity: SessionIdentity, method: SessionMethod): string {
  if (!identity || typeof identity !== 'object' || !token(id) || !token(identity.session) || !Number.isSafeInteger(identity.epoch) || identity.epoch < 1 ||
      !METHODS.includes(method)) throw new ServiceError('invalid_request');
  return `control ${PROTOCOL} ${id} ${identity.session} ${identity.epoch} ${method}\n`;
}
export function decodeReply(line: string): SessionReply {
  let r: SessionReply;
  try { r = JSON.parse(line); } catch { throw new ServiceError('invalid_reply'); }
  if (!r || r.protocol !== PROTOCOL || !token(r.id) || !token(r.session) || !Number.isSafeInteger(r.epoch) || r.epoch < 1 ||
      typeof r.ok !== 'boolean') throw new ServiceError('invalid_reply');
  if (r.ok) {
    const s = r.result;
    if (!s || typeof s.paused !== 'boolean' || typeof s.system !== 'string' || typeof s.synthetic !== 'boolean' ||
        !Array.isArray(s.capabilities) || s.capabilities.some(m => !METHODS.includes(m)) || r.error !== undefined)
      throw new ServiceError('invalid_reply');
  } else if (typeof r.error !== 'string' || r.result !== undefined) throw new ServiceError('invalid_reply');
  return r;
}
