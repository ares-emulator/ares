const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('ares', {
  session: (method, identity, params) => ipcRenderer.invoke('session', method, identity, params),
  ready: () => ipcRenderer.send('ready'),
  open: options => ipcRenderer.invoke('open', options),
  systems: () => ipcRenderer.invoke('systems'),
  settings: () => ipcRenderer.invoke('settings'),
  command: data => ipcRenderer.send('command', data),
  onSessionState: callback => ipcRenderer.on('session-state', (_event, reply) => callback(reply)),
  onStatus: callback => ipcRenderer.on('status', (_event, text) => callback(text)),
});
ipcRenderer.on('frames', event => {
  window.postMessage({ type: 'ares-frames' }, '*', event.ports);
});
