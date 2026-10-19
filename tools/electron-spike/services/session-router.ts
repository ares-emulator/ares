import { METHODS, ServiceError, type SessionIdentity, type SessionMethod, type SessionReply } from './contracts';
import type { NativeSessionClient } from './native-client';

/** Electron main owns this router, independently of renderer/plugin lifetimes. */
export class SessionRouter {
  private client?: NativeSessionClient;
  attach(client: NativeSessionClient): void { this.detach(); this.client = client; }
  detach(): void { this.client?.close(); this.client = undefined; }
  async request(method: SessionMethod, identity?: SessionIdentity): Promise<SessionReply> {
    if (!METHODS.includes(method)) throw new ServiceError('invalid_request');
    if (!this.client) throw new ServiceError('native_host_disconnected');
    return this.client.request(method, identity);
  }
}
