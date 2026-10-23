import { spawn, type ChildProcess } from 'node:child_process'
import net from 'node:net'
import fs from 'node:fs/promises'
import { constants } from 'node:fs'
import path from 'node:path'
import os from 'node:os'
import { randomBytes, randomUUID, timingSafeEqual } from 'node:crypto'
import {
  decodeReply,
  encodeRequest,
  ServiceError,
  type SessionClient,
  type SessionIdentity,
  type ServiceMethod,
  type ServiceParams,
  type ServiceResult,
  type SessionReply,
} from './contracts.js'
const MAX_LINE = 196608
const token = (v: unknown): v is string =>
  typeof v === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(v)
export interface ConnectionInfo {
  protocol: 1
  instance: string
  endpoint: string
  token: string
}
export const defaultServiceDirectory = () =>
  path.join(
    process.platform === 'darwin' ? '/tmp' : os.tmpdir(),
    `ares-services-${process.getuid?.() ?? 'user'}`,
  )
function privateFile(stat: { uid: number; mode: number }): void {
  if (process.platform === 'win32')
    throw new ServiceError('unsupported_platform')
  if (stat.uid !== process.getuid!() || stat.mode & 0o077)
    throw new ServiceError('insecure_service_permissions')
}
export async function readConnection(
  file: string,
  instance: string,
): Promise<ConnectionInfo> {
  try {
    const parent = await fs.lstat(path.dirname(file))
    privateFile(parent)
    if (!parent.isDirectory() || parent.isSymbolicLink())
      throw new ServiceError('invalid_connection_file')
    const handle = await fs.open(
      file,
      constants.O_RDONLY | constants.O_NOFOLLOW,
    )
    try {
      const stat = await handle.stat()
      privateFile(stat)
      if (!stat.isFile() || stat.size > 4096)
        throw new ServiceError('invalid_connection_file')
      const info = JSON.parse(await handle.readFile('utf8')) as ConnectionInfo
      if (
        info.protocol !== 1 ||
        !token(info.instance) ||
        !/^[a-f0-9]{64}$/.test(info.token) ||
        !path.isAbsolute(info.endpoint) ||
        info.endpoint.length > 100 ||
        path.dirname(info.endpoint) !== path.dirname(file)
      )
        throw new ServiceError('invalid_connection_file')
      if (info.instance !== instance) throw new ServiceError('wrong_instance')
      return info
    } finally {
      await handle.close()
    }
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code === 'ENOENT')
      throw new ServiceError('app_not_running')
    if (error instanceof ServiceError) throw error
    throw new ServiceError('invalid_connection_file')
  }
}
function lines(socket: net.Socket, receive: (message: any) => void): void {
  let buffer = ''
  socket.setEncoding('utf8')
  socket.on('data', (data) => {
    buffer += data
    let end: number
    try {
      while ((end = buffer.indexOf('\n')) >= 0) {
        if (end > MAX_LINE) throw new ServiceError('message_too_large')
        const line = buffer.slice(0, end)
        buffer = buffer.slice(end + 1)
        receive(JSON.parse(line))
      }
      if (buffer.length > MAX_LINE) throw new ServiceError('message_too_large')
    } catch {
      socket.destroy(new ServiceError('invalid_message'))
    }
  })
}
function send(socket: net.Socket, message: unknown): void {
  const wire = JSON.stringify(message) + '\n'
  if (socket.destroyed || socket.writableLength + wire.length > 196608) {
    socket.destroy()
    return
  }
  socket.write(wire)
}
export function namedConnection(directory: string, name: string): string {
  if (!/^[a-zA-Z0-9][a-zA-Z0-9_-]{0,47}$/.test(name) || !path.isAbsolute(directory)) throw new ServiceError('invalid_instance_name');
  return path.join(directory, `named-${name}.json`);
}
async function connectionInstance(file: string): Promise<string> {
  const parent = await fs.lstat(path.dirname(file)); privateFile(parent);
  if (!parent.isDirectory() || parent.isSymbolicLink()) throw new ServiceError('invalid_connection_file');
  const h = await fs.open(file, constants.O_RDONLY | constants.O_NOFOLLOW);
  try { const st = await h.stat(); privateFile(st); if (!st.isFile() || st.size > 4096) throw new ServiceError('invalid_connection_file'); return JSON.parse(await h.readFile('utf8')).instance; }
  finally { await h.close(); }
}
/** Opt-in, private local endpoint. No renderer-owned backend and no TCP listener. */
export class LocalServiceHost {
  readonly instance = randomUUID()
  private server = net.createServer((socket) => this.accept(socket))
  private sockets = new Set<net.Socket>()
  private claimLock?: ChildProcess
  private claimFile?: string
  info?: ConnectionInfo
  metadataFile?: string
  constructor(
    private client: SessionClient,
    private manage?: (params: any) => Promise<unknown>,
  ) {}
  async listen(directory = defaultServiceDirectory(), name?: string, lockExecutable?: string): Promise<string> {
    if (process.platform === 'win32')
      throw new ServiceError('unsupported_platform')
    directory = path.resolve(directory)
    await fs.mkdir(directory, { recursive: true, mode: 0o700 })
    const stat = await fs.lstat(directory)
    privateFile(stat)
    if (!stat.isDirectory() || stat.isSymbolicLink())
      throw new ServiceError('insecure_service_permissions')
    if (name) {
      this.claimFile = namedConnection(directory, name);
      if (!lockExecutable) throw new ServiceError('service_lock_unavailable');
      const child = spawn(lockExecutable, ['--service-lock', path.join(directory, `named-${name}.lock`)], { stdio: ['pipe', 'pipe', 'pipe'] });
      this.claimLock = child;
      try {
        await new Promise<void>((resolve, reject) => {
          const timer = setTimeout(() => { child.kill(); reject(new ServiceError('instance_claim_timeout')); }, 3000);
          child.once('error', e => { clearTimeout(timer); reject(e); });
          child.once('exit', () => { clearTimeout(timer); reject(new ServiceError('instance_name_in_use')); });
          child.stdout!.once('data', b => { clearTimeout(timer); b.toString() === 'locked\n' ? resolve() : reject(new ServiceError('service_lock_unavailable')); });
        });
        // Probe the existing private record with its own fresh credentials. A live
        // pre-existing owner is never displaced, even if its claim lock was lost.
        try {
          const instance = await connectionInstance(this.claimFile);
          await readConnection(this.claimFile, instance);
          const probe = new LocalServiceClient(this.claimFile, instance, 1000);
          let live = false;
          try { await probe.request('session.status'); live = true; }
          catch (e) {
            // Only a refused/missing endpoint proves the old launch is gone.
            // Authentication failures, saturation and timeouts are inconclusive.
            if (!(e instanceof ServiceError && e.code === 'app_not_running')) throw new ServiceError('instance_liveness_uncertain');
          } finally { probe.close(); }
          if (live) throw new ServiceError('instance_name_in_use');
        } catch (e) {
          if (!(e instanceof ServiceError && e.code === 'app_not_running') && (e as NodeJS.ErrnoException).code !== 'ENOENT') throw e;
        }
      } catch (e) { child.stdin!.end(); this.claimLock = undefined; throw e; }
    }
    try {
    const endpoint = path.join(directory, `${this.instance}.sock`)
    if (endpoint.length > 100) throw new ServiceError('service_path_too_long')
    this.info = {
      protocol: 1,
      instance: this.instance,
      endpoint,
      token: randomBytes(32).toString('hex'),
    }
    await new Promise<void>((resolve, reject) => {
      this.server.once('error', reject)
      this.server.listen(endpoint, () => {
        this.server.off('error', reject)
        resolve()
      })
    })
    await fs.chmod(endpoint, 0o600)
    this.metadataFile = path.join(directory, `${this.instance}.json`)
    try {
      await fs.writeFile(this.metadataFile, JSON.stringify(this.info), {
        mode: 0o600,
        flag: 'wx',
      })
    } catch (error) {
      await this.close()
      throw error
    }
    if (this.claimFile) {
      const temporary = `${this.claimFile}.${this.instance}.tmp`;
      try {
        await fs.writeFile(temporary, JSON.stringify(this.info), { mode: 0o600, flag: 'wx' });
        await fs.rename(temporary, this.claimFile);
      } catch (e) { await fs.rm(temporary, {force: true}); await this.close(); throw e; }
    }
    return this.metadataFile
    } catch (e) { await this.close(); throw e; }
  }
  async close(): Promise<void> {
    if (this.claimFile && this.claimLock) {
      try { if (await connectionInstance(this.claimFile) === this.instance) await fs.rm(this.claimFile); } catch (e) { if ((e as NodeJS.ErrnoException).code !== 'ENOENT') throw e; }
    }
    this.claimLock?.stdin?.end(); this.claimLock = undefined;
    for (const socket of this.sockets) socket.destroy()
    if (this.server.listening)
      await new Promise<void>((resolve) => this.server.close(() => resolve()))
    if (this.metadataFile) await fs.rm(this.metadataFile, { force: true })
    if (this.info) await fs.rm(this.info.endpoint, { force: true })
  }
  private accept(socket: net.Socket): void {
    if (this.sockets.size >= 8) {
      socket.destroy()
      return
    }
    this.sockets.add(socket)
    const connection = randomUUID(),
      seen = new Set<string>()
    let authenticated = false,
      inFlight = 0,
      lastLocalId = 0
    const helloTimeout = setTimeout(() => socket.destroy(), 3000)
    socket.on('error', () => {})
    socket.once('close', () => {
      clearTimeout(helloTimeout)
      this.sockets.delete(socket)
      if (authenticated)
        void this.client
          .request('execution.release', undefined, undefined, connection)
          .catch(() => {})
    })
    lines(socket, (message) => {
      const error = (code: string, end = false) => {
        send(socket, {
          protocol: 1,
          type: 'reply',
          id: token(message?.id) ? message.id : 'invalid',
          connection,
          ok: false,
          error: code,
        })
        if (end) socket.end()
      }
      if (!message || message.protocol !== 1 || !token(message.id)) {
        error('invalid_request', true)
        return
      }
      if (!authenticated) {
        if (message.type !== 'hello' || message.instance !== this.instance) {
          error('wrong_instance', true)
          return
        }
        if (
          typeof message.token !== 'string' ||
          !/^[a-f0-9]{64}$/.test(message.token) ||
          !timingSafeEqual(
            Buffer.from(message.token),
            Buffer.from(this.info!.token),
          )
        ) {
          error('unauthorized', true)
          return
        }
        authenticated = true
        clearTimeout(helloTimeout)
        send(socket, {
          protocol: 1,
          type: 'hello',
          id: message.id,
          instance: this.instance,
          connection,
          ok: true,
        })
        return
      }
      if (message.type !== 'request' || message.connection !== connection) {
        error('stale_connection')
        return
      }
      // Our clients use monotonically increasing localN IDs. A high-water mark
      // rejects replays with constant storage, permitting long operation polling.
      // Arbitrary third-party IDs retain the bounded deduplication quota.
      const localId = /^local([0-9]+)$/.exec(message.id)
      if (localId) {
        const sequence = Number(localId[1])
        if (!Number.isSafeInteger(sequence) || sequence < 1) {
          error('invalid_request')
          return
        }
        if (sequence <= lastLocalId) {
          error('duplicate_request')
          return
        }
        lastLocalId = sequence
      } else {
        if (seen.has(message.id)) {
          error('duplicate_request')
          return
        }
        if (seen.size >= 1024) {
          error('connection_request_limit', true)
          return
        }
        seen.add(message.id)
      }
      if (inFlight >= 16) {
        error('request_overload')
        return
      }
      if (message.method === 'plugin.manage') {
        if (!this.manage) {
          error('unavailable')
          return
        }
        ++inFlight
        void this.manage(message.params)
          .then(
            (result) => {
              if (Buffer.byteLength(JSON.stringify(result)) > 48000) {
                error('plugin_result_too_large; request a smaller page')
                return
              }
              if (!socket.destroyed)
                send(socket, {
                  protocol: 1,
                  type: 'reply',
                  id: message.id,
                  connection,
                  ok: true,
                  result,
                })
            },
            (failure) =>
              error(
                String(
                  failure instanceof Error ? failure.message : failure,
                ).slice(0, 2048),
              ),
          )
          .finally(() => --inFlight)
        return
      }
      // Reset and owner-release stay main-owned actions. Owners come from this
      // authenticated connection generation, never from untrusted request data.
      if (
        [
          'session.reset',
          'execution.release',
          'execution.batchBegin',
          'execution.batchCheck',
        ].includes(message.method)
      ) {
        error('unavailable')
        return
      }
      if (
        !['session.status', 'memory.regions'].includes(message.method) &&
        !message.identity
      ) {
        error('session_required')
        return
      }
      try {
        encodeRequest(
          'validation',
          message.identity ?? { session: 'discovery', epoch: 1 },
          message.method,
          message.params,
        )
      } catch {
        error('invalid_request')
        return
      }
      ++inFlight
      void this.client
        .request(message.method, message.identity, message.params, connection)
        .then(
          (reply) => {
            if (!socket.destroyed)
              send(socket, {
                protocol: 1,
                type: 'reply',
                id: message.id,
                connection,
                ok: true,
                reply,
              })
          },
          (failure) =>
            error(
              failure instanceof ServiceError
                ? failure.code
                : 'native_host_disconnected',
            ),
        )
        .finally(() => --inFlight)
    })
  }
}
/** Reconnects only for a NEW request. Never re-sends a pending or timed-out request. */
export class LocalServiceClient implements SessionClient {
  identity?: SessionIdentity
  private socket?: net.Socket
  private connection?: string
  private connecting?: Promise<void>
  private nextId = 0
  private stopped = false
  private generation = 0
  private pending = new Map<
    string,
    {
      method: ServiceMethod | 'plugin.manage'
      resolve: (r: SessionReply<any>) => void
      reject: (e: Error) => void
      timer: ReturnType<typeof setTimeout>
    }
  >()
  constructor(
    readonly metadataFile: string,
    readonly instance: string,
    private timeoutMs = 30000,
    private named = false,
  ) {}
  static named(directory: string, name: string, timeoutMs = 30000): LocalServiceClient {
    return new LocalServiceClient(namedConnection(directory, name), '', timeoutMs, true);
  }
  onDisconnect?: () => void
  close(): void {
    this.stopped = true
    this.drop(new ServiceError('app_disconnected'))
  }
  disconnect(): void {
    this.drop(new ServiceError('app_disconnected'))
  }
  private drop(error: Error): void {
    ++this.generation
    this.connection = undefined
    this.identity = undefined
    this.onDisconnect?.()
    this.socket?.destroy()
    this.socket = undefined
    for (const p of this.pending.values()) {
      clearTimeout(p.timer)
      p.reject(error)
    }
    this.pending.clear()
  }
  private async connect(): Promise<void> {
    if (this.stopped) throw new ServiceError('app_disconnected')
    if (this.socket && this.connection) return
    if (this.connecting) return this.connecting
    const generation = this.generation
    this.connecting = (async () => {
      let selected = this.instance;
      if (this.named) {
        try { selected = await connectionInstance(this.metadataFile); }
        catch (e) { if ((e as NodeJS.ErrnoException).code === 'ENOENT') throw new ServiceError('instance_not_running'); throw e; }
      }
      const info = await readConnection(this.metadataFile, selected)
      if (this.stopped || this.generation !== generation)
        throw new ServiceError('app_disconnected')
      await new Promise<void>((resolve, reject) => {
        const socket = net.createConnection(info.endpoint)
        this.socket = socket
        let ready = false
        const timer = setTimeout(
          () => socket.destroy(new ServiceError('request_timeout')),
          this.timeoutMs,
        )
        socket.on('error', (error) => {
          const failure =
            error instanceof ServiceError
              ? error
              : new ServiceError(ready ? 'app_disconnected' : 'app_not_running')
          if (this.socket === socket) this.drop(failure)
          clearTimeout(timer)
          reject(failure)
        })
        socket.once('close', () => {
          if (this.socket === socket)
            this.drop(new ServiceError('app_disconnected'))
          clearTimeout(timer)
          if (!ready) reject(new ServiceError('app_disconnected'))
        })
        socket.once('connect', () =>
          send(socket, {
            protocol: 1,
            type: 'hello',
            id: 'hello',
            instance: selected,
            token: info.token,
          }),
        )
        lines(socket, (message) => {
          if (this.socket !== socket) return
          if (!ready) {
            clearTimeout(timer)
            if (
              message.ok !== true ||
              message.type !== 'hello' ||
              message.id !== 'hello' ||
              message.protocol !== 1 ||
              message.instance !== selected ||
              !token(message.connection)
            ) {
              const error = new ServiceError(message.error || 'wrong_instance')
              socket.destroy()
              reject(error)
              return
            }
            this.connection = message.connection
            ready = true
            resolve()
            return
          }
          if (
            message.protocol !== 1 ||
            message.type !== 'reply' ||
            message.connection !== this.connection
          ) {
            socket.destroy(new ServiceError('stale_connection'))
            return
          }
          const waiting = this.pending.get(message.id)
          if (!waiting) {
            socket.destroy(new ServiceError('unexpected_reply'))
            return
          }
          this.pending.delete(message.id)
          clearTimeout(waiting.timer)
          if (message.ok === false && typeof message.error === 'string') {
            const error = new ServiceError(message.error)
            waiting.reject(error)
            if (message.error === 'connection_request_limit') this.drop(error)
            return
          }
          try {
            if (message.ok !== true) throw new ServiceError('invalid_reply')
            if (waiting.method === 'plugin.manage') {
              waiting.resolve(message.result)
              return
            }
            const reply = decodeReply(
              JSON.stringify(message.reply),
              waiting.method,
            )
            if (reply.ok)
              this.identity = { session: reply.session, epoch: reply.epoch }
            waiting.resolve(reply)
          } catch (error) {
            waiting.reject(error as Error)
            socket.destroy()
          }
        })
      })
    })()
    try {
      await this.connecting
    } finally {
      this.connecting = undefined
    }
  }
  async manage(params: unknown): Promise<any> {
    await this.connect()
    if (this.pending.size >= 16) throw new ServiceError('request_overload')
    const socket = this.socket,
      connection = this.connection
    if (!socket || !connection || socket.destroyed)
      throw new ServiceError('app_disconnected')
    const id = `local${++this.nextId}`
    return new Promise((resolve, reject) => {
      const timer = setTimeout(
        () => this.drop(new ServiceError('request_timeout')),
        this.timeoutMs,
      )
      this.pending.set(id, { method: 'plugin.manage', resolve, reject, timer })
      send(socket, {
        protocol: 1,
        type: 'request',
        connection,
        id,
        method: 'plugin.manage',
        params,
      })
    })
  }
  async request<M extends ServiceMethod>(
    method: M,
    identity?: SessionIdentity,
    params?: ServiceParams<M>,
  ): Promise<SessionReply<ServiceResult<M>>> {
    await this.connect()
    if (this.pending.size >= 16) throw new ServiceError('request_overload')
    if (!['session.status', 'memory.regions'].includes(method))
      identity ??= this.identity
    if (!identity && !['session.status', 'memory.regions'].includes(method))
      throw new ServiceError('session_required')
    encodeRequest(
      'validation',
      identity ?? { session: 'discovery', epoch: 1 },
      method,
      params,
    )
    const socket = this.socket,
      connection = this.connection
    if (!socket || !connection || socket.destroyed)
      throw new ServiceError('app_disconnected')
    const id = `local${++this.nextId}`
    return new Promise((resolve, reject) => {
      const timeout =
        method === 'experiment.batch'
          ? Math.max(
              this.timeoutMs,
              (params as { timeoutMs: number }).timeoutMs + 5000,
            )
          : this.timeoutMs
      const timer = setTimeout(
        () => this.drop(new ServiceError('request_timeout')),
        timeout,
      )
      this.pending.set(id, { method, resolve, reject, timer })
      send(socket, {
        protocol: 1,
        type: 'request',
        connection,
        id,
        method,
        identity,
        params,
      })
    })
  }
}
