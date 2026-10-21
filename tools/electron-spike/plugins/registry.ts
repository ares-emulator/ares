import fs from 'node:fs/promises'
import path from 'node:path'
import { randomUUID } from 'node:crypto'
import {
  ServiceError,
  type SessionClient,
  type SessionIdentity,
  type SessionReply,
  type SessionStatus,
  type ServiceMethod,
} from '../services/contracts.js'
import { activation, validateManifest } from './manifest.js'
import type {
  PluginManifest,
  PluginInfo,
  PluginCatalog,
  PluginLease,
  GameIdentity,
  Capability,
} from './contracts.js'
interface Record extends PluginInfo {
  root: string
  owner?: string
  pending: number
  storageQueue: Promise<unknown>
}
export class PluginRegistry {
  private records: Record[] = []
  private enabled = new Set<string>()
  private directories: string[] = []
  private current?: {
    identity: SessionIdentity
    system: string
    game: GameIdentity | null
  }
  private renderer = false
  private mutation = Promise.resolve()
  private configurationError?: string
  constructor(
    private bundledRoot: string,
    private dataRoot: string,
    private client: SessionClient,
    private currentIdentity: () => SessionIdentity | undefined,
    private changed: (catalog: PluginCatalog) => void = () => {},
  ) {}
  private configFile() {
    return path.join(this.dataRoot, 'enabled.json')
  }
  async initialize(extra: string[] = []) {
    await fs.mkdir(this.dataRoot, { recursive: true })
    this.dataRoot = await fs.realpath(this.dataRoot)
    try {
      const bytes = await fs.readFile(this.configFile(), 'utf8')
      if (bytes.length > 32768)
        throw new Error('plugin configuration exceeds 32 KiB')
      const c = JSON.parse(bytes)
      if (Array.isArray(c.directories))
        this.directories = c.directories
          .filter((s: unknown) => typeof s === 'string')
          .slice(0, 64)
      if (Array.isArray(c.enabled))
        this.enabled = new Set(
          c.enabled.filter((s: unknown) => typeof s === 'string'),
        )
    } catch (e: any) {
      if (e.code !== 'ENOENT') this.configurationError = String(e)
    }
    for (const dir of extra)
      if (!this.directories.includes(dir)) this.directories.push(dir)
    await this.scan(extra)
    return this.catalog()
  }
  private async persist() {
    const file = this.configFile(),
      temp = file + '.' + randomUUID()
    try {
      await fs.writeFile(
        temp,
        JSON.stringify({
          directories: this.directories,
          enabled: [...this.enabled],
        }),
        { flag: 'wx', mode: 0o600 },
      )
      await fs.rename(temp, file)
    } finally {
      await fs.rm(temp, { force: true })
    }
  }
  private async within(root: string, relative: string) {
    const file = await fs.realpath(path.join(root, relative))
    if (file !== root && !file.startsWith(root + path.sep))
      throw new Error('Plugin path escapes its directory')
    return file
  }
  private async descriptor(source: string, bundled: boolean): Promise<Record> {
    const base = {
      root: source,
      source,
      bundled,
      enabled: false,
      state: 'error' as const,
      logs: [],
      pending: 0,
      storageQueue: Promise.resolve(),
    }
    try {
      const root = await fs.realpath(source)
      const manifestFile = await this.within(root, 'manifest.json')
      const stat = await fs.stat(manifestFile)
      if (!stat.isFile() || stat.size > 16384)
        throw new Error('manifest.json must be a regular file under 16 KiB')
      const manifest = validateManifest(
        JSON.parse(await fs.readFile(manifestFile, 'utf8')),
      )
      for (const file of [manifest.entry, manifest.styles].filter(
        Boolean,
      ) as string[]) {
        const location = await this.within(root, file)
        const info = await fs.stat(location)
        if (!info.isFile() || info.size > 2097152)
          throw new Error(`${file}: expected a built file under 2 MiB`)
      }
      return {
        ...base,
        root,
        manifest,
        enabled: this.enabled.has(manifest.id),
        state: 'disabled',
      }
    } catch (e) {
      return {
        ...base,
        error: `${String(e)}. Check manifest.json and build the declared entry before enabling.`,
      }
    }
  }
  async scan(autoEnable: string[] = []) {
    this.revokeAll()
    const candidates: { source: string; bundled: boolean }[] = []
    try {
      for (const dir of (
        await fs.readdir(this.bundledRoot, { withFileTypes: true })
      ).slice(0, 64))
        if (dir.isDirectory())
          candidates.push({
            source: path.join(this.bundledRoot, dir.name),
            bundled: true,
          })
    } catch (e: any) {
      if (e.code !== 'ENOENT') throw e
    }
    for (const source of this.directories.slice(0, 64))
      candidates.push({ source, bundled: false })
    const records: Record[] = []
    const ids = new Set<string>()
    for (const c of candidates) {
      const r = await this.descriptor(c.source, c.bundled)
      if (r.manifest) {
        if (ids.has(r.manifest.id)) {
          r.error = `Duplicate plugin id ${r.manifest.id}; bundled IDs cannot be replaced.`
          r.state = 'error'
          r.enabled = false
          r.manifest = undefined
        } else {
          ids.add(r.manifest.id)
          if (autoEnable.includes(c.source)) {
            this.enabled.add(r.manifest.id)
            r.enabled = true
          }
        }
      }
      records.push(r)
    }
    if (this.configurationError)
      records.push({
        root: '',
        source: this.configFile(),
        bundled: false,
        enabled: false,
        state: 'error',
        error: this.configurationError,
        logs: [],
        pending: 0,
        storageQueue: Promise.resolve(),
      })
    this.records = records
    await this.persist()
    this.reconcile()
    return this.catalog()
  }
  catalog(): PluginCatalog {
    return {
      apiVersion: 1,
      plugins: this.records.map(
        ({ root, owner, pending, storageQueue, ...r }) => structuredClone(r),
      ),
    }
  }
  attach() {
    this.renderer = true
    this.reconcile()
    return this.catalog()
  }
  detach() {
    this.renderer = false
    this.revokeAll()
    this.reconcile()
  }
  observe(reply: SessionReply<SessionStatus>) {
    if (!reply.ok || !reply.result) return
    const s = reply.result
    const game = s.game ?? null
    const key = JSON.stringify([reply.session, reply.epoch, s.system, game])
    if (
      this.current &&
      key ===
        JSON.stringify([
          this.current.identity.session,
          this.current.identity.epoch,
          this.current.system,
          this.current.game,
        ])
    )
      return
    this.revokeAll()
    this.current = {
      identity: { session: reply.session, epoch: reply.epoch },
      system: s.system,
      game,
    }
    this.reconcile()
  }
  disconnect() {
    this.revokeAll()
    this.current = undefined
    this.reconcile()
  }
  private revoke(r: Record) {
    const owner = r.owner
    r.lease = undefined
    r.owner = undefined
    if (owner)
      void this.client
        .request('execution.release', undefined, undefined, owner)
        .catch(() => {})
  }
  private revokeAll() {
    for (const r of this.records) this.revoke(r)
  }
  private reconcile() {
    for (const r of this.records) {
      if (!r.manifest || r.state === 'error') continue
      if (!r.enabled) {
        this.revoke(r)
        r.state = 'disabled'
        continue
      }
      if (r.manifest.mode && r.manifest.mode !== 'panel') {
        r.state = 'unsupported'
        r.error = `${r.manifest.mode} mode is reserved; only panels/commands are implemented.`
        continue
      }
      const match =
        this.current &&
        activation(r.manifest, this.current.system, this.current.game)
      if (!this.renderer || !this.current || !match?.matches) {
        this.revoke(r)
        r.state = 'waiting'
        continue
      }
      if (!r.lease) {
        const token = randomUUID()
        r.owner = 'plugin-' + token
        r.lease = {
          token,
          identity: { ...this.current.identity },
          system: this.current.system,
          game: this.current.game,
          profile: match.profile,
          baseURL: `ares-plugin://${r.manifest.id}/${token}/`,
        }
      }
      r.state = 'active'
      r.error = undefined
    }
    this.changed(this.catalog())
  }
  async enable(id: string, enabled: boolean) {
    return this.serial(async () => {
      const r = this.records.find((r) => r.manifest?.id === id)
      if (!r) throw new Error('Unknown plugin')
      if (typeof enabled !== 'boolean') throw new Error('Invalid enable flag')
      this.revoke(r)
      r.enabled = enabled
      if (enabled) {
        this.enabled.add(id)
        r.state = 'waiting'
      } else {
        this.enabled.delete(id)
        r.state = 'disabled'
      }
      await this.persist()
      this.reconcile()
      return this.catalog()
    })
  }
  async add(source: string) {
    return this.serial(async () => {
      if (this.directories.length >= 64)
        throw new Error('At most 64 local plugin directories')
      source = await fs.realpath(source)
      if (!this.directories.includes(source)) this.directories.push(source)
      await this.scan()
      return this.catalog()
    })
  }
  refresh() {
    return this.serial(() => this.scan())
  }
  private serial<T>(fn: () => Promise<T>) {
    const job = this.mutation.then(fn)
    this.mutation = job.then(
      () => {},
      () => {},
    )
    return job
  }
  private lease(token: string) {
    const r = this.records.find((r) => r.lease?.token === token)
    const now = this.currentIdentity()
    if (
      !r?.lease ||
      !now ||
      now.session !== r.lease.identity.session ||
      now.epoch !== r.lease.identity.epoch
    )
      throw new ServiceError('stale_plugin_scope')
    return r
  }
  async request(token: string, method: ServiceMethod, params: any) {
    const r = this.lease(token)
    let capability: Capability | undefined
    if (method === 'session.status') capability = 'session.read'
    if (['memory.regions', 'memory.read'].includes(method))
      capability = 'memory.read'
    if (method === 'memory.write') capability = 'memory.write'
    if (
      [
        'session.pause',
        'session.resume',
        'execution.runFrames',
        'execution.runUntilFrame',
        'execution.stepInstructions',
        'execution.cancel',
        'execution.status',
      ].includes(method)
    )
      capability = 'session.control'
    if (!capability || !r.manifest!.capabilities.includes(capability))
      throw new ServiceError('plugin_capability_denied')
    if (r.pending >= 8) throw new ServiceError('busy')
    ++r.pending
    try {
      const reply = await this.client.request(
        method,
        r.lease!.identity,
        params,
        r.owner,
      )
      this.lease(token)
      return reply
    } finally {
      --r.pending
    }
  }
  async resource(url: string) {
    const u = new URL(url),
      segments = u.pathname.split('/').filter(Boolean),
      token = segments.shift() ?? ''
    const r = this.lease(token)
    if (u.hostname !== r.manifest!.id || u.search || u.hash)
      throw new Error('Invalid plugin resource')
    const relative = segments.map(decodeURIComponent).join('/')
    if (
      !relative ||
      relative.includes('\\') ||
      relative.split('/').some((p) => p === '..' || p === '.' || !p)
    )
      throw new Error('Invalid plugin path')
    const file = await this.within(r.root, relative),
      stat = await fs.stat(file)
    if (!stat.isFile() || stat.size > 2097152)
      throw new Error('Plugin resource exceeds 2 MiB')
    const mime: { [key: string]: string } = {
      '.js': 'text/javascript',
      '.mjs': 'text/javascript',
      '.css': 'text/css',
      '.json': 'application/json',
      '.png': 'image/png',
      '.svg': 'image/svg+xml',
      '.webp': 'image/webp',
      '.woff2': 'font/woff2',
    }
    const type = mime[path.extname(file)]
    if (!type) throw new Error('Unsupported plugin resource type')
    const bytes = await fs.readFile(file)
    this.lease(token)
    return { bytes, type }
  }
  fail(token: string, message: string) {
    const r = this.lease(token)
    r.error = String(message).slice(0, 2048)
    this.revoke(r)
    r.state = 'error'
    this.changed(this.catalog())
  }
  log(token: string, level: string, message: string) {
    const r = this.lease(token)
    if (
      !['info', 'warn', 'error'].includes(level) ||
      typeof message !== 'string'
    )
      throw new Error('Invalid plugin log')
    r.logs.push({ level, message: message.slice(0, 2048) })
    if (r.logs.length > 128) r.logs.shift()
    this.changed(this.catalog())
  }
  async storage(
    token: string,
    operation: string,
    key: string,
    value?: unknown,
  ) {
    const r = this.lease(token)
    if (
      !/^[A-Za-z0-9._-]{1,64}$/.test(key) ||
      ['__proto__', 'constructor', 'prototype'].includes(key)
    )
      throw new Error('Invalid storage key')
    const file = path.join(
      this.dataRoot,
      `${r.manifest!.id}-v${r.manifest!.storageVersion ?? 1}.json`,
    )
    const job = r.storageQueue.then(async () => {
      this.lease(token)
      let data: { [key: string]: unknown } = {}
      try {
        const stat = await fs.lstat(file)
        if (!stat.isFile() || stat.isSymbolicLink() || stat.size > 16384)
          throw new Error('Invalid storage file')
        data = JSON.parse(await fs.readFile(file, 'utf8'))
        if (!data || typeof data !== 'object' || Array.isArray(data))
          throw new Error('Invalid stored object')
      } catch (e: any) {
        if (e.code !== 'ENOENT') throw e
      }
      if (operation === 'get') {
        this.lease(token)
        return data[key]
      }
      if (operation === 'delete') delete data[key]
      else if (operation === 'set') {
        const serialized = JSON.stringify(value)
        if (serialized === undefined) throw new Error('Storage requires JSON')
        data[key] = JSON.parse(serialized)
      } else throw new Error('Unknown storage operation')
      const bytes = JSON.stringify(data)
      if (Buffer.byteLength(bytes) > 16384)
        throw new Error('Plugin storage exceeds 16 KiB')
      const temp = file + '.' + randomUUID()
      try {
        await fs.writeFile(temp, bytes, { flag: 'wx', mode: 0o600 })
        this.lease(token)
        await fs.rename(temp, file)
      } finally {
        await fs.rm(temp, { force: true })
      }
      return undefined
    })
    r.storageQueue = job.catch(() => {})
    return job
  }
}
