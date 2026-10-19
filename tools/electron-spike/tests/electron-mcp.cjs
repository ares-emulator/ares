'use strict';
const { app, BrowserWindow } = require('electron');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { once } = require('node:events');
const { makeFixture } = require('../fixture.cjs');
const { StdioPeer } = require('./stdio-peer.cjs');
const { LocalServiceClient } = require('../dist/services/local-endpoint.js');
const temporary = fs.mkdtempSync('/tmp/ares-mcp-');
app.setPath('userData', path.join(temporary, 'profile'));
const rom = path.join(temporary, 'fixture.gb'), settings = path.join(temporary, 'settings.bml');
fs.writeFileSync(rom, makeFixture()); fs.writeFileSync(settings, '');
Object.assign(process.env, { ARES_ROM: rom, ARES_SYSTEM: 'Game Boy', ARES_SILENT: '1', ARES_SETTINGS: settings, ARES_SAVE_DIR: path.join(temporary, 'saves'), ARES_ENABLE_SERVICES: '1', ARES_SERVICE_DIR: path.join(temporary, 'ipc') });
delete process.env.ARES_AUTOTEST;
const { sessions, serviceHost, stopNative } = require('../main.cjs');
const peers = [];
async function until(fn) { for (let i = 0; i < 200; ++i) { try { const result = await fn(); if (result) return result; } catch {} await new Promise(r => setTimeout(r, 25)); } throw new Error('Integration timed out'); }
function peer(instance = serviceHost.instance) {
  const result = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', serviceHost.metadataFile, '--instance', instance], { env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '' } });
  peers.push(result); return result;
}
const timeout = setTimeout(() => { console.error('MCP integration timeout'); app.exit(1); }, 30000);
app.on('will-quit', () => { clearTimeout(timeout); fs.rmSync(temporary, { recursive: true, force: true }); });
app.whenReady().then(async () => {
  const initial = await until(() => sessions.request('session.status'));
  assert.equal(initial.result.system, 'Game Boy');
  await until(() => serviceHost.metadataFile);
  const window = BrowserWindow.getAllWindows()[0];
  await until(() => window.webContents.executeJavaScript("document.getElementById('memory-region')?.options.length > 0"));
  const mcp = peer(); await mcp.initialize();
  const list = (await mcp.request('tools/list', {})).result.tools;
  assert.deepEqual(list.map(t => t.name).sort(), ['emu_status', 'pause', 'resume', 'memory_regions', 'read_memory', 'write_memory', 'run_frames', 'run_until_frame', 'step', 'operation_status', 'cancel_operation'].sort());
  assert.equal(list.find(t => t.name === 'read_memory').inputSchema.properties.length.default, 64);
  assert.equal(list.find(t => t.name === 'resume').inputSchema.properties.keep_overrides.default, false);
  assert.equal(list.find(t => t.name === 'emu_status').inputSchema.properties.verbose.default, false);
  for (const [name, properties, required] of [
    ['emu_status', ['verbose'], []], ['pause', [], []], ['resume', ['keep_overrides'], []],
    ['memory_regions', [], []], ['read_memory', ['addr', 'length', 'region'], ['addr', 'region']],
    ['write_memory', ['addr', 'hex_bytes', 'region'], ['addr', 'hex_bytes', 'region']],
    ['run_frames', ['cop0', 'fpr', 'max_seconds', 'n', 'screenshot_after', 'verbose'], ['n']],
    ['run_until_frame', ['k', 'max_seconds'], ['k']], ['step', [], []],
    ['operation_status', ['cop0', 'fpr', 'operation_id', 'verbose'], ['operation_id']],
    ['cancel_operation', ['operation_id'], []],
  ]) {
    const schema = list.find(t => t.name === name).inputSchema;
    assert.deepEqual(Object.keys(schema.properties).sort(), properties);
    assert.deepEqual((schema.required || []).sort(), required);
  }
  assert.equal(list.find(t => t.name === 'run_frames').inputSchema.properties.max_seconds.default, 60);
  assert.equal(list.find(t => t.name === 'run_frames').inputSchema.properties.screenshot_after.default, null);
  assert.equal(list.find(t => t.name === 'operation_status').inputSchema.properties.fpr.default, false);
  assert.equal(list.find(t => t.name === 'cancel_operation').inputSchema.properties.operation_id.default, null);
  assert.equal((await mcp.tool('pause')).paused, true);
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Resume'"));
  const regions = await mcp.tool('memory_regions'); assert.equal(regions.session, initial.session);
  const ram = regions.regions.find(r => r.name === 'CPU WRAM');
  assert.ok(ram);
  assert.equal((await mcp.tool('read_memory', { region: ram.id, addr: 0 })).len, 64);
  const baseline = await mcp.tool('read_memory', { region: ram.id, addr: '0xf0', length: 4 });
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: '0xf0', hex_bytes: 'deadbeef' })).written, 4);
  await window.webContents.executeJavaScript(`document.getElementById('memory-region').value=${JSON.stringify(ram.id)}; document.getElementById('memory-region').dispatchEvent(new Event('change'));`);
  await window.webContents.executeJavaScript("document.getElementById('memory-offset').value='0xf0'; document.getElementById('memory-length').value='4'; document.getElementById('memory-read').click()");
  await until(() => window.webContents.executeJavaScript("document.getElementById('memory-output').textContent.includes('deadbeef')"));
  assert.equal((await mcp.tool('read_memory', { region: 'CPU WRAM', addr: 240, length: 4 })).hex, 'deadbeef');
  const loaded = once(window.webContents, 'did-finish-load'); window.webContents.reload(); await loaded;
  assert.equal((await mcp.tool('emu_status')).session, initial.session);
  assert.equal((await mcp.tool('read_memory', { region: ram.id, addr: 240, length: 4 })).hex, 'deadbeef');
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: 240, hex_bytes: baseline.hex })).written, 4);
  const wrong = peer('wrong-instance'); await wrong.initialize(); assert.equal((await wrong.tool('emu_status')).error, 'wrong_instance');
  assert.equal((await mcp.tool('resume', { keep_overrides: true })).error, 'unavailable');
  assert.equal((await mcp.tool('step')).error, 'unavailable');
  const reconnect = new LocalServiceClient(serviceHost.metadataFile, serviceHost.instance);
  assert.equal((await reconnect.request('session.status')).session, initial.session);
  reconnect.disconnect(); assert.equal((await reconnect.request('session.status')).session, initial.session); reconnect.close();
  await sessions.request('session.reset');
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: 240, hex_bytes: '00' })).error, 'stale_epoch');
  assert.equal((await mcp.tool('memory_regions')).epoch, 2);
  await window.webContents.executeJavaScript("document.getElementById('synthetic').click()");
  await until(async () => (await sessions.request('session.status')).session !== initial.session);
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: 240, hex_bytes: '00' })).error, 'stale_session');
  const fresh = await mcp.tool('memory_regions');
  assert.equal((await mcp.tool('read_memory', { region: fresh.regions[0].id, addr: 240, length: 1 })).hex, 'f0');
  await mcp.tool('pause');
  const exact = await mcp.tool('run_frames', { n: 2 });
  assert.equal(exact.stop_reason, 'frames_done'); assert.equal(exact.elapsed_frames, 2);
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Resume'"));
  const pendingOperation = mcp.tool('run_frames', { n: 1000000, max_seconds: 10 });
  await until(async () => (await sessions.request('session.status')).result.operation_id);
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Pause'"));
  await window.webContents.executeJavaScript("document.getElementById('pause').click()");
  assert.equal((await pendingOperation).stop_reason, 'paused');
  // Renderer reload must preserve an external owner's operation.
  const longRun = mcp.tool('run_frames', { n: 1000000, max_seconds: 10 });
  const activeId = await until(async () => (await sessions.request('session.status')).result.operation_id);
  const reloaded = once(window.webContents, 'did-finish-load'); window.webContents.reload(); await reloaded;
  assert.equal((await sessions.request('session.status')).result.operation_id, activeId);
  assert.equal((await mcp.tool('cancel_operation', { operation_id: activeId })).stop_reason, 'cancelled');
  assert.equal((await longRun).stop_reason, 'cancelled');
  const revoked = mcp.tool('run_frames', { n: 1000000, max_seconds: 10 });
  await until(async () => (await sessions.request('session.status')).result.operation_id);
  await window.webContents.executeJavaScript("document.getElementById('synthetic').click()");
  await until(async () => (await sessions.request('session.status')).session !== fresh.session);
  assert.ok(['stale_session', 'native_host_disconnected'].includes((await revoked).error));
  await mcp.tool('emu_status');
  assert.equal((await mcp.tool('operation_status', { operation_id: exact.operation_id })).error, 'stale_session');
  await stopNative();
  assert.equal((await mcp.tool('pause')).error, 'native_host_disconnected');
  for (const p of peers) assert.equal((await p.close())[0], 0, p.stderr);
  console.log('MCP_INTEGRATION_PASS: real stdio, shared paused GB WRAM bytes, renderer reload, instance/session errors, native disconnect, bundled Electron Node runtime');
  app.quit();
}).catch(async error => { console.error(error); for (const p of peers) await p.close(); app.exit(1); });
