'use strict';
// Run in a disposable Electron app profile, never the user's running app.
const { app, BrowserWindow } = require('electron');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { once } = require('node:events');
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-electron-session-'));
app.setPath('userData', path.join(temporary, 'profile'));
process.env.ARES_SAVE_DIR = path.join(temporary, 'saves');
delete process.env.ARES_ROM;
delete process.env.ARES_AUTOTEST;
const { sessions } = require('../main.cjs');
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
async function until(check) {
  for (let i = 0; i < 200; i++) {
    try { const result = await check(); if (result) return result; } catch {}
    await sleep(25);
  }
  throw new Error('Electron check timed out');
}
const timeout = setTimeout(() => { console.error('Electron session test timed out'); app.exit(1); }, 20000);
app.on('will-quit', () => { clearTimeout(timeout); fs.rmSync(temporary, { recursive: true, force: true }); });
app.whenReady().then(async () => {
  const first = await until(() => sessions.request('session.status'));
  assert.equal(first.result.system, 'Synthetic');
  const window = BrowserWindow.getAllWindows()[0];
  const rendererErrors = [];
  window.webContents.on('console-message', (_event, details) => { if (details.level === 'error') rendererErrors.push(details.message); });
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause')?.textContent === 'Pause'"));
  // Renderer JavaScript is blocked. Main/native controls must keep working.
  const blocked = window.webContents.executeJavaScript('(() => { const end = performance.now() + 750; while (performance.now() < end) {} return true; })()');
  await sleep(75);
  const started = performance.now();
  const paused = await sessions.request('session.pause');
  assert.equal(paused.result.paused, true);
  assert.ok(performance.now() - started < 350, 'Native control waited for the renderer');
  await blocked;
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
  console.log('ELECTRON_SESSION_PASS: stalled renderer controls, reload identity, authoritative pause/resume');
  app.quit();
}).catch(error => { console.error(error); app.exit(1); });
