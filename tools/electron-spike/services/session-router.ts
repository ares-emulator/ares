import { TraceArtifacts } from './trace-artifacts.js';
import type { TraceExportParams } from './artifact-contracts.js';
import { METHODS, PROTOCOL, encodeRequest, ServiceError, type SessionIdentity, type ServiceMethod, type ServiceParams, type ServiceResult, type SessionReply } from './contracts.js';
import type { NativeSessionClient } from './native-client.js';

function mainCapabilities(reply: SessionReply<any>): void {
  if (reply.ok && Array.isArray(reply.result?.capabilities) && !reply.result.capabilities.includes('artifact.traceExport')) reply.result.capabilities.push('artifact.traceExport');
}

/** Electron main owns this router, independently of renderer/plugin lifetimes. */
export class SessionRouter {
  private artifacts = new TraceArtifacts();
  private nextArtifactId = 0;
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
        mainCapabilities(reply);
        if (this.client === client && reply.ok) this.onSessionState?.(reply);
      } catch { /* NativeSessionClient owns disconnect/timeout handling. */ }
      finally { pending = false; }
    }, 250);
    this.stateTimer.unref();
  }
  detach(): void {
    this.artifacts.close();
    clearInterval(this.stateTimer); this.stateTimer = undefined;
    this.client?.close(); this.client = undefined;
  }
  async request<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>, owner = 'ui'): Promise<SessionReply<ServiceResult<M>>> {
    if (!METHODS.includes(method)) throw new ServiceError('invalid_request');
    if (method === 'execution.release') this.artifacts.release(owner);
    if (!this.client) throw new ServiceError('native_host_disconnected');
    const client = this.client;
    if (method === 'artifact.traceExport') {
      const startingIdentity = { ...(identity ?? client.identity) };
      encodeRequest('validation', startingIdentity, method, params, owner);
      const envelope = { protocol: PROTOCOL, id: `artifact${++this.nextArtifactId}`, ...startingIdentity };
      try {
        const result = await this.artifacts.export(client, startingIdentity, params as TraceExportParams, owner, () => this.client === client);
        return { ...envelope, ok: true, result } as SessionReply<ServiceResult<M>>;
      } catch (error) { return { ...envelope, ...client.identity, ok: false, error: error instanceof ServiceError ? error.code : 'write_failed' }; }
    }
    const reply = await client.request(method, identity, params, owner);
    if (method.startsWith('session.')) mainCapabilities(reply);
    if (this.client === client && reply.ok && method.startsWith('session.')) this.onSessionState?.(reply as SessionReply);
    if (this.client === client && reply.ok && method.startsWith('execution.') &&
        (method !== 'execution.status' || (reply.result as { state?: string })?.state === 'done')) {
      const state = await client.request('session.status');
      mainCapabilities(state);
      if (this.client === client && state.ok) this.onSessionState?.(state);
    }
    return reply;
  }
}
