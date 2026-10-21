import type {
  SessionIdentity,
  SessionStatus,
  ServiceMethod,
  ServiceParams,
  ServiceResult,
  SessionReply,
} from '../services/contracts.js'
export const PLUGIN_API_VERSION = 1
export const CAPABILITIES = [
  'session.read',
  'session.control',
  'memory.read',
  'memory.write',
] as const
export type Capability = (typeof CAPABILITIES)[number]
export interface PluginManifest {
  id: string
  name: string
  version: string
  apiVersion: 1
  entry: string
  styles?: string
  mode?: 'panel' | 'overlay' | 'page' | 'shell'
  storageVersion?: number
  activation?: {
    systems?: string[]
    games?: { profile: string; sha256: string; revision?: string }[]
  }
  capabilities: Capability[]
  contributes: {
    panels?: { id: string; title: string }[]
    commands?: { id: string; title: string }[]
  }
}
export interface GameIdentity {
  sha256: string
  size: number
  kind: 'program.rom'
}
export interface PluginLease {
  token: string
  identity: SessionIdentity
  system: string
  game: GameIdentity | null
  profile: string | null
  baseURL: string
}
export interface PluginInfo {
  manifest?: PluginManifest
  source: string
  bundled: boolean
  enabled: boolean
  state: 'disabled' | 'waiting' | 'active' | 'error' | 'unsupported'
  error?: string
  lease?: PluginLease
  logs: { level: string; message: string }[]
}
export interface PluginCatalog {
  apiVersion: 1
  plugins: PluginInfo[]
}
export interface PluginBridge {
  catalog(): Promise<PluginCatalog>
  enable(id: string, enabled: boolean): Promise<PluginCatalog>
  add(): Promise<PluginCatalog>
  refresh(): Promise<PluginCatalog>
  fail(token: string, message: string): Promise<void>
  request<M extends ServiceMethod>(
    token: string,
    method: M,
    params?: ServiceParams<M>,
  ): Promise<SessionReply<ServiceResult<M>>>
  storage(
    token: string,
    operation: 'get' | 'set' | 'delete',
    key: string,
    value?: unknown,
  ): Promise<unknown>
  log(
    token: string,
    level: 'info' | 'warn' | 'error',
    message: string,
  ): Promise<void>
  changed(callback: (catalog: PluginCatalog) => void): () => void
}
export type PluginStatusReply = SessionReply<SessionStatus>
