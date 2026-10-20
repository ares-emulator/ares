import { fileURLToPath } from 'node:url';
import { app, BrowserWindow, dialog, nativeImage } from 'electron';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { once } from 'node:events';
import { makeFixture } from '../fixture.cjs';
import { StdioPeer } from './stdio-peer.cjs';
import { LocalServiceClient } from '../dist/services/local-endpoint.js';
const __dirname = path.dirname(fileURLToPath(import.meta.url));
const temporary = fs.mkdtempSync('/tmp/ares-mcp-');
app.setPath('userData', path.join(temporary, 'profile'));
const rom = path.join(temporary, 'fixture.gb'), settings = path.join(temporary, 'settings.bml');
fs.writeFileSync(rom, makeFixture()); fs.writeFileSync(settings, '');
Object.assign(process.env, { ARES_ROM: rom, ARES_SYSTEM: 'Game Boy', ARES_SILENT: '1', ARES_SETTINGS: settings, ARES_SAVE_DIR: path.join(temporary, 'saves'), ARES_ENABLE_SERVICES: '1', ARES_SERVICE_DIR: path.join(temporary, 'ipc') });
delete process.env.ARES_AUTOTEST;
const { sessions, serviceHost, stopNative } = await import('../dist/main.js');
const peers = [];
async function until(fn) { for (let i = 0; i < 200; ++i) { try { const result = await fn(); if (result) return result; } catch {} await new Promise(r => setTimeout(r, 25)); } throw new Error('Integration timed out'); }
function peer(instance = serviceHost.instance) {
  const result = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', serviceHost.metadataFile, '--instance', instance], { env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '' } });
  peers.push(result); return result;
}
let stage = 'startup';
const testStarted = performance.now();
const enter = value => { stage = value; console.log(`INTEGRATION_STAGE: ${((performance.now() - testStarted) / 1000).toFixed(1)}s ${value}`); };
const timeout = setTimeout(() => { console.error(`MCP integration timeout: ${stage}`); app.exit(1); }, 120000);
app.on('will-quit', () => { clearTimeout(timeout); fs.rmSync(temporary, { recursive: true, force: true }); });
app.whenReady().then(async () => {
  enter('native startup');
  const initial = await until(() => sessions.request('session.status'));
  assert.equal(initial.result.system, 'Game Boy');
  await until(() => serviceHost.metadataFile);
  enter('renderer startup');
  const window = BrowserWindow.getAllWindows()[0];
  await until(() => window.webContents.executeJavaScript("document.getElementById('memory-region')?.options.length > 0"));
  enter('MCP initialize');
  const mcp = peer(); await mcp.initialize();
  enter('MCP tools');
  const list = (await mcp.request('tools/list', {})).result.tools;
  assert.deepEqual(list.map(t => t.name).sort(), ['emu_status', 'pause', 'resume', 'memory_regions', 'read_memory', 'write_memory', 'run_frames', 'run_until_frame', 'step', 'operation_status', 'cancel_operation', 'cpu_registers', 'last_stop', 'step_instructions', 'run_until_hit', 'break_add', 'break_remove', 'break_list', 'watch_add', 'watch_remove', 'watch_list', 'read_memory_va', 'write_memory_va', 'controller_set', 'controller_release', 'controller_state', 'controller_host_mode', 'trace_pc', 'trace_calls', 'trace_page', 'overlay_log', 'profiler_control', 'profiler_clear', 'cpu_profiler_top', 'controller_tap', 'screenshot'].sort());
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
  enter('shared GB inspector memory');
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
  enter('native presented screenshot');
  const imageReply = await mcp.request('tools/call', { name: 'screenshot', arguments: { source: 'presented', region: [0, 0, 4, 4], scale: 2, inline: true } });
  assert.equal(imageReply.result.isError, undefined);
  const imageContent = imageReply.result.content.find(c => c.type === 'image');
  const imageMetadata = JSON.parse(imageReply.result.content.find(c => c.type === 'text').text);
  assert.equal(imageContent.mimeType, 'image/png'); assert.equal(imageMetadata.epoch, initial.epoch);
  assert.deepEqual(nativeImage.createFromBuffer(Buffer.from(imageContent.data, 'base64')).getSize(), { width: 8, height: 8 });
  enter('paused reload');
  const loaded = once(window.webContents, 'did-finish-load'); window.webContents.reload(); await loaded;
  assert.equal((await mcp.tool('emu_status')).session, initial.session);
  assert.equal((await mcp.tool('read_memory', { region: ram.id, addr: 240, length: 4 })).hex, 'deadbeef');
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: 240, hex_bytes: baseline.hex })).written, 4);
  enter('wrong instance and reconnect');
  const wrong = peer('wrong-instance'); await wrong.initialize(); assert.equal((await wrong.tool('emu_status')).error, 'wrong_instance');
  assert.equal((await mcp.tool('resume', { keep_overrides: true })).error, 'unavailable');
  assert.equal((await mcp.tool('step')).error, 'unavailable');
  const reconnect = new LocalServiceClient(serviceHost.metadataFile, serviceHost.instance);
  assert.equal((await reconnect.request('session.status')).session, initial.session);
  reconnect.disconnect(); assert.equal((await reconnect.request('session.status')).session, initial.session); reconnect.close();
  enter('reset and ROM revocation');
  await sessions.request('session.reset');
  assert.equal((await mcp.tool('screenshot', { source: 'presented' })).error, 'no_frame');
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: 240, hex_bytes: '00' })).error, 'stale_epoch');
  assert.equal((await mcp.tool('memory_regions')).epoch, 2);
  await window.webContents.executeJavaScript("document.getElementById('synthetic').click()");
  await until(async () => (await sessions.request('session.status')).session !== initial.session);
  assert.equal((await mcp.tool('write_memory', { region: ram.id, addr: 240, hex_bytes: '00' })).error, 'stale_session');
  const fresh = await mcp.tool('memory_regions');
  assert.equal((await mcp.tool('read_memory', { region: fresh.regions[0].id, addr: 240, length: 1 })).hex, 'f0');
  enter('owned execution and UI pause');
  await mcp.tool('pause');
  enter('owned run_frames');
  const exact = await mcp.tool('run_frames', { n: 2 });
  assert.equal(exact.stop_reason, 'frames_done'); assert.equal(exact.elapsed_frames, 2);
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Resume'"));
  enter('UI pause cancels operation');
  const pendingOperation = mcp.tool('run_frames', { n: 1000000, max_seconds: 10 });
  await until(async () => (await sessions.request('session.status')).result.operation_id);
  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Pause'"));
  await window.webContents.executeJavaScript("document.getElementById('pause').click()");
  assert.equal((await pendingOperation).stop_reason, 'paused');
  // Renderer reload must preserve an external owner's operation.
  enter('reload during owned operation');
  const longRun = mcp.tool('run_frames', { n: 1000000, max_seconds: 10 });
  const activeId = await until(async () => (await sessions.request('session.status')).result.operation_id);
  const reloaded = once(window.webContents, 'did-finish-load'); window.webContents.reload(); await reloaded;
  assert.equal((await sessions.request('session.status')).result.operation_id, activeId);
  assert.equal((await mcp.tool('cancel_operation', { operation_id: activeId })).stop_reason, 'cancelled');
  assert.equal((await longRun).stop_reason, 'cancelled');
  enter('replacement during owned operation');
  const revoked = mcp.tool('run_frames', { n: 1000000, max_seconds: 10 });
  await until(async () => (await sessions.request('session.status')).result.operation_id);
  await window.webContents.executeJavaScript("document.getElementById('synthetic').click()");
  await until(async () => (await sessions.request('session.status')).session !== fresh.session);
  assert.ok(['stale_session', 'native_host_disconnected'].includes((await revoked).error));
  await mcp.tool('emu_status');
  assert.equal((await mcp.tool('operation_status', { operation_id: exact.operation_id })).error, 'stale_session');
  enter('N64 provenance opt-in and stopped UI');
  const n64Image = Buffer.alloc(0x1000); n64Image.writeUInt32BE(0x80371240); n64Image.write('DMA UI FIXTURE', 0x20); n64Image[0x3e] = 0x45;
  const n64File = path.join(temporary, 'fixture.z64'); fs.writeFileSync(n64File, n64Image);
  const showOpenDialog = dialog.showOpenDialog;
  dialog.showOpenDialog = async () => ({ canceled: false, filePaths: [n64File] });
  try { await window.webContents.executeJavaScript('window.ares.open({system: "Nintendo 64"})'); }
  finally { dialog.showOpenDialog = showOpenDialog; }
  await until(async () => (await sessions.request('session.status')).result.system === 'Nintendo 64');
  await mcp.tool('emu_status'); assert.equal((await mcp.tool('pause')).paused, true);
  assert.equal((await mcp.tool('overlay_log')).history_complete, true, 'services-enabled app records provenance from boot');
  assert.equal((await mcp.tool('profiler_control')).enabled, true);
  assert.equal((await mcp.tool('step_instructions')).executed, 1);
  assert.equal((await mcp.tool('cpu_profiler_top')).timebase_hz, 187500000);
  assert.equal((await mcp.tool('profiler_clear')).cleared, true);
  assert.equal((await mcp.tool('profiler_control', { enable: false })).enabled, false);
  const tap = await mcp.tool('controller_tap', { button: 'a', max_seconds: 0.01 });
  assert.equal(tap.stop_reason, 'timeout'); assert.equal(tap.released, true);
  assert.equal((await mcp.tool('controller_state')).ports[0].pending.overridden, false);
  const tracePath = path.join(temporary, 'electron trace.json');
  const exportedTrace = await mcp.tool('trace_pc', { max_seconds: 0.01, max_records: 4, inline_limit: 0, path: tracePath });
  assert.equal(exportedTrace.path, tracePath); assert.equal(exportedTrace.entries.length, 0);
  assert.equal(JSON.parse(fs.readFileSync(tracePath, 'utf8')).entries.length, exportedTrace.total_entries);
  assert.ok(exportedTrace.artifact.bytes > 0);

  await until(() => window.webContents.executeJavaScript("document.getElementById('pause').textContent === 'Resume'"));
  enter('native shutdown');
  await stopNative();
  assert.equal((await mcp.tool('pause')).error, 'native_host_disconnected');
  for (const p of peers) assert.equal((await p.close())[0], 0, p.stderr);
  console.log('MCP_INTEGRATION_PASS: real stdio, shared paused GB WRAM bytes, renderer reload, instance/session errors, native disconnect, bundled Electron Node runtime');
  app.quit();
}).catch(async error => { console.error(error); for (const p of peers) await p.close(); app.exit(1); });
