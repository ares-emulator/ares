import type { PluginCatalog, PluginInfo, PluginCandidate } from './contracts.js'
import type { PluginModule } from './sdk.js'
import { PluginScope, bounded, jsonState, type Dock } from './scope.js'
const bridge = window.ares.plugins
const manager = document.createElement('details')
manager.id = 'plugins'
manager.innerHTML =
  '<summary>Plugins</summary><p>Local plugins run trusted JavaScript in this renderer.</p><button data-add>Add directory</button><button data-refresh>Refresh manifests</button><select aria-label="Plugin dock position"><option>bottom</option><option>left</option><option>right</option></select><div data-list></div><div data-commands></div><output data-health></output>'
document.querySelector('aside')!.append(manager)
const dock = document.createElement('section')
dock.id = 'plugin-dock'
dock.hidden = true
const tabs = document.createElement('nav'),
  body = document.createElement('div')
dock.append(tabs, body)
const position = manager.querySelector('select')!
position.value = localStorage.getItem('plugin-dock-position') || 'bottom'
function place() {
  dock.dataset.position = position.value
  localStorage.setItem('plugin-dock-position', position.value)
  if (position.value === 'bottom')
    document.querySelector('footer')!.before(dock)
  else document.querySelector('main')!.append(dock)
}
position.onchange = place
place()
const scopes = new Map<string, PluginScope>(),
  jobs = new Map<string, Promise<void>>(),
  visibility = new Map<HTMLElement, (v: boolean) => void>()
let catalog: PluginCatalog = { apiVersion: 1, plugins: [] }
const errors = new Map<string, string>()
function showError(error: unknown) {
  manager.open = true
  errors.set('host', String(error))
  render()
}
function health() {
  const active = [...scopes.values()].filter((s) => !s.abort.signal.aborted)
  const counts = {
    scopes: active.length,
    listeners: active.reduce((n, s) => n + s.listeners, 0),
    timers: active.reduce((n, s) => n + s.timers, 0),
    pending: active.reduce((n, s) => n + s.pending, 0),
    jobs: jobs.size,
  }
  for (const [key, value] of Object.entries(counts))
    manager.dataset[key] = String(value)
  manager.querySelector('[data-health]')!.textContent =
    `${counts.scopes} scopes · ${counts.listeners} listeners · ${counts.timers} timers · ${counts.pending} requests`
  dock.hidden = !tabs.childElementCount
}
function render() {
  const list = manager.querySelector('[data-list]')!
  list.replaceChildren()
  for (const p of catalog.plugins) {
    const row = document.createElement('div'),
      label = document.createElement('label'),
      check = document.createElement('input')
    check.type = 'checkbox'
    check.checked = p.enabled
    check.disabled = !p.manifest
    check.onchange = () =>
      void bridge
        .enable(p.manifest!.id, check.checked)
        .then(update)
        .catch(showError)
    label.append(
      check,
      `${p.manifest?.name ?? p.source}: ${p.reload ?? p.state}`,
    )
    row.title = p.source
    row.append(label)
    if (p.manifest) {
      const reload = document.createElement('button')
      reload.textContent = 'Reload'
      reload.dataset.pluginReload = p.manifest.id
      reload.disabled =
        !p.enabled || p.reload === 'building' || p.reload === 'activating'
      reload.onclick = () =>
        void bridge.reload(p.manifest!.id).then(update).catch(showError)
      row.append(reload)
    }
    const detail = document.createElement('pre')
    detail.textContent = [
      p.error || errors.get(p.manifest?.id ?? '') || '',
      ...p.logs.slice(-8).map((log) => `${log.level}: ${log.message}`),
    ]
      .filter(Boolean)
      .join('\n')
    row.append(detail)
    list.append(row)
  }
  if (errors.has('host')) list.append(errors.get('host')!)
  health()
}
manager.querySelector<HTMLButtonElement>('[data-add]')!.onclick = () =>
  void bridge.add().then(update).catch(showError)
manager.querySelector<HTMLButtonElement>('[data-refresh]')!.onclick = () =>
  void bridge.refresh().then(update).catch(showError)
