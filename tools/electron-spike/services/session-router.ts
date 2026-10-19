import { METHODS, ServiceError, type SessionIdentity, type ServiceMethod, type ServiceParams, type ServiceResult, type SessionReply } from './contracts.js';
import type { NativeSessionClient } from './native-client.js';

/** Electron main owns this router, independently of renderer/plugin lifetimes. */
export class SessionRouter {
  private client?: NativeSessionClient;
  private stateTimer?: ReturnType<typeof setInterval>;
  constructor(private onSessionState?: (reply: SessionReply) => void) {}
  attach(client: NativeSessionClient): void {
    this.detach(); this.client = client;
    if (!this.onSessionState) return;
    // CPU break/watch hits can stop ordinary play without a request or a video
    // frame. Main observes that state independently of the renderer's lifetime.
    let pending = false;
    this.stateTimer = setInterval(async () => {
      if (pending || this.client !== client) return;
      pending = true;
      try {
        const reply = await client.request('session.status');
        if (this.client === client && reply.ok) this.onSessionState?.(reply);
      } catch { /* NativeSessionClient owns disconnect/timeout handling. */ }
      finally { pending = false; }
    }, 250);
    this.stateTimer.unref();
  }
  detach(): void {
    clearInterval(this.stateTimer); this.stateTimer = undefined;
    this.client?.close(); this.client = undefined;
  }
  async request<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>, owner = 'ui'): Promise<SessionReply<ServiceResult<M>>> {
    if (!METHODS.includes(method)) throw new ServiceError('invalid_request');
    if (!this.client) throw new ServiceError('native_host_disconnected');
    const client = this.client;
    const reply = await client.request(method, identity, params, owner);
    if (this.client === client && reply.ok && method.startsWith('session.')) this.onSessionState?.(reply as SessionReply);
    if (this.client === client && reply.ok && method.startsWith('execution.') &&
        (method !== 'execution.status' || (reply.result as { state?: string })?.state === 'done')) {
      const state = await client.request('session.status');
      if (this.client === client && state.ok) this.onSessionState?.(state);
    }
    return reply;
  }
}
