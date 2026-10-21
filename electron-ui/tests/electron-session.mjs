import { fileURLToPath } from 'node:url';
// Run in a disposable Electron app profile, never the user's running app.
import { app, BrowserWindow } from 'electron';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { once } from 'node:events';
const __dirname = path.dirname(fileURLToPath(import.meta.url));
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-electron-session-'));
app.setPath('userData', path.join(temporary, 'profile'));
process.env.ARES_SAVE_DIR = path.join(temporary, 'profile', 'saves');
delete process.env.ARES_ROM;
delete process.env.ARES_AUTOTEST;
const { sessions } = await import('../dist/main.js');
import { MemoryClient, hexToBytes } from '../dist/services/memory.js';
const memory = new MemoryClient(sessions);
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(check) {
  for (let i = 0; i < 200; i++) {
    try { const result = await check(); if (result) return result; } catch {}
    await sleep(25);
  }
  throw new Error('Electron check timed out');
}
let stage = 'startup';
const enter = value => { stage = value; console.log(`INTEGRATION_STAGE: ${value}`); };
const timeout = setTimeout(() => { console.error(`Electron session test timed out: ${stage}`); app.exit(1); }, 60000);
app.on('will-quit', () => { clearTimeout(timeout); fs.rmSync(temporary, { recursive: true, force: true }); });
app.whenReady().then(async () => {
  enter('native startup');
  const first = await until(() => sessions.request('session.status'));
  assert.equal(first.result.system, 'Synthetic');
  enter('renderer startup');
  const window = BrowserWindow.getAllWindows()[0];
  const rendererErrors = [];
  window.webContents.on('console-message', (_event, details) => { if (details.level === 'error') rendererErrors.push(details.message); });
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause')?.textContent === 'Pause'"));
  // Renderer JavaScript is blocked. Main/native controls must keep working.
  enter('stalled renderer controls');
  const blocked = window.webContents.executeJavaScript('(() => { const end = performance.now() + 750; while (performance.now() < end) {} return true; })()');
  await sleep(75);
  const started = performance.now();
  const paused = await sessions.request('session.pause');
  assert.equal(paused.result.paused, true);
  assert.ok(performance.now() - started < 350, 'Native control waited for the renderer');
  await blocked;
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Resume'"));
  await until(() => window.webContents.executeJavaScript("document.getElementById('memory-region').options.length === 3"));
  enter('inspector');
  const fixture = (await memory.regions())[0];
  const original = await memory.read(fixture, 240, 4);
  try {
    await window.webContents.executeJavaScript(`
      document.getElementById('memory-offset').value = '0xf0';
      document.getElementById('memory-length').value = '4';
      document.getElementById('memory-bytes').value = 'de ad be ef';
      document.getElementById('memory-write').click();
    `);
    await until(() => window.webContents.executeJavaScript("document.getElementById('memory-output').textContent.includes('Wrote 4 bytes')"));
    assert.equal((await memory.read(fixture, 240, 4)).hex, 'deadbeef');
    await window.webContents.executeJavaScript("document.getElementById('memory-read').click()");
    await until(() => window.webContents.executeJavaScript("document.getElementById('memory-output').textContent.includes('deadbeef')"));
    await window.webContents.executeJavaScript(`(async () => {
      const aside = document.querySelector('aside'), target = document.getElementById('memory-refresh');
      aside.scrollTop += target.getBoundingClientRect().top - aside.getBoundingClientRect().top - 12;
      await new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    })()`);
    const screenshot = await window.webContents.capturePage();
    const artifact = path.resolve(__dirname, '../artifacts/memory-inspector.png');
    fs.mkdirSync(path.dirname(artifact), { recursive: true }); fs.writeFileSync(artifact, screenshot.toPNG());
  } finally {
    await memory.write(fixture, 240, hexToBytes(original.hex));
    assert.equal((await memory.read(fixture, 240, 4)).hex, original.hex);
  }
  enter('paused reload');
  const loaded = once(window.webContents, 'did-finish-load');
  window.webContents.reload(); await loaded;
  const after = await sessions.request('session.status');
  assert.equal(after.session, first.session);
  assert.equal(after.epoch, first.epoch);
  assert.equal(after.result.paused, true);
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Resume'"));
  await window.webContents.executeJavaScript("document.getElementById('pause').click()");
  await until(async () => !(await sessions.request('session.status')).result.paused);
  assert.equal((await sessions.request('session.status')).session, first.session);
  // Reload while running too; stale frame-port acknowledgements must not grant
  // an extra native credit or violate the renderer's one-frame queue.
  enter('running reload');
  const loadedRunning = once(window.webContents, 'did-finish-load');
  window.webContents.reload(); await loadedRunning;
  await until(() => window.webContents.executeJavaScript('stats().frames > 5'));
  assert.equal((await sessions.request('session.status')).session, first.session);
  assert.equal(await window.webContents.executeJavaScript('stats().maxPending'), 1);
  assert.equal(await window.webContents.executeJavaScript('stats().glError'), 0);
  assert.ok(!rendererErrors.some(message => message.includes('Frame credit')), rendererErrors.join('\n'));
  await window.webContents.executeJavaScript("document.getElementById('synthetic').click()");
  const replacement = await until(async () => {
    const reply = await sessions.request('session.status');
    return reply.session !== first.session && reply;
  });
  assert.equal(replacement.epoch, 1);
  assert.equal((await sessions.request('session.pause', { session: first.session, epoch: first.epoch })).error, 'stale_session');
  assert.equal((await sessions.request('session.status')).result.paused, false);
  console.log('ELECTRON_SESSION_PASS: stalled renderer controls, reload identity, authoritative pause/resume, inspector read/write/restore');
  app.quit();
}).catch(error => { console.error(error); app.exit(1); });
