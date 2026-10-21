import type {
  SessionIdentity,
  ServiceMethod,
  ServiceParams,
  ServiceResult,
  SessionReply,
} from '../services/contracts.js'
import type { GameIdentity } from './contracts.js'
export type Disposer = () => void | Promise<void>
export interface PanelContext {
  element: HTMLElement
  onVisibility(callback: (visible: boolean) => void): Disposer
}
export interface PluginContext {
  readonly id: string
  readonly session: SessionIdentity
  readonly system: string
  readonly game: GameIdentity | null
  readonly profile: string | null
  readonly signal: AbortSignal
  services: {
    request<M extends ServiceMethod>(
      method: M,
      params?: ServiceParams<M>,
    ): Promise<SessionReply<ServiceResult<M>>>
  }
  ui: {
    registerPanel(
      id: string,
      mount: (context: PanelContext) => void | Disposer,
    ): Disposer
    registerCommand(id: string, run: () => void | Promise<void>): Disposer
    openPanel(id: string): void
  }
  scope: {
    add(dispose: Disposer): Disposer
    listen(
      target: EventTarget,
      event: string,
      listener: EventListener,
    ): Disposer
    interval(callback: () => void, milliseconds: number): Disposer
  }
  storage: {
    get<T = unknown>(key: string): Promise<T | undefined>
    set(key: string, value: unknown): Promise<void>
    delete(key: string): Promise<void>
  }
  log: {
    info(message: string): void
    warn(message: string): void
    error(message: string): void
  }
}
export interface PluginModule {
  /** Pure candidate validation without service or UI effects. */
  prepare?(): void | Promise<void>
  stateVersion?: number
  saveState?(): unknown | Promise<unknown>
  activate(
    context: PluginContext,
    state?: unknown,
  ): void | Disposer | Promise<void | Disposer>
}