function host(id: string): Dock {
  return {
    tabs,
    body,
    commands: manager.querySelector('[data-commands]')!,
    visibility,
    changed: health,
    error: (error) => {
      const scope = scopes.get(id)
      if (!scope || scope.abort.signal.aborted) return
      errors.set(id, String(error))
      manager.open = true
      void scope.dispose().catch(showError)
      void bridge.fail(scope.token, String(error)).catch(() => {})
      render()
    },
  }
}
async function load(info: PluginInfo): Promise<PluginModule> {
  const module = await bounded(
    import(info.lease!.baseURL + info.manifest!.entry) as Promise<PluginModule>,
    'Plugin import',
  )
  if (typeof module.activate !== 'function')
    throw new Error('Entry must export activate(context)')
  if (
    module.stateVersion !== undefined &&
    (!Number.isSafeInteger(module.stateVersion) ||
      module.stateVersion < 1 ||
      module.stateVersion > 10000)
  )
    throw new Error('stateVersion must be an integer from 1 to 10000')
  if (module.prepare)
    await bounded(Promise.resolve(module.prepare()), 'Candidate preparation')
  return module
}
function isCurrent(info: PluginInfo) {
  return catalog.plugins.some(
    (p) =>
      p.manifest?.id === info.manifest!.id &&
      p.enabled &&
      (p.lease?.token === info.lease!.token ||
        p.candidate?.lease.token === info.lease!.token),
  )
}
async function mount(
  info: PluginInfo,
  module: PluginModule,
  state: unknown,
  visible: string[],
) {
  const id = info.manifest!.id,
    scope = new PluginScope(info, module, host(id))
  scopes.set(id, scope)
  health()
  try {
    await scope.activate(state)
    if (scope.abort.signal.aborted) throw new Error('stale_plugin_scope')
    scope.restore(visible)
    await bridge.finish(scope.token)
    if (scope.abort.signal.aborted) throw new Error('stale_plugin_scope')
    scope.publish()
    errors.delete(id)
  } catch (error) {
    await scope.dispose().catch((cleanup) => {
      errors.set(id, `Cleanup failed: ${String(cleanup)}`)
    })
    throw error
  }
}
async function initial(info: PluginInfo) {
  try {
    const module = await load(info)
    if (!isCurrent(info)) return
    await mount(info, module, undefined, [])
  } catch (error) {
    if (isCurrent(info)) {
      errors.set(info.manifest!.id, String(error))
      manager.open = true
      await bridge.fail(info.lease!.token, String(error)).catch(() => {})
    }
  }
}
async function replace(p: PluginInfo, candidate: PluginCandidate) {
  const id = p.manifest!.id,
    old = scopes.get(id),
    staged: PluginInfo = {
      ...p,
      manifest: candidate.manifest,
      lease: candidate.lease,
      artifact: candidate.artifact,
    },
    visible = old?.visible() ?? []
  let state: unknown,
    version: number | undefined,
    begun = false,
    newInfo: PluginInfo | undefined
  try {
    const module = await load(staged)
    if (old?.module.saveState) {
      state = jsonState(
        await bounded(
          Promise.resolve(old.module.saveState()),
          'UI state serialization',
        ),
      )
      version = old.module.stateVersion
    }
    newInfo = await bridge.begin(candidate.lease.token)
    begun = true
    // Main has revoked the old writer before any candidate activation/restore.
    if (old) await old.dispose()
    if (
      !isCurrent(staged) &&
      catalog.plugins.some((p) => p.manifest?.id === id && !p.enabled)
    )
      throw new Error('stale_plugin_scope')
    await mount(
      newInfo,
      module,
      module.stateVersion !== undefined && module.stateVersion === version
        ? state
        : undefined,
      visible,
    )
  } catch (error) {
    if (!begun) {
      await bridge.reject(candidate.lease.token, String(error)).catch(() => {})
      return
    }
    const failed = scopes.get(id)
    await failed?.dispose().catch(() => {})
    try {
      const restored = await bridge.rollback(
        newInfo!.lease!.token,
        String(error),
      )
      const module = await load(restored)
      await mount(
        restored,
        module,
        module.stateVersion !== undefined && module.stateVersion === version
          ? state
          : undefined,
        visible,
      )
    } catch (rollbackError) {
      const message = `Replacement failed: ${String(error)}; rollback failed: ${String(rollbackError)}`
      errors.set(id, message)
      manager.open = true
      const current = scopes.get(id)
      if (current) await current.dispose().catch(() => {})
      const token = current?.token ?? newInfo?.lease?.token
      if (token) await bridge.fail(token, message).catch(() => {})
    }
  }
}
function start(id: string, work: () => Promise<void>) {
  const job = work()
    .catch(showError)
    .finally(() => {
      jobs.delete(id)
      update(catalog)
    })
  jobs.set(id, job)
  health()
}
function update(next: PluginCatalog) {
  if ((next.revision ?? 0) < (catalog.revision ?? 0)) return
  catalog = next
  for (const [id, scope] of scopes) {
    const p = next.plugins.find((p) => p.manifest?.id === id)
    if (
      !p?.enabled ||
      !p.lease ||
      (!jobs.has(id) && p.lease.token !== scope.token)
    ) {
      // Session/disable invalidation is synchronous even during a replacement job.
      if (!p?.enabled || !p.lease || !jobs.has(id)) {
        void scope.dispose().catch(showError)
        scopes.delete(id)
      }
    }
  }
  for (const p of next.plugins) {
    const id = p.manifest?.id
    if (!id || jobs.has(id)) continue
    if (p.candidate) start(id, () => replace(p, p.candidate!))
    else if (p.lease && !scopes.has(id)) start(id, () => initial(p))
  }
  render()
}
bridge.changed(update)
void bridge.catalog().then(update).catch(showError)

bridge.actions((action) => {
  const scope = [...scopes.values()].find(
    (scope) => scope.token === action.token,
  )
  void (async () => {
    try {
      if (!scope) throw new Error('stale_plugin_scope')
      await scope.action(action.kind, action.name)
      await bridge.result(action.id, action.token)
    } catch (error) {
      await bridge
        .result(action.id, action.token, String(error))
        .catch(() => {})
    }
  })()
})
