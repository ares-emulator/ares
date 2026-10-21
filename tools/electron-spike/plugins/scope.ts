import type { PluginInfo } from './contracts.js'
import type { Disposer, PluginContext, PluginModule } from './sdk.js'
export async function bounded<T>(
  promise: Promise<T>,
  label: string,
  milliseconds = 2000,
): Promise<T> {
  let timer: ReturnType<typeof setTimeout> | undefined
  try {
    return await Promise.race([
      promise,
      new Promise<never>((_, reject) => {
        timer = setTimeout(
          () => reject(new Error(`${label} timed out (${milliseconds}ms)`)),
          milliseconds,
        )
      }),
    ])
  } finally {
    clearTimeout(timer)
  }
}
export function jsonState(value: unknown): unknown {
  if (value === undefined) return undefined
  const bytes = JSON.stringify(value)
  if (bytes === undefined || new TextEncoder().encode(bytes).length > 16384)
    throw new Error('Transient UI state must be JSON under 16 KiB')
  return JSON.parse(bytes)
}
export interface Dock {
  tabs: HTMLElement
  body: HTMLElement
  commands: HTMLElement
  visibility: Map<HTMLElement, (visible: boolean) => void>
  changed(): void
  error(error: unknown): void
}
/** Owns every SDK effect; registrations and timers remain inert until publish. */
export class PluginScope {
  readonly abort = new AbortController()
  readonly token: string
  private disposers = new Set<Disposer>()
  private arms = new Set<() => void>()
  private tabFragment = document.createDocumentFragment()
  private panelFragment = document.createDocumentFragment()
  private commandFragment = document.createDocumentFragment()
  private panels = new Map<
    string,
    {
      element: HTMLElement
      open: () => void
      setVisible: (value: boolean) => void
    }
  >()
  private commands = new Map<string, () => void | Promise<void>>()
  private links: HTMLLinkElement[] = []
  private ready = false
  private published = false
  private disposal?: Promise<void>
  listeners = 0
  timers = 0
  pending = 0
  readonly context: PluginContext
  constructor(
    readonly info: PluginInfo,
    readonly module: PluginModule,
    private dock: Dock,
  ) {
    const m = info.manifest!,
      lease = info.lease!
    this.token = lease.token
    const alive = () => {
      if (this.abort.signal.aborted) throw new Error('stale_plugin_scope')
    }
    const writable = () => {
      alive()
      if (!this.ready)
        throw new Error(
          'plugin_not_ready: use a command for writes; activation/reload never replays mutations',
        )
    }
    const add = (dispose: Disposer): Disposer => {
      alive()
      let active = true
      const once = () => {
        if (!active) return
        active = false
        this.disposers.delete(once)
        return dispose()
      }
      this.disposers.add(once)
      return once
    }
    const effect = (start: () => Disposer) => {
      alive()
      let cancel: Disposer | undefined,
        active = true
      const arm = () => {
        if (active && !cancel && !this.abort.signal.aborted) cancel = start()
      }
      this.arms.add(arm)
      if (this.published) arm()
      return add(() => {
        active = false
        this.arms.delete(arm)
        return cancel?.()
      })
    }
    const fail = (error: unknown) => this.dock.error(error)
    this.context = {
      id: m.id,
      session: lease.identity,
      system: lease.system,
      game: lease.game,
      profile: lease.profile,
      signal: this.abort.signal,
      services: {
        request: async (method, params) => {
          alive()
          if (
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
            writable()
          if (this.pending >= 8) throw new Error('busy')
          ++this.pending
          this.dock.changed()
          try {
            const reply = await window.ares.plugins.request(
              this.token,
              method,
              params,
            )
            alive()
            return reply
          } finally {
            --this.pending
            this.dock.changed()
          }
        },
      },
      scope: {
        add,
        listen: (target, event, listener) =>
          effect(() => {
            const guarded: EventListener = (e) => {
              if (!this.abort.signal.aborted)
                try {
                  void Promise.resolve(listener(e)).catch((error) => {
                    if (!this.abort.signal.aborted) fail(error)
                  })
                } catch (error) {
                  fail(error)
                }
            }
            target.addEventListener(event, guarded)
            ++this.listeners
            this.dock.changed()
            return () => {
              target.removeEventListener(event, guarded)
              --this.listeners
              this.dock.changed()
            }
          }),
        interval: (callback, milliseconds) => {
          if (!Number.isFinite(milliseconds) || milliseconds < 50)
            throw new Error('Plugin intervals must be at least 50ms')
          if (this.arms.size >= 128)
            throw new Error('At most 128 scoped effects')
          return effect(() => {
            const timer = setInterval(() => {
              if (
                !this.abort.signal.aborted &&
                [...this.panels.values()].some((p) => !p.element.hidden)
              )
                try {
                  void Promise.resolve(callback()).catch((error) => {
                    if (!this.abort.signal.aborted) fail(error)
                  })
                } catch (error) {
                  fail(error)
                }
            }, milliseconds)
            ++this.timers
            this.dock.changed()
            return () => {
              clearInterval(timer)
              --this.timers
              this.dock.changed()
            }
          })
        },
      },
      storage: {
        get: async <T>(key: string) => {
          alive()
          const value = await window.ares.plugins.storage(
            this.token,
            'get',
            key,
          )
          alive()
          return value as T | undefined
        },
        set: async (key, value) => {
          writable()
          await window.ares.plugins.storage(this.token, 'set', key, value)
          alive()
        },
        delete: async (key) => {
          writable()
          await window.ares.plugins.storage(this.token, 'delete', key)
          alive()
        },
      },
      log: {
        info: (message) => {
          alive()
          void window.ares.plugins
            .log(this.token, 'info', message)
            .catch(() => {})
        },
        warn: (message) => {
          alive()
          void window.ares.plugins
            .log(this.token, 'warn', message)
            .catch(() => {})
        },
        error: (message) => {
          alive()
          void window.ares.plugins
            .log(this.token, 'error', message)
            .catch(() => {})
        },
      },
      ui: {
        registerPanel: (id, mount) => {
          alive()
          const contribution = m.contributes.panels?.find((p) => p.id === id)
          if (!contribution || this.panels.has(id))
            throw new Error('Undeclared or duplicate panel: ' + id)
          const tab = document.createElement('button'),
            element = document.createElement('article'),
            visibility = new Set<(value: boolean) => void>()
          tab.textContent = contribution.title
          element.hidden = true
          element.dataset.pluginPanel = m.id + ':' + id
          if (this.published) {
            this.dock.tabs.append(tab)
            this.dock.body.append(element)
          } else {
            this.tabFragment.append(tab)
            this.panelFragment.append(element)
          }
          let mounted = false,
            cleanup: void | Disposer
          const setVisible = (value: boolean) => {
            if (element.hidden === !value) return
            element.hidden = !value
            for (const callback of visibility) callback(value)
          }
          const open = () => {
            alive()
            for (const [other, notify] of this.dock.visibility)
              if (other !== element) notify(false)
            setVisible(true)
            if (!mounted) {
              mounted = true
              cleanup = mount({
                element,
                onVisibility: (callback) => {
                  alive()
                  visibility.add(callback)
                  callback(!element.hidden)
                  return add(() => {
                    visibility.delete(callback)
                  })
                },
              })
            }
            this.dock.changed()
          }
          tab.onclick = () => {
            try {
              writable()
              if (!element.hidden) setVisible(false)
              else open()
            } catch (error) {
              fail(error)
            }
          }
          this.panels.set(id, { element, open, setVisible })
          this.dock.visibility.set(element, setVisible)
          return add(() => {
            tab.remove()
            element.remove()
            this.dock.visibility.delete(element)
            visibility.clear()
            this.panels.delete(id)
            return cleanup?.()
          })
        },
        registerCommand: (id, run) => {
          alive()
          const contribution = m.contributes.commands?.find((c) => c.id === id)
          if (!contribution || this.commands.has(id))
            throw new Error('Undeclared or duplicate command: ' + id)
          this.commands.set(id, run)
          const button = document.createElement('button')
          button.textContent = contribution.title
          button.dataset.pluginCommand = m.id + ':' + id
          button.onclick = () => {
            try {
              writable()
              void Promise.resolve(run()).catch(fail)
            } catch (error) {
              fail(error)
            }
          }
          if (this.published) this.dock.commands.append(button)
          else this.commandFragment.append(button)
          return add(() => {
            button.remove()
            this.commands.delete(id)
          })
        },
        openPanel: (id) => {
          alive()
          const panel = this.panels.get(id)
          if (!panel) throw new Error('Unknown panel: ' + id)
          panel.open()
        },
      },
    }
  }
  async action(kind: 'open' | 'command', name: string) {
    if (!this.ready || this.abort.signal.aborted)
      throw new Error('stale_plugin_scope')
    if (kind === 'open') {
      const panel = this.panels.get(name)
      if (!panel) throw new Error('Unknown panel')
      panel.open()
    } else {
      const run = this.commands.get(name)
      if (!run) throw new Error('Unknown command')
      await run()
    }
    if (this.abort.signal.aborted) throw new Error('stale_plugin_scope')
  }
  visible() {
    return [...this.panels]
      .filter(([, p]) => !p.element.hidden)
      .map(([id]) => id)
  }
  async activate(state?: unknown) {
    const result = Promise.resolve(this.module.activate(this.context, state))
    void result.then(
      (cleanup) => {
        if (typeof cleanup === 'function') {
          if (this.abort.signal.aborted)
            void Promise.resolve(cleanup()).catch(console.error)
          else this.context.scope.add(cleanup)
        }
      },
      () => {},
    )
    await bounded(result, 'Plugin activation')
  }
  restore(ids: string[]) {
    for (const id of ids) this.panels.get(id)?.open()
  }
  publish() {
    if (this.abort.signal.aborted) throw new Error('stale_plugin_scope')
    this.ready = true
    this.published = true
    this.dock.tabs.append(this.tabFragment)
    this.dock.body.append(this.panelFragment)
    this.dock.commands.append(this.commandFragment)
    if (this.info.manifest!.styles) {
      const link = document.createElement('link')
      link.rel = 'stylesheet'
      link.href = this.info.lease!.baseURL + this.info.manifest!.styles
      document.head.append(link)
      this.links.push(link)
    }
    for (const arm of this.arms) arm()
    this.dock.changed()
  }
  dispose(): Promise<void> {
    if (this.disposal) return this.disposal
    this.abort.abort()
    this.ready = false
    const results: Promise<unknown>[] = []
    for (const dispose of [...this.disposers].reverse())
      try {
        results.push(Promise.resolve(dispose()))
      } catch (error) {
        results.push(Promise.reject(error))
      }
    for (const link of this.links) link.remove()
    this.links = []
    this.arms.clear()
    this.dock.changed()
    this.disposal = bounded(
      Promise.allSettled(results).then((results) => {
        const rejected = results.find((r) => r.status === 'rejected')
        if (rejected?.status === 'rejected') throw rejected.reason
      }),
      'Plugin cleanup',
      1000,
    ).then(() => {})
    return this.disposal
  }
}
