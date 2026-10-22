import { mountSettingsForm } from '../settings/form.js'
import type { PluginSettingsRequest } from '../settings/contracts.js'
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
  commands: HTMLElement
  registerPanel(id: string, title: string, mount: (context: import('../workspace/runtime.js').PanelMount) => void | Disposer): Disposer
  openPanel(id: string, automatic?: boolean): void
  changed(): void
  error(error: unknown): void
}
/** Owns every SDK effect; registrations and timers remain inert until publish. */
export class PluginScope {
  readonly abort = new AbortController()
  readonly token: string
  private disposers = new Set<Disposer>()
  private arms = new Set<() => void>()
  private commandFragment = document.createDocumentFragment()
  private panels = new Map<string, { instances: Set<{ visible: boolean }>; open(automatic?: boolean): void }>()
  private pendingOpens = new Set<string>()
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
    let currentMount: { disposers: Set<Disposer>; visible: boolean } | undefined
    const add = (dispose: Disposer): Disposer => {
      const owner = currentMount
      alive()
      let active = true
      const once = () => {
        if (!active) return
        active = false
        this.disposers.delete(once)
        owner?.disposers.delete(once)
        return dispose()
      }
      this.disposers.add(once)
      owner?.disposers.add(once)
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
    const settingsRequest = async (request: PluginSettingsRequest) => {
      alive()
      if (['set', 'reset', 'chooseFile'].includes(request.operation)) writable()
      if (this.pending >= 8) throw new Error('busy')
      ++this.pending
      try {
        const snapshot = await window.ares.plugins.settings(this.token, request)
        alive(); return snapshot
      } finally { --this.pending }
    }

    this.context = {
      id: m.id,
      session: lease.identity,
      system: lease.system,
      game: lease.game,
      profile: lease.profile,
      signal: this.abort.signal,
      settings: {
        catalog: () => settingsRequest({ operation: 'catalog' }),
        set: (id, value) => settingsRequest({ operation: 'set', id, value }),
        reset: id => settingsRequest({ operation: 'reset', id }),
        chooseFile: id => settingsRequest({ operation: 'chooseFile', id }),
        changed: callback => effect(() => window.ares.preferences.changed(() => {
          void settingsRequest({ operation: 'catalog' }).then(snapshot => {
            if (!this.abort.signal.aborted) callback(snapshot)
          }).catch(() => {})
        })),
        registerSection: async (section, descriptors) => {
          await settingsRequest({ operation: 'register', section, descriptors })
          return add(() => {
            if (this.abort.signal.aborted) return
            return window.ares.plugins.settings(this.token, { operation: 'unregister', section }).then(() => {}, () => {})
          })
        },
        mount: element => {
          alive()
          return effect(() => mountSettingsForm(element, this.context.settings))
        },
      },
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
        listen: (target, event, listener) => {
          const view = currentMount
          return effect(() => {
            const guarded: EventListener = (e) => {
              if (!this.abort.signal.aborted && (!view || view.visible))
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
          })
        },
        interval: (callback, milliseconds) => {
          const view = currentMount
          if (!Number.isFinite(milliseconds) || milliseconds < 50)
            throw new Error('Plugin intervals must be at least 50ms')
          if (this.arms.size >= 128)
            throw new Error('At most 128 scoped effects')
          return effect(() => {
            const timer = setInterval(() => {
              if (
                !this.abort.signal.aborted &&
                (view ? view.visible : [...this.panels.values()].some(p => [...p.instances].some(instance => instance.visible)))
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
          const contribution = m.contributes.panels?.find(p => p.id === id)
          if (!contribution || this.panels.has(id)) throw new Error('Undeclared or duplicate panel: ' + id)
          const instances = new Set<{ visible: boolean }>()
          const install = effect(() => this.dock.registerPanel(id, contribution.title, context => {
            const instance = { visible: true, disposers: new Set<Disposer>() }
            const abort = new AbortController(), visibility = new Set<Disposer>()
            instances.add(instance)
            const cancel = context.onVisibility(visible => { instance.visible = visible })
            const previous = currentMount; currentMount = instance
            let cleanup: void | Disposer
            try {
              cleanup = mount({
                instanceId: context.instanceId,
                element: context.element,
                signal: abort.signal,
                onVisibility: callback => {
                  alive()
                  const dispose = context.onVisibility(visible => { if (!abort.signal.aborted && !this.abort.signal.aborted) callback(visible) })
                  visibility.add(dispose)
                  return () => { visibility.delete(dispose); return dispose() }
                },
              })
            } catch (error) {
              abort.abort(); instances.delete(instance); void cancel()
              for (const dispose of [...instance.disposers].reverse()) void dispose()
              for (const dispose of visibility) void dispose()
              throw error
            } finally { currentMount = previous }
            let disposed = false
            return async () => {
              if (disposed) return
              disposed = true; abort.abort(); instances.delete(instance)
              const actions = [cancel, ...visibility, ...[...instance.disposers].reverse(), ...(cleanup ? [cleanup] : [])]
              const settled = await Promise.allSettled(actions.map(dispose => Promise.resolve().then(dispose)))
              const failed = settled.find(result => result.status === 'rejected')
              if (failed?.status === 'rejected') throw failed.reason
            }
          }))
          this.panels.set(id, { instances, open: automatic => this.dock.openPanel(id, automatic) })
          return add(() => { this.panels.delete(id); return install() })
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
        openPanel: id => {
          alive()
          if (!this.panels.has(id)) throw new Error('Unknown panel: ' + id)
          if (!this.published) this.pendingOpens.add(id)
          else this.panels.get(id)!.open()
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
  publish() {
    if (this.abort.signal.aborted) throw new Error('stale_plugin_scope')
    this.ready = true
    this.published = true
    this.dock.commands.append(this.commandFragment)
    if (this.info.manifest!.styles) {
      const link = document.createElement('link')
      link.rel = 'stylesheet'
      link.href = this.info.lease!.baseURL + this.info.manifest!.styles
      document.head.append(link)
      this.links.push(link)
    }
    for (const arm of this.arms) arm()
    for (const id of this.pendingOpens) this.panels.get(id)?.open(true)
    this.pendingOpens.clear()
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
