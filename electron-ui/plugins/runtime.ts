import { workspace, showSettings, notify } from '../workspace/shell.js'
import { panelKey } from '../workspace/model.js'
import type { PluginCatalog, PluginInfo, PluginCandidate } from './contracts.js'
import type { PluginModule } from './sdk.js'
import { PluginScope, bounded, jsonState, type Dock } from './scope.js'
const bridge = window.ares.plugins
const manager = document.createElement('div')
manager.id = 'plugins'
manager.innerHTML = '<p class="help">Choose which plugins are available in your workspace.</p><p data-recovery hidden>Plugins are disabled for this launch. Disable a broken plugin here, then restart without --disable-plugins.</p><div class="plugin-actions"><button data-add>Add plugin directory…</button><button data-refresh>Refresh</button></div><div data-list></div><div id="plugin-commands" data-commands></div>'
document.getElementById('plugin-management')!.append(manager)
const scopes = new Map<string, PluginScope>(),
  jobs = new Map<string, Promise<void>>()
let catalog: PluginCatalog = { apiVersion: 1, plugins: [] }
const errors = new Map<string, string>()
function showError(error: unknown) {
  showSettings('plugins')
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

}
function render() {
  const recovery = !!catalog.startupDisabled
  manager.querySelector<HTMLElement>('[data-recovery]')!.hidden = !recovery
  manager.querySelector<HTMLButtonElement>('[data-add]')!.disabled = recovery
  if (recovery) showSettings('plugins')
  const list = manager.querySelector('[data-list]')!
  list.replaceChildren()
  for (const p of catalog.plugins) {
    const row = document.createElement('div'),
      label = document.createElement('label'),
      check = document.createElement('input')
    check.type = 'checkbox'
    check.checked = recovery ? !!p.configuredEnabled : p.enabled
    check.disabled = !p.manifest || (recovery && !p.configuredEnabled)
    check.onchange = () =>
      void bridge
        .enable(p.manifest!.id, check.checked)
        .then(update)
        .catch(showError)
    label.append(
      check,
      `${p.manifest?.name ?? p.source}: ${p.reload ?? p.state}`,
    )
    row.className = 'plugin-card'
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
    commands: manager.querySelector('[data-commands]')!,
    registerPanel: (local, title, mount) => workspace.registerPanel(panelKey(id, local), title, context => {
      try { return mount(context) } catch (error) {
        const scope = scopes.get(id)
        if (scope && !scope.abort.signal.aborted) {
          errors.set(id, String(error)); void scope.dispose().catch(showError)
          void bridge.fail(scope.token, String(error)).catch(() => {})
        }
        throw error
      }
    }),
    openPanel: (local, automatic) => workspace.openPanel(panelKey(id, local), automatic),
    changed: health,
    error: error => {
      const scope = scopes.get(id)
      if (!scope || scope.abort.signal.aborted) return
      errors.set(id, String(error)); notify(error)
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
) {
  const id = info.manifest!.id,
    scope = new PluginScope(info, module, host(id))
  scopes.set(id, scope)
  health()
  try {
    await scope.activate(state)
    if (scope.abort.signal.aborted) throw new Error('stale_plugin_scope')
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
    await mount(info, module, undefined)
  } catch (error) {
    if (isCurrent(info)) {
      errors.set(info.manifest!.id, String(error))
      showSettings('plugins')
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
    }
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
        )
    } catch (rollbackError) {
      const message = `Replacement failed: ${String(error)}; rollback failed: ${String(rollbackError)}`
      errors.set(id, message)
      showSettings('plugins')
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
  workspace.setKnown(next.plugins.flatMap(plugin => plugin.manifest?.contributes.panels?.map(panel => ({ key: panelKey(plugin.manifest!.id, panel.id), title: `${plugin.manifest!.name} / ${panel.title}` })) ?? []))
  for (const [id, scope] of scopes) {
    const p = next.plugins.find((p) => p.manifest?.id === id)
    if (
      !p?.enabled ||
      !p.lease ||
      (!jobs.has(id) && p.lease.token !== scope.token && p.candidate?.lease.token !== scope.token)
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
    // finish() may resolve before its catalog event arrives. A completed
    // replacement still matches the candidate in the older catalog; do not
    // dispose it or run activate() a second time while waiting for that event.
    if (p.candidate && scopes.get(id)?.token !== p.candidate.lease.token) start(id, () => replace(p, p.candidate!))
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
