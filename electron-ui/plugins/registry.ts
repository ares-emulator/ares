import type { PluginSettingsService } from '../settings/plugins.js'
import type { PluginSettingsRequest } from '../settings/contracts.js'
import { watch, type FSWatcher } from 'node:fs'
import { buildArtifact, type PluginArtifact } from './artifacts.js'
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
  built?: PluginArtifact
  next?: PluginArtifact
  previous?: PluginArtifact
  revision?: number
  owner?: string
  pending: number
  storageQueue: Promise<unknown>
}
export class PluginRegistry {
  private settingsHost?: PluginSettingsService
  connectSettings(host: PluginSettingsService) {
    if (this.settingsHost) throw new Error('settings_host_already_connected')
    this.settingsHost = host
  }
  async settings(token: string, request: PluginSettingsRequest) {
    const r = this.lease(token)
    if (!this.settingsHost) throw new Error('settings_unavailable')
    const snapshot = await this.settingsHost.request(token, r.manifest!, r.phase === 'active', request, () => { this.authorize(token) })
    this.lease(token)
    return snapshot
  }
  private startupDisabled = false
  private records: Record[] = []
  private catalogRevision = 0
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
  private watchers = new Map<
    Record,
    {
      watcher: FSWatcher
      timer?: ReturnType<typeof setTimeout>
      busy: boolean
      dirty: boolean
    }
  >()
  private closed = false
  private building = new Set<Record>()
  private pruning?: Promise<void>
  private capturing = 0
  private pins = new Set<string>()
  private async capture(source: string) {
    while (this.pruning) await this.pruning
    this.capturing++
    try {
      const built = await buildArtifact(source, this.cache())
      this.pins.add(built.hash)
      return built
    } finally {
      this.capturing--
    }
  }
  private cache() {
    return path.join(this.dataRoot, 'artifacts')
  }
  private notify() {
    this.changed(this.catalog())
  }
  private info(r: Record) {
    return this.catalog().plugins.find((p) => p.source === r.source)!
  }
  private newLease(manifest: PluginManifest): PluginLease {
    const now = this.currentIdentity(),
      current = this.current
    if (
      !this.renderer ||
      !current ||
      !now ||
      now.session !== current.identity.session ||
      now.epoch !== current.identity.epoch
    )
      throw new ServiceError('stale_plugin_scope')
    const match = activation(manifest, current.system, current.game)
    if (!match.matches || (manifest.mode && manifest.mode !== 'panel'))
      throw new Error('Candidate no longer matches the current session/mode')
    const token = randomUUID()
    return {
      token,
      identity: { ...now },
      system: current.system,
      game: current.game,
      profile: match.profile,
      baseURL: `ares-plugin://${manifest.id}/${token}/`,
    }
  }
  private syncWatchers() {
    for (const [r, w] of this.watchers)
      if (this.closed || !r.enabled || !this.records.includes(r)) {
        clearTimeout(w.timer)
        w.watcher.close()
        this.watchers.delete(r)
      }
    if (this.closed) return
    for (const r of this.records)
      if (r.enabled && r.manifest && !this.watchers.has(r)) {
        try {
          const watcher = watch(
            r.source,
            { recursive: true },
            (_event, file) => {
              const parts = String(file ?? '').split(/[\\/]/)
              if (
                parts.some(
                  (p) =>
                    p.startsWith('.') ||
                    ['node_modules', 'artifacts'].includes(p),
                ) ||
                (r.manifest?.build &&
                  path.dirname(r.manifest.entry) !== '.' &&
                  !r.manifest.build.entry.startsWith(
                    path.dirname(r.manifest.entry) + '/',
                  ) &&
                  (parts.join('/') === path.dirname(r.manifest.entry) ||
                    parts
                      .join('/')
                      .startsWith(path.dirname(r.manifest.entry) + '/')))
              )
                return
              const w = this.watchers.get(r)
              if (!w) return
              r.revision = (r.revision ?? 0) + 1
              if (r.candidate) {
                r.candidate = undefined
                r.next = undefined
                this.notify()
              }
              w.dirty = true
              clearTimeout(w.timer)
              w.timer = setTimeout(() => void this.watchedReload(r), 180)
              w.timer.unref()
            },
          )
          watcher.unref()
          watcher.on('error', (error) => {
            r.error = `Plugin watcher: ${error.message}. Use Reload after fixing the source directory.`
            r.reload = 'error'
            this.notify()
          })
          this.watchers.set(r, { watcher, busy: false, dirty: false })
        } catch (error) {
          r.error = `Cannot watch plugin: ${String(error)}`
          r.reload = 'error'
        }
      }
  }
  private async watchedReload(r: Record) {
    const w = this.watchers.get(r)
    if (!w || w.busy || !w.dirty) return
    w.busy = true
    w.dirty = false
    try {
      await this.reloadRecord(r, false)
    } finally {
      w.busy = false
      if (w.dirty && this.watchers.has(r)) {
        w.timer = setTimeout(() => void this.watchedReload(r), 180)
        w.timer.unref()
      }
    }
  }
  async close() {
    this.closed = true
    this.syncWatchers()
    this.detach()
  }
  private prune(): Promise<void> {
    if (this.pruning) return this.pruning
    if (this.building.size || this.capturing) return Promise.resolve()
    const job = this.pruneArtifacts()
    this.pruning = job
    void job.then(
      () => {
        this.pruning = undefined
      },
      () => {
        this.pruning = undefined
      },
    )
    return job
  }
  private async pruneArtifacts() {
    if (this.building.size || this.capturing) return
    const keep = new Set(
      this.records
        .flatMap((r) => [r.built?.hash, r.next?.hash, r.previous?.hash])
        .filter(Boolean),
    )
    for (const hash of this.pins) keep.add(hash)
    for (const entry of await fs.readdir(this.cache()).catch(() => []))
      if (/^[a-f0-9]{64}$/.test(entry) && !keep.has(entry))
        await fs.rm(path.join(this.cache(), entry), {
          recursive: true,
          force: true,
        })
  }
  private async reloadRecord(r: Record, force: boolean) {
    if (!r.enabled || !r.manifest || this.closed) return
    if (r.phase === 'activating') {
      const w = this.watchers.get(r)
      if (w) w.dirty = true
      return
    }
    if (this.building.has(r)) {
      const w = this.watchers.get(r)
      if (w) w.dirty = true
      return
    }
    this.building.add(r)
    let captured: PluginArtifact | undefined
    const revision = r.revision ?? 0
    r.reload = 'building'
    this.notify()
    try {
      const built = (captured = await this.capture(r.source))
      if (
        this.closed ||
        !r.enabled ||
        !this.records.includes(r) ||
        revision !== (r.revision ?? 0)
      )
        return
      if (built.manifest.id !== r.manifest.id)
        throw new Error(
          'Plugin id cannot change during reload; add it as a new directory',
        )
      if (!force && built.hash === r.built?.hash) {
        r.reload = undefined
        return
      }
      if ((r.phase as string) === 'activating') {
        const w = this.watchers.get(r)
        if (w) w.dirty = true
        return
      }
      if (!r.lease) {
        r.built = built
        r.root = built.root
        r.artifact = built.hash
        r.manifest = built.manifest
        r.state = 'waiting'
        r.error = undefined
        r.reload = undefined
        this.reconcile()
        return
      }
      const lease = this.newLease(built.manifest)
      r.next = built
      r.candidate = { manifest: built.manifest, lease, artifact: built.hash }
      r.reload = 'staging'
      r.error = undefined
    } catch (error) {
      r.error = `Build/reload failed: ${String(error)}`.slice(0, 2048)
      r.reload = 'error'
    } finally {
      if (captured) this.pins.delete(captured.hash)
      this.building.delete(r)
      void this.prune().catch(() => {})
      const w = this.watchers.get(r)
      if (w?.dirty && !w.busy) {
        clearTimeout(w.timer)
        w.timer = setTimeout(() => void this.watchedReload(r), 180)
        w.timer.unref()
      }
      this.notify()
    }
  }
  async reload(id: string) {
    if (this.startupDisabled) throw new Error('plugins_disabled_until_restart')
    const r = this.records.find((r) => r.manifest?.id === id)
    if (!r) throw new Error('Unknown plugin')
    r.revision = (r.revision ?? 0) + 1
    await this.reloadRecord(r, true)
    return this.catalog()
  }
  private candidate(token: string) {
    const r = this.records.find((r) => r.candidate?.lease.token === token)
    if (!r?.candidate || !r.enabled)
      throw new ServiceError('stale_plugin_scope')
    const now = this.currentIdentity()
    if (
      !now ||
      now.session !== r.candidate.lease.identity.session ||
      now.epoch !== r.candidate.lease.identity.epoch
    )
      throw new ServiceError('stale_plugin_scope')
    return r
  }
  reject(token: string, message: string) {
    const r = this.candidate(token)
    r.candidate = undefined
    r.next = undefined
    r.error = `Candidate rejected: ${String(message)}`.slice(0, 2048)
    r.reload = 'error'
    this.notify()
    void this.prune().catch(() => {})
  }
  begin(token: string) {
    return this.serial(async () => {
      const r = this.candidate(token),
        candidate = r.candidate!,
        built = r.next!
      this.newLease(candidate.manifest)
      r.previous = r.built
      r.candidate = undefined
      r.next = undefined
      const release = this.revoke(r, true)
      r.phase = 'activating'
      try {
        await release
      } catch (error) {
        if (!r.lease) {
          r.state = 'error'
          r.error = `Owner release failed; replacement blocked: ${String(error)}`
          this.notify()
        }
        throw error
      }
      if (!r.enabled || !this.renderer)
        throw new ServiceError('stale_plugin_scope')
      const now = this.currentIdentity()
      if (
        !now ||
        now.session !== candidate.lease.identity.session ||
        now.epoch !== candidate.lease.identity.epoch
      )
        throw new ServiceError('stale_plugin_scope')
      r.built = built
      r.root = built.root
      r.artifact = built.hash
      r.manifest = built.manifest
      r.lease = candidate.lease
      r.owner = 'plugin-' + token
      r.phase = 'activating'
      r.reload = 'activating'
      return this.info(r)
    })
  }
  finish(token: string) {
    const r = this.lease(token)
    this.settingsHost?.finish(token)
    r.phase = 'active'
    r.candidate = undefined
    r.next = undefined
    r.reload = r.error ? 'error' : undefined
    const w = this.watchers.get(r)
    if (w?.dirty) {
      clearTimeout(w.timer)
      w.timer = setTimeout(() => void this.watchedReload(r), 180)
      w.timer.unref()
    }
    this.notify()
    void this.prune().catch(() => {})
  }
  rollback(token: string, message: string) {
    return this.serial(async () => {
      const r = this.lease(token),
        previous = r.previous
      if (!previous) throw new Error('No previous artifact for rollback')
      const release = this.revoke(r, true)
      r.phase = 'activating'
      await release
      r.built = previous
      r.previous = undefined
      r.root = previous.root
      r.artifact = previous.hash
      r.manifest = previous.manifest
      r.lease = this.newLease(previous.manifest)
      r.owner = 'plugin-' + r.lease.token
      r.phase = 'activating'
      r.error =
        `Activation failed; previous artifact restored: ${String(message)}`.slice(
          0,
          2048,
        )
      r.reload = 'activating'
      return this.info(r)
    })
  }

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
  async initialize(
    extra: string[] = [],
    defaults: string[] = [],
    startupDisabled = false,
  ) {
    this.startupDisabled = startupDisabled
    await fs.mkdir(this.dataRoot, { recursive: true })
    this.dataRoot = await fs.realpath(this.dataRoot)
    try {
      const stat = await fs.stat(this.configFile())
      if (!stat.isFile() || stat.size > 32768)
        throw new Error('plugin configuration must be a regular file under 32 KiB')
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
      else if (!startupDisabled)
        for (const id of defaults) this.enabled.add(id)
    }
    if (!startupDisabled)
      for (const dir of extra)
        if (!this.directories.includes(dir)) this.directories.push(dir)
    await this.scan(startupDisabled ? [] : extra)
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
      if (this.startupDisabled)
        return {
          ...base,
          root,
          manifest,
          enabled: false,
          state: 'disabled',
        }
      const built = await this.capture(root)
      return {
        ...base,
        root: built.root,
        built,
        artifact: built.hash,
        manifest: built.manifest,
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
    const oldRecords = this.records
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
      let r: Record
      const existing = oldRecords.find((r) => r.source === c.source)
      if (existing?.manifest) {
        r = existing
        if (r.enabled) await this.reloadRecord(r, true)
      } else r = await this.descriptor(c.source, c.bundled)
      if (r.manifest) {
        if (ids.has(r.manifest.id)) {
          r.error = `Duplicate plugin id ${r.manifest.id}; bundled IDs cannot be replaced.`
          r.state = 'error'
          r.enabled = false
          r.manifest = undefined
        } else {
          ids.add(r.manifest.id)
          if (!this.startupDisabled && autoEnable.includes(c.source)) {
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
    for (const old of oldRecords)
      if (!records.includes(old)) void this.revoke(old)
    this.records = records
    for (const r of records) if (r.built) this.pins.delete(r.built.hash)
    this.syncWatchers()
    if (!this.startupDisabled) await this.persist()
    this.reconcile()
    return this.catalog()
  }
  catalog(): PluginCatalog {
    return {
      apiVersion: 1,
      startupDisabled: this.startupDisabled,
      revision: ++this.catalogRevision,
      plugins: this.records.map(
        ({
          root,
          owner,
          pending,
          storageQueue,
          built,
          next,
          previous,
          revision,
          ...r
        }) => structuredClone({
          ...r,
          ...(this.startupDisabled ? {
            configuredEnabled: this.enabled.has(r.manifest?.id ?? ''),
          } : {}),
        }),
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
  private revoke(r: Record, strict = false) {
    const owner = r.owner
    if (r.lease) this.settingsHost?.revoke(r.lease.token)
    r.lease = undefined
    r.owner = undefined
    r.candidate = undefined
    r.next = undefined
    r.phase = undefined
    if (owner)
      return this.client
        .request('execution.release', undefined, undefined, owner)
        .then(
          (reply) => {
            if (strict && !reply.ok)
              throw new Error(reply.error || 'Plugin owner release failed')
          },
          (error) => {
            if (strict) throw error
          },
        )
    return Promise.resolve()
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
      // A replacement has revoked its old lease and is awaiting native owner
      // release. Status polling must not create an intermediate writer here.
      if (!r.lease && r.phase === 'activating') continue
      if (!r.lease) {
        const token = randomUUID()
        r.phase = 'activating'
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
    }
    this.syncWatchers()
    this.changed(this.catalog())
  }
  async enable(id: string, enabled: boolean) {
    return this.serial(async () => {
      if (this.startupDisabled && enabled)
        throw new Error('plugins_disabled_until_restart')
      const r = this.records.find((r) => r.manifest?.id === id)
      // Even a broken/missing manifest can be disabled by its persisted ID.
      if (
        !r && this.startupDisabled && enabled === false && this.enabled.has(id)
      ) {
        this.enabled.delete(id)
        await this.persist()
        return this.catalog()
      }
      if (!r) throw new Error('Unknown plugin')
      if (typeof enabled !== 'boolean') throw new Error('Invalid enable flag')
      this.revoke(r)
      r.revision = (r.revision ?? 0) + 1
      r.enabled = enabled
      if (enabled) {
        this.enabled.add(id)
        r.state = 'waiting'
        try {
          const built = await this.capture(r.source)
          this.pins.delete(built.hash)
          if (built.manifest.id !== id)
            throw new Error('Plugin id changed; add it as a new plugin')
          r.built = built
          r.root = built.root
          r.artifact = built.hash
          r.manifest = built.manifest
          r.error = undefined
          r.reload = undefined
        } catch (error) {
          r.state = 'error'
          r.error = `Enable/build failed: ${String(error)}`.slice(0, 2048)
        }
      } else {
        this.enabled.delete(id)
        r.state = 'disabled'
      }
      await this.persist()
      this.syncWatchers()
      this.reconcile()
      return this.catalog()
    })
  }
  async resetStartup() {
    if (!this.startupDisabled)
      throw new Error('restart_with_disable_plugins_to_reset')
    return this.serial(async () => {
      this.revokeAll()
      this.enabled.clear()
      this.directories = []
      this.records = []
      this.configurationError = undefined
      await this.scan()
      await this.persist()
      return this.catalog()
    })
  }
  async add(source: string) {
    if (this.startupDisabled) throw new Error('plugins_disabled_until_restart')
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
  authorize(token: string) {
    const r = this.lease(token)
    if (r.phase !== 'active') throw new ServiceError('plugin_not_ready')
    return this.info(r)
  }
  async request(token: string, method: ServiceMethod, params: any) {
    const r = this.lease(token)
    let capability: Capability | undefined
    if (['n64.debug.lastStop', 'n64.debug.registers'].includes(method))
      capability = 'debug.read'
    if (method === 'session.status') capability = 'session.read'
    if (['memory.regions', 'memory.read', 'n64.memory.read'].includes(method))
      capability = 'memory.read'
    if (
      ['memory.write', 'n64.memory.write', 'n64.memory.writeMany'].includes(
        method,
      )
    )
      capability = 'memory.write'
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
    if (
      r.phase !== 'active' &&
      ![
        'session.status',
        'memory.regions',
        'memory.read',
        'n64.memory.read',
        'n64.debug.lastStop',
        'n64.debug.registers',
        'execution.status',
      ].includes(method)
    )
      throw new ServiceError('plugin_not_ready')
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
    const candidate = this.records.find(
      (r) => r.candidate?.lease.token === token,
    )
    const r = candidate ? this.candidate(token) : this.lease(token)
    const resourceRoot = candidate ? r.next!.root : r.root
    const manifest = candidate ? r.candidate!.manifest : r.manifest!
    if (u.hostname !== manifest.id || u.search || u.hash)
      throw new Error('Invalid plugin resource')
    const relative = segments.map(decodeURIComponent).join('/')
    if (
      !relative ||
      relative.includes('\\') ||
      relative.split('/').some((p) => p === '..' || p === '.' || !p)
    )
      throw new Error('Invalid plugin path')
    const file = await this.within(resourceRoot, relative),
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
    if (candidate) this.candidate(token)
    else this.lease(token)
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
    if (r.phase !== 'active' && operation !== 'get')
      throw new ServiceError('plugin_not_ready')
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
