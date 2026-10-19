import { app, BrowserWindow, ipcMain, dialog, MessageChannelMain, type MessagePortMain } from 'electron';
import { spawn, execFile, type ChildProcessByStdio } from 'node:child_process';
import type { Readable, Writable } from 'node:stream';
import path from 'node:path';
import fs from 'node:fs';
import { fileURLToPath } from 'node:url';
import { performance } from 'node:perf_hooks';
import { FrameParser, type NativeFrame } from './protocol.js';
import { randomUUID } from 'node:crypto';
import { NativeSessionClient } from './services/native-client.js';
import { LocalServiceHost } from './services/local-endpoint.js';
import { SessionRouter } from './services/session-router.js';
import type { NativeCommand, StartOptions } from './bridge.js';
import type { SessionIdentity, ServiceMethod, ServiceParams } from './services/contracts.js';
// Compiled entry lives in dist/ in both checkout and copied distribution.
const appRoot = fileURLToPath(new URL('../', import.meta.url));
interface NativeSession {
  child: ChildProcessByStdio<Writable, Readable, Readable>;
  system: string; generation: number; outstanding: boolean; lastSequence: number;
  diagnostic: string; failure?: string; lastFrame?: NativeFrame; closed: Promise<void>;
}
export const sessions = new SessionRouter(reply => {
  if (window && !window.isDestroyed() && !window.webContents.isDestroyed()) window.webContents.send('session-state', reply);
});
export const serviceHost = new LocalServiceHost(sessions);
export { stop as stopNative };
const installedHost = path.join(process.resourcesPath, 'native/bin/ares-electron-host');
const installedLayout = app.isPackaged || fs.existsSync(installedHost);
const hostPath = process.env.ARES_HOST || (installedLayout ? installedHost : path.resolve(appRoot, '../../build-electron-make/bin/ares-electron-host'));
let generation = 0, settingsFile = process.env.ARES_SETTINGS || '';
const probe = (args: string[]): Promise<string> => new Promise((resolve, reject) => {
  execFile(hostPath, args, { timeout: 30000 }, (error, stdout, stderr) => {
    if (error) reject(new Error(stderr.trim() || error.message)); else resolve(stdout.trim());
  });
});
let window: BrowserWindow;
let active: NativeSession | null = null, port: MessagePortMain | null = null;
let quitting = false, operation: Promise<void> = Promise.resolve();
const status = (text: string) => { if (window && !window.isDestroyed()) window.webContents.send('status', text); };
function command(text: string) {
  if (active && !active.child.stdin.destroyed && active.child.stdin.writable) active.child.stdin.write(text + '\n');
}
async function stop() {
  const old = active;
  if (!old) return;
  active = null;
  sessions.detach();
  // EOF also releases the native command reader. Escalate only for a stuck host.
  old.child.stdin.end('quit\n');
  const timer = setTimeout(() => old.child.kill('SIGKILL'), 2000);
  await old.closed;
  clearTimeout(timer);
}
async function start(options: StartOptions) {
  const saveDirectory = process.env.ARES_SAVE_DIR || (installedLayout ? path.join(app.getPath('userData'), 'saves') : path.join(appRoot, 'artifacts', 'saves'));
  fs.mkdirSync(saveDirectory, { recursive: true });
  let system = 'Synthetic';
  // Probe before stopping the running game, so an unsupported file leaves it intact.
  if (options.rom) {
    const identifyArgs = ['--identify', options.rom, '--save-dir', saveDirectory];
    if (options.system) identifyArgs.push('--system', options.system);
    if (settingsFile) identifyArgs.push('--settings', settingsFile);
    const matches = (await probe(identifyArgs)).split('\n').filter(Boolean);
    system = matches[0];
    if (matches.length > 1) {
      const choice = await dialog.showMessageBox(window, { type: 'question', title: 'Choose system',
        message: 'This file can be loaded by more than one system.', buttons: [...matches, 'Cancel'], cancelId: matches.length });
      if (choice.response === matches.length) return;
      system = matches[choice.response];
    }
  }
  await stop();
  const args = options.rom ? ['--rom', options.rom] : ['--synthetic'];
  if (options.rom) args.push('--system', system);
  if (process.env.ARES_ENABLE_SERVICES === '1' || process.argv.includes('--services')) args.push('--debug-provenance');
  for (const media of options.media || []) args.push('--media', media);
  if (settingsFile) args.push('--settings', settingsFile);
  if (options.hd) args.push('--1080p');
  if (options.silent || process.env.ARES_SILENT === '1') args.push('--silent');
  args.push('--save-dir', saveDirectory);
  const identity = { session: randomUUID(), epoch: 1 };
  args.push('--control-session', identity.session);
  const child = spawn(hostPath, args, { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] }) as ChildProcessByStdio<Writable, Readable, Readable>;
  const session: NativeSession = { child, system, generation: ++generation, outstanding: false, lastSequence: 0, diagnostic: '', closed: Promise.resolve() };
  session.closed = new Promise(resolve => child.once('close', (code, signal) => {
    if (active === session) { active = null; sessions.detach(); status(session.failure || ((code !== 0 || signal) ? `Host stopped (${signal || code}). ${session.diagnostic}` : 'Host stopped. Open a ROM or restart a test.')); }
    resolve();
  }));
  active = session;
  const client = new NativeSessionClient(child.stdin, child.stdio[3] as Readable, identity, error => {
    if (active === session) { session.failure = String(error); status(session.failure); child.kill(); }
  });
  sessions.attach(client);
  child.on('error', error => status(`Cannot start native host: ${error.message}`));
  child.stdin.on('error', () => {}); // exit races are reported by close/error
  child.stderr.on('data', data => { process.stderr.write(data); session.diagnostic = data.toString().trim(); if (active === session) status(session.diagnostic); });
  const parser = new FrameParser(frame => {
    if (active !== session) return;
    session.outstanding = true;
    session.lastSequence = frame.sequence;
    session.lastFrame = frame;
    // This is intentionally a copied baseline. MessagePortMain does not transfer ArrayBuffers.
    port?.postMessage({ ...frame, generation: session.generation, receivedMs: performance.timeOrigin + performance.now() });
  });
  child.stdout.on('data', data => {
    try { parser.push(data); } catch (error) { session.failure = String(error); status(session.failure); child.kill(); }
  });
  port?.postMessage({ reset: true, system });
  command('ack');
}
function enqueue(task: () => Promise<void>) {
  operation = operation.then(task).catch(error => { status(error.message); console.error(error); });
  return operation;
}
app.whenReady().then(async () => {
  if (process.env.ARES_ENABLE_SERVICES === '1' || process.argv.includes('--services')) {
    const file = await serviceHost.listen(process.env.ARES_SERVICE_DIR);
    console.error(`ares service instance ${serviceHost.instance}: ${file}`);
  }
  window = new BrowserWindow({ width: 1100, height: 800, minWidth: 650, minHeight: 480,
    webPreferences: { preload: path.join(appRoot, 'dist/preload.cjs'), contextIsolation: true,
      nodeIntegration: false, sandbox: true, backgroundThrottling: false } });
  window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  window.webContents.on('will-navigate', event => event.preventDefault());
  window.webContents.on('render-process-gone', () => { port?.close(); port = null; });
  window.webContents.on('did-start-loading', () => { port?.close(); port = null; });
  ipcMain.on('ready', event => {
    if (event.sender !== window.webContents) return;
    port?.close();
    const channel = new MessageChannelMain();
    port = channel.port1;
    port.on('message', ({ data }) => {
      if (port !== channel.port1) return;
      if (active && data?.generation === active.generation && data?.ack === active.lastSequence && active.outstanding) {
        active.outstanding = false; command('ack');
      }
    });
    port.start();
    window.webContents.postMessage('frames', null, [channel.port2]);
    if (active) {
      port.postMessage({ reset: true, system: active.system });
      if (active.outstanding && active.lastFrame) port.postMessage({ ...active.lastFrame, generation: active.generation, receivedMs: performance.timeOrigin + performance.now() });
    } else enqueue(() => start(process.env.ARES_ROM ? { rom: process.env.ARES_ROM, system: process.env.ARES_SYSTEM } : { hd: process.env.ARES_HD === '1' }));
  });
  ipcMain.handle('session', async (event, method: ServiceMethod, identity?: SessionIdentity, params?: ServiceParams<ServiceMethod>) => {
    if (event.sender !== window.webContents || event.senderFrame !== window.webContents.mainFrame) throw new Error('invalid_sender');
    if (!['session.status', 'memory.regions'].includes(method) && !identity) throw new Error('invalid_request');
    return sessions.request(method, identity, params);
  });
  ipcMain.handle('systems', async event => {
    if (event.sender !== window.webContents) return [];
    return (await probe(['--systems'])).split('\n').filter(Boolean);
  });
  ipcMain.handle('settings', async event => {
    if (event.sender !== window.webContents) return;
    const result = await dialog.showOpenDialog(window, { title: 'Use ares firmware and core settings',
      properties: ['openFile'], filters: [{ name: 'ares settings', extensions: ['bml'] }] });
    if (!result.canceled) { settingsFile = result.filePaths[0]; status('Selected ares settings; they will apply to the next ROM.'); }
  });
  ipcMain.handle('open', async (event, options) => {
    if (event.sender !== window.webContents) return;
    const result = await dialog.showOpenDialog(window, { title: 'Open ROM (additional selections supply companion media)',
      properties: ['openFile', 'multiSelections'], filters: [{ name: 'All ROMs and media', extensions: ['*'] }] });
    if (!result.canceled) await enqueue(() => start({ rom: result.filePaths[0], media: result.filePaths.slice(1),
      system: typeof options?.system === 'string' ? options.system : '' }));
  });
  ipcMain.on('command', (event, data: NativeCommand) => {
    if (event.sender !== window.webContents || !data) return;
    if (data.type === 'synthetic') enqueue(() => start({ hd: data.hd === true }));

    if (data.type === 'input' && Number.isInteger(data.sequence) && Number.isInteger(data.mask)) {
      const axis = (value: number | undefined) => typeof value === 'number' && Number.isFinite(value) ? Math.max(-32767, Math.min(32767, Math.round(value))) : 0;
      command(`input ${data.sequence >>> 0} ${data.mask >>> 0} ${axis(data.x)} ${axis(data.y)} ${axis(data.rx)} ${axis(data.ry)}`);
    }
    if (data.type === 'key' && typeof data.name === 'string' && /^[A-Za-z0-9]{1,32}$/.test(data.name)) {
      command(`key ${data.name} ${data.value ? 1 : 0}`);
    }
    if (data.type === 'clear') command('clear');
    if (data.type === 'mouse') {
      const limit = (value: number | undefined) => typeof value === 'number' && Number.isFinite(value) ? Math.max(-32767, Math.min(32767, Math.round(value))) : 0;
      command(`mouse ${limit(data.x)} ${limit(data.y)} ${data.mask & 15}`);
    }
    if (data.type === 'report') {
      // Opt-in automated renderer smoke test; no general filesystem API is exposed.
      if (process.env.ARES_REPORT) fs.writeFileSync(process.env.ARES_REPORT, JSON.stringify(data.report, null, 2));
      if (process.env.ARES_AUTOTEST === '1') app.quit();
    }
  });
  window.loadFile(path.join(appRoot, 'index.html'), { query: {
    autotest: process.env.ARES_AUTOTEST || '', duration: process.env.ARES_TEST_SECONDS || '12'
  } });
}).catch(error => { console.error(error); app.quit(); });
app.on('window-all-closed', () => app.quit());
app.on('before-quit', event => {
  if (quitting) return;
  event.preventDefault(); quitting = true;
  enqueue(stop).finally(async () => { await serviceHost.close(); app.quit(); });
});
