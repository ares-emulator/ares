const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('ares', {
  ready: () => ipcRenderer.send('ready'),
  open: options => ipcRenderer.invoke('open', options),
  systems: () => ipcRenderer.invoke('systems'),
  settings: () => ipcRenderer.invoke('settings'),
  command: data => ipcRenderer.send('command', data),
  onStatus: callback => ipcRenderer.on('status', (_event, text) => callback(text)),
});
ipcRenderer.on('frames', event => {
  window.postMessage({ type: 'ares-frames' }, '*', event.ports);
});
