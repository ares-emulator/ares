import { contextBridge, ipcRenderer } from 'electron'
import type { AresBridge } from './bridge.js'
const api: AresBridge = {
  preferences: {
    catalog: () => ipcRenderer.invoke('preferences.catalog'),
    set: (id, value) => ipcRenderer.invoke('preferences.set', id, value),
    reset: id => ipcRenderer.invoke('preferences.reset', id),
    chooseFile: id => ipcRenderer.invoke('preferences.chooseFile', id),
    changed: callback => {
      const listener = (_event: unknown, snapshot: import('./settings/contracts.js').SettingsSnapshot) => callback(snapshot)
      ipcRenderer.on('preferences-changed', listener)
      return () => ipcRenderer.removeListener('preferences-changed', listener)
    },
  },
  plugins: {
    settings: (token, request) => ipcRenderer.invoke('plugin.settings', token, request),
    actions: (callback) => {
      const listener = (_event: unknown, action: any) => callback(action)
      ipcRenderer.on('plugin-action', listener)
      return () => ipcRenderer.removeListener('plugin-action', listener)
    },
    result: (id, token, error) =>
      ipcRenderer.invoke('plugin.actionResult', id, token, error),
    reload: (id) => ipcRenderer.invoke('plugin.reload', id),
    begin: (token) => ipcRenderer.invoke('plugin.begin', token),
    finish: (token) => ipcRenderer.invoke('plugin.finish', token),
    reject: (token, message) =>
      ipcRenderer.invoke('plugin.reject', token, message),
    rollback: (token, message) =>
      ipcRenderer.invoke('plugin.rollback', token, message),
    fail: (token, message) => ipcRenderer.invoke('plugin.fail', token, message),
    catalog: () => ipcRenderer.invoke('plugin.catalog'),
    enable: (id, enabled) => ipcRenderer.invoke('plugin.enable', id, enabled),
    add: () => ipcRenderer.invoke('plugin.add'),
    refresh: () => ipcRenderer.invoke('plugin.refresh'),
    request: (token, method, params) =>
      ipcRenderer.invoke('plugin.request', token, method, params),
    storage: (token, op, key, value) =>
      ipcRenderer.invoke('plugin.storage', token, op, key, value),
    log: (token, level, message) =>
      ipcRenderer.invoke('plugin.log', token, level, message),
    changed: (callback) => {
      const listener = (_event: unknown, catalog: any) => callback(catalog)
      ipcRenderer.on('plugin-catalog', listener)
      return () => ipcRenderer.removeListener('plugin-catalog', listener)
    },
  },
  session: (method, identity, params) =>
    ipcRenderer.invoke('session', method, identity, params),
  ready: () => ipcRenderer.send('ready'),
  open: (options) => ipcRenderer.invoke('open', options),
  systems: () => ipcRenderer.invoke('systems'),
  settings: () => ipcRenderer.invoke('settings'),
  command: (data) => ipcRenderer.send('command', data),
  onSessionState: (callback) =>
    ipcRenderer.on('session-state', (_event, reply) => callback(reply)),
  onStatus: (callback) =>
    ipcRenderer.on('status', (_event, text) => callback(text)),
}
contextBridge.exposeInMainWorld('ares', api)
ipcRenderer.on('frames', (event) => {
  window.postMessage({ type: 'ares-frames' }, '*', event.ports)
})
