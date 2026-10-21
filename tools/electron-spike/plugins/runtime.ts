import type { PluginCatalog, PluginInfo } from './contracts.js'
import type { PluginContext, PluginModule, Disposer } from './sdk.js'
const bridge = window.ares.plugins
const manager = document.createElement('details')
manager.id = 'plugins'
manager.innerHTML =
  '<summary>Plugins</summary><p>Local plugins run trusted JavaScript in this renderer.</p><button data-add>Add directory</button><button data-refresh>Refresh manifests</button><select aria-label="Plugin dock position"><option>bottom</option><option>left</option><option>right</option></select><div data-list></div><div data-commands></div>'
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
const visibilityEvents = new Map<HTMLElement, Set<(v: boolean) => void>>()
const scopes = new Map<string, { dispose: Disposer }>()
let catalog: PluginCatalog = { apiVersion: 1, plugins: [] }
const errors = new Map<string, string>()
function showError(error: unknown) {
  manager.open = true
  errors.set('host', String(error))
  render()
}
manager.querySelector<HTMLButtonElement>('[data-add]')!.onclick = () =>
  void bridge.add().then(update).catch(showError)
manager.querySelector<HTMLButtonElement>('[data-refresh]')!.onclick = () =>
  void bridge.refresh().then(update).catch(showError)
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
    label.append(check, `${p.manifest?.name ?? p.source}: ${p.state}`)
    row.title = p.source
    row.append(label)
    const detail = document.createElement('pre')
    detail.textContent = [p.error || errors.get(p.manifest?.id ?? '') || '', ...p.logs.slice(-8).map(log => `${log.level}: ${log.message}`)].filter(Boolean).join('\n')
    row.append(detail)
    list.append(row)
  }
  if (errors.has('host')) list.append(errors.get('host')!)
}
async function activate(p: PluginInfo) {
  const m = p.manifest!,
    lease = p.lease!,
    token = lease.token,
    abort = new AbortController(),
    disposers = new Set<Disposer>()
  errors.delete(m.id)
  const alive = () => {
    if (abort.signal.aborted) throw new Error('stale_plugin_scope')
  }
  const add = (fn: Disposer) => {
    alive()
    let active = true
    const dispose = () => {
      if (!active) return
      active = false
      disposers.delete(dispose)
      fn()
    }
    disposers.add(dispose)
    return dispose
  }
  const dispose = () => {
    abort.abort()
    for (const fn of [...disposers].reverse())
      try {
        fn()
      } catch (e) {
        console.error(e)
      }
    if (!tabs.childElementCount) dock.hidden = true
  }
  scopes.set(token, { dispose })
  const panels = new Map<string, { open: () => void }>(),
    commands = new Set<string>()
  const fail = (e: unknown) => {
    if (abort.signal.aborted) return
    manager.open = true
    errors.set(m.id, String(e))
    dispose()
    void bridge.fail(token, String(e)).catch(() => {})
    render()
  }
  const context: PluginContext = {
    id: m.id,
    session: lease.identity,
    system: lease.system,
    game: lease.game,
    profile: lease.profile,
    signal: abort.signal,
    services: {
      request: async (method, params) => {
        alive()
        const reply = await bridge.request(token, method, params)
        alive()
        return reply
      },
    },
    scope: {
      add,
      listen: (target, event, listener) => {
        alive()
        target.addEventListener(event, listener)
        return add(() => target.removeEventListener(event, listener))
      },
      interval: (fn, ms) => {
        alive()
        if (!Number.isFinite(ms) || ms < 50)
          throw new Error('Plugin intervals must be at least 50ms')
        const timer = setInterval(() => {
          if (!abort.signal.aborted)
            try {
              fn()
            } catch (e) {
              fail(e)
            }
        }, ms)
        return add(() => clearInterval(timer))
      },
    },
    storage: {
      get: async <T>(key: string) => {
        alive()
        const value = await bridge.storage(token, 'get', key)
        alive()
        return value as T | undefined
      },
      set: async (key, value) => {
        alive()
        await bridge.storage(token, 'set', key, value)
        alive()
      },
      delete: async (key) => {
        alive()
        await bridge.storage(token, 'delete', key)
        alive()
      },
    },
    log: Object.fromEntries(
      ['info', 'warn', 'error'].map((level) => [
        level,
        (message: string) => {
          alive()
          void bridge.log(token, level as 'info', message).catch(showError)
        },
      ]),
    ) as PluginContext['log'],
    ui: {
      registerPanel: (id, mount) => {
        alive()
        const contribution = m.contributes.panels?.find((p) => p.id === id)
        if (!contribution || panels.has(id))
          throw new Error('Undeclared or duplicate panel: ' + id)
        const tab = document.createElement('button'),
          element = document.createElement('article'),
          visibility = new Set<(v: boolean) => void>()
        element.dataset.pluginPanel = m.id + ':' + id
        element.hidden = true
        tab.textContent = contribution.title
        tabs.append(tab)
        body.append(element)
        visibilityEvents.set(element, visibility)
        let mounted = false,
          cleanup: void | Disposer
        const open = () => {
          alive()
          dock.hidden = false
          for (const other of body.children)
            if (other !== element) {
              ;(other as HTMLElement).hidden = true
              for (const event of visibilityEvents.get(other as HTMLElement) ??
                [])
                event(false)
            }
          for (const event of visibility) event(true)
          element.hidden = false
          if (!mounted) {
            mounted = true
            cleanup = mount({
              element,
              onVisibility: (callback) => {
                alive()
                visibility.add(callback)
                callback(!element.hidden)
                return add(() => visibility.delete(callback))
              },
            })
          }
        }
        tab.onclick = () => {
          try {
            if (!element.hidden) {
              element.hidden = true
              for (const event of visibility) event(false)
            } else open()
          } catch (e) {
            fail(e)
          }
        }
        panels.set(id, { open })
        return add(() => {
          try {
            cleanup?.()
          } finally {
            tab.remove()
            element.remove()
            visibility.clear()
            visibilityEvents.delete(element)
            panels.delete(id)
          }
        })
      },
      registerCommand: (id, run) => {
        alive()
        const contribution = m.contributes.commands?.find((c) => c.id === id)
        if (!contribution || commands.has(id))
          throw new Error('Undeclared or duplicate command: ' + id)
        commands.add(id)
        const button = document.createElement('button')
        button.dataset.pluginCommand = m.id + ':' + id
        button.textContent = contribution.title
        button.onclick = () => {
          try {
            alive()
            void Promise.resolve(run()).catch(fail)
          } catch (e) {
            fail(e)
          }
        }
        manager.querySelector('[data-commands]')!.append(button)
        return add(() => {
          button.remove()
          commands.delete(id)
        })
      },
      openPanel: (id) => {
        alive()
        const panel = panels.get(id)
        if (!panel) throw new Error('Unknown panel: ' + id)
        panel.open()
      },
    },
  }
  try {
    if (m.styles) {
      const link = document.createElement('link')
      link.rel = 'stylesheet'
      link.href = lease.baseURL + m.styles
      document.head.append(link)
      add(() => link.remove())
    }
    const module = (await import(lease.baseURL + m.entry)) as PluginModule
    alive()
    if (typeof module.activate !== 'function')
      throw new Error('Entry must export activate(context)')
    const cleanup = await module.activate(context)
    if (typeof cleanup === 'function') {
      if (abort.signal.aborted) cleanup()
      else add(cleanup)
    }
  } catch (e) {
    fail(e)
  }
}
function update(next: PluginCatalog) {
  catalog = next
  const active = new Set(
    next.plugins.flatMap((p) => (p.lease ? [p.lease.token] : [])),
  )
  for (const [token, scope] of scopes)
    if (!active.has(token)) {
      scope.dispose()
      scopes.delete(token)
    }
  for (const p of next.plugins)
    if (p.lease && !scopes.has(p.lease.token)) void activate(p)
  render()
}
bridge.changed(update)
void bridge.catalog().then(update).catch(showError)
