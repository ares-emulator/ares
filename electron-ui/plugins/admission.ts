/** Shared renderer/host per-plugin admission bound; no queue or replay. */
export const PLUGIN_REQUEST_LIMIT = 8
export class PluginAdmissionError extends Error {
  readonly code = 'request_overload'
  readonly limit = PLUGIN_REQUEST_LIMIT
  constructor(readonly pending: number) {
    super(`request_overload: pending=${pending}, limit=${PLUGIN_REQUEST_LIMIT}`)
    this.name = 'PluginAdmissionError'
  }
}

/** Plain data crosses both IPC and contextBridge; Errors lose custom fields. */
export interface PluginBridgeFailure {
  plugin_error: { code: string; message: string; pending?: number; limit?: number }
}
export async function pluginBridgeResult<T>(run: () => Promise<T>): Promise<T | PluginBridgeFailure> {
  try { return await run() } catch (error) {
    const e = error as {code?: unknown; message?: unknown; pending?: unknown; limit?: unknown}
    const message = typeof e?.message === 'string' ? e.message : String(error)
    const proposed = typeof e?.code === 'string' ? e.code : message.split(':')[0]!
    const code = /^[a-z][a-z0-9_]{0,63}$/.test(proposed) ? proposed : 'plugin_request_failed'
    return {plugin_error: {code, message,
      ...(Number.isSafeInteger(e?.pending) ? {pending: e.pending as number} : {}),
      ...(Number.isSafeInteger(e?.limit) ? {limit: e.limit as number} : {})}}
  }
}
export function unwrapPluginBridge<T>(value: T | PluginBridgeFailure): T {
  if (value && typeof value === 'object' && 'plugin_error' in value) {
    const detail = (value as PluginBridgeFailure).plugin_error
    throw Object.assign(new Error(detail.message), detail)
  }
  return value as T
}
