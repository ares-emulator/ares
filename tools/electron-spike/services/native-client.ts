import type { Readable, Writable } from 'node:stream';
import { decodeReply, encodeRequest, ServiceError, type SessionClient, type SessionIdentity, type ServiceMethod, type ServiceParams, type ServiceResult, type SessionReply } from './contracts';

/** One native process connection. Bounded, correlated requests; never retries mutations. */
export class NativeSessionClient implements SessionClient {
  private nextId = 0;
  private buffer = '';
  private closed = false;
  private pending = new Map<string, { method: ServiceMethod; resolve: (r: SessionReply<unknown>) => void; reject: (e: Error) => void; timer: ReturnType<typeof setTimeout> }>();
  constructor(private input: Writable, output: Readable, public identity: SessionIdentity,
              private onFailure: (e: Error) => void, private timeoutMs = 30000) {
    output.setEncoding('utf8');
    output.on('data', (chunk: string) => {
      try {
        this.buffer += chunk;
        let end: number;
        while ((end = this.buffer.indexOf('\n')) >= 0) {
          if (end > 65536) throw new ServiceError('reply_too_large');
          const line = this.buffer.slice(0, end); this.buffer = this.buffer.slice(end + 1);
          const id = JSON.parse(line)?.id;
          const waiting = this.pending.get(id);
          if (!waiting) throw new ServiceError('unexpected_reply');
          const reply = decodeReply(line, waiting.method);
          if (reply.session !== this.identity.session) throw new ServiceError('wrong_session');
          this.pending.delete(reply.id); clearTimeout(waiting.timer);
          // Errors may report a new reset epoch. Do not replay the rejected request.
          this.identity = { session: reply.session, epoch: reply.epoch };
          waiting.resolve(reply);
        }
        if (this.buffer.length > 65536) throw new ServiceError('reply_too_large');
      } catch (error) { this.fail(error as Error); }
    });
    output.on('end', () => this.fail(new ServiceError('native_host_disconnected')));
    output.on('error', error => this.fail(error));
    input.on('error', () => this.fail(new ServiceError('native_host_disconnected')));
  }
  request<M extends ServiceMethod>(method: M, identity = this.identity, params?: ServiceParams<M>, owner = 'ui'): Promise<SessionReply<ServiceResult<M>>> {
    if (this.closed) return Promise.reject(new ServiceError('native_host_disconnected'));
    // Reserve capacity for the endpoint's eight owner revocations. If even the
    // bounded reserve is exhausted, fail closed: main terminates the host rather
    // than leaving disconnected execution running until its deadline.
    const release = method === 'execution.release';
    if (this.pending.size >= (release ? 40 : 32) || this.input.writableLength > (release ? 49152 : 16384)) {
      if (release) this.fail(new ServiceError('owner_revocation_failed'));
      return Promise.reject(new ServiceError(release ? 'owner_revocation_failed' : 'busy'));
    }
    const id = `req${++this.nextId}`;
    let wire: string;
    try { wire = encodeRequest(id, identity, method, params, owner); } catch (error) { return Promise.reject(error); }
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => this.fail(new ServiceError('request_timeout')), this.timeoutMs);
      this.pending.set(id, { method, resolve: reply => resolve(reply as SessionReply<ServiceResult<M>>), reject, timer });
      this.input.write(wire, error => { if (error) this.fail(error); });
    });
  }
  close(): void { this.fail(new ServiceError('native_host_disconnected'), false); }
  private fail(error: Error, notify = true): void {
    if (this.closed) return;
    this.closed = true;
    for (const p of this.pending.values()) { clearTimeout(p.timer); p.reject(error); }
    this.pending.clear();
    if (notify) this.onFailure(error);
  }
}
