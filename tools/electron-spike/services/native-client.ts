import type { Readable, Writable } from 'node:stream';
import { decodeReply, encodeRequest, ServiceError, type SessionClient, type SessionIdentity, type SessionMethod, type SessionReply } from './contracts';

/** One native process connection. Bounded, correlated requests; never retries mutations. */
export class NativeSessionClient implements SessionClient {
  private nextId = 0;
  private buffer = '';
  private closed = false;
  private pending = new Map<string, { resolve: (r: SessionReply) => void; reject: (e: Error) => void; timer: ReturnType<typeof setTimeout> }>();
  constructor(private input: Writable, output: Readable, public identity: SessionIdentity,
              private onFailure: (e: Error) => void, private timeoutMs = 30000) {
    output.setEncoding('utf8');
    output.on('data', (chunk: string) => {
      try {
        this.buffer += chunk;
        if (this.buffer.length > 65536) throw new ServiceError('reply_too_large');
        let end: number;
        while ((end = this.buffer.indexOf('\n')) >= 0) {
          const reply = decodeReply(this.buffer.slice(0, end)); this.buffer = this.buffer.slice(end + 1);
          if (reply.session !== this.identity.session) throw new ServiceError('wrong_session');
          const waiting = this.pending.get(reply.id);
          if (!waiting) throw new ServiceError('unexpected_reply');
          this.pending.delete(reply.id); clearTimeout(waiting.timer);
          // Errors may report a new reset epoch. Do not replay the rejected request.
          this.identity = { session: reply.session, epoch: reply.epoch };
          waiting.resolve(reply);
        }
      } catch (error) { this.fail(error as Error); }
    });
    output.on('end', () => this.fail(new ServiceError('native_host_disconnected')));
    output.on('error', error => this.fail(error));
    input.on('error', () => this.fail(new ServiceError('native_host_disconnected')));
  }
  request(method: SessionMethod, identity = this.identity): Promise<SessionReply> {
    if (this.closed) return Promise.reject(new ServiceError('native_host_disconnected'));
    if (this.pending.size >= 32 || this.input.writableLength > 16384) return Promise.reject(new ServiceError('busy'));
    const id = `req${++this.nextId}`;
    let wire: string;
    try { wire = encodeRequest(id, identity, method); } catch (error) { return Promise.reject(error); }
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => this.fail(new ServiceError('request_timeout')), this.timeoutMs);
      this.pending.set(id, { resolve, reject, timer });
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
