'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const { StdioPeer } = require('./stdio-peer.cjs');
const { writeStateFixture } = require('./state-file-fixture.cjs');
const { makeFixture } = require('../fixture.cjs');
async function run() {
  const source = path.resolve(process.argv[2] || path.join(__dirname, '../artifacts/distribution/Ares.app'));
  const temporary = fs.mkdtempSync('/tmp/ares-package-');
  const bundle = path.join(temporary, 'Ares.app');
  fs.cpSync(source, bundle, { recursive: true, verbatimSymlinks: true, mode: fs.constants.COPYFILE_FICLONE });
  const resources = path.join(bundle, 'Contents/Resources');
  const rom = path.join(temporary, 'fixture.gb'), report = path.join(temporary, 'renderer.json');
  fs.writeFileSync(rom, makeFixture());
  const settings = path.join(temporary, 'settings.bml'); fs.writeFileSync(settings, '');
  const env = { ...process.env, PATH: '/usr/bin:/bin', NODE_OPTIONS: '', NODE_PATH: '', ARES_ROM: rom, ARES_SYSTEM: 'Game Boy', ARES_SETTINGS: settings, ARES_SAVE_DIR: path.join(temporary, 'saves'), ARES_ENABLE_SERVICES: '1', ARES_SERVICE_DIR: path.join(temporary, 'ipc'), ARES_SILENT: '1', ARES_AUTOTEST: '1', ARES_TEST_SECONDS: '8', ARES_REPORT: report };
  delete env.ARES_HOST; delete env.ELECTRON_RUN_AS_NODE;
  // This executable, launcher, JS graph, host and dynamic libraries all come from
  // the copied distribution. PATH deliberately has no developer Node/npm/Python.
  const checkout = path.resolve(__dirname, '../../..');
  const profile = `(version 1)(allow default)(deny file-read* (subpath ${JSON.stringify(checkout)}))`;
  const confined = (command, args) => ['/usr/bin/sandbox-exec', ['-p', profile, command, ...args]];
  // Seatbelt cannot nest Chromium's own sandbox. Constrain the plain native
  // preflight and Node adapter; keep the GUI's normal renderer sandbox enabled.
  execFileSync('/usr/bin/sandbox-exec', ['-p', profile, path.join(resources, 'native/bin/ares-electron-host'), '--rom', rom, '--system', 'Game Boy', '--settings', settings, '--save-dir', path.join(temporary, 'preflight/saves'), '--silent', '--paused'], { cwd: temporary, env, input: 'quit\n', timeout: 10000 });
  const child = spawn(path.join(bundle, 'Contents/MacOS/Electron'), [`--user-data-dir=${path.join(temporary, 'profile')}`], { cwd: temporary, env });
  const closed = once(child, 'close'); let diagnostics = ''; child.stdout.resume(); child.stderr.on('data', b => diagnostics += b);
  const timeout = setTimeout(() => child.kill('SIGKILL'), 20000); let peer;
  try {
    let file;
    for (let i = 0; i < 300 && !file; ++i) {
      if (fs.existsSync(env.ARES_SERVICE_DIR)) file = fs.readdirSync(env.ARES_SERVICE_DIR).find(f => f.endsWith('.json'));
      if (!file) await new Promise(resolve => setTimeout(resolve, 20));
    }
    assert.ok(file, diagnostics);
    file = path.join(env.ARES_SERVICE_DIR, file);
    const info = JSON.parse(fs.readFileSync(file));
    const [mcpCommand, mcpArgs] = confined(path.join(resources, 'ares-mcp'), ['--connection', file, '--instance', info.instance]);
    peer = new StdioPeer(mcpCommand, mcpArgs, { cwd: temporary, env });
    await peer.initialize();
    const inventory = (await peer.request('tools/list', {})).result.tools.map(t => t.name);
    assert.equal(inventory.length, 59);
    for (const name of ['profiler_control', 'profiler_clear', 'cpu_profiler_top', 'controller_tap', 'screenshot', 'audio_capture', 'audio_status', 'audio_save', 'video_start', 'video_status', 'video_stop', 'state_info', 'state_list', 'state_peek', 'state_save', 'state_load', 'batch', 'cpu_set_registers', 'rsp_registers', 'rdp_state', 'peripheral_registers', 'disassemble', 'rsp_debug', 'rsp_break', 'rsp_step', 'rsp_run', 'rsp_stop']) assert.ok(inventory.includes(name));
    let status;
    for (let i = 0; i < 200; ++i) { status = await peer.tool('emu_status'); if (status.system === 'Game Boy') break; await new Promise(r => setTimeout(r, 20)); }
    assert.equal(status.system, 'Game Boy', diagnostics);
    for (const [tool, args] of [['rsp_debug', {}], ['rsp_break', {}], ['rsp_step', {}], ['rsp_run', {}], ['rsp_stop', {}], ['cpu_set_registers', { gpr: { t0: 1 } }], ['rsp_registers', {}], ['rdp_state', {}], ['peripheral_registers', {}], ['disassemble', { addr: 0, count: 1 }]]) assert.equal((await peer.tool(tool, args)).error, 'unavailable');
    assert.equal((await peer.tool('batch', { steps: [{ kind: 'run', n: 0 }] })).error, 'unavailable');
    assert.ok(status.capabilities.includes('artifact.traceExport'), 'packaged main advertises artifact service');
    const offlineFile = path.join(temporary, 'offline fixture.bs'); writeStateFixture(offlineFile);
    assert.equal((await peer.tool('state_save', { path: path.join(temporary, 'unsupported.gb.bs') })).error, 'unavailable');
    assert.equal((await peer.tool('state_load', { path: offlineFile })).error, 'unavailable');
    const pausedBeforeInspection = (await peer.tool('emu_status')).paused;
    assert.equal((await peer.tool('state_info', { path: offlineFile })).compatibility, 'header_supported_configuration_unverified');
    assert.equal((await peer.tool('state_peek', { path: offlineFile, addr: '0x80000001', length: 7 })).hex, '41424344454647');
    assert.ok((await peer.tool('state_list', { directory: temporary })).entries.some(e => e.path === offlineFile));
    assert.equal((await peer.tool('emu_status')).paused, pausedBeforeInspection);
    assert.equal((await peer.tool('pause')).paused, true);
    const regions = await peer.tool('memory_regions');
    const region = regions.regions.find(r => r.writable && r.readable && r.name.includes('RAM'));
    assert.ok(region, JSON.stringify(regions));
    const offset = region.size - 4;
    const before = await peer.tool('read_memory', { region: region.id, addr: offset, length: 4 });
    try {
      assert.equal((await peer.tool('write_memory', { region: region.id, addr: offset, hex_bytes: '12345678' })).written, 4);
      assert.equal((await peer.tool('read_memory', { region: region.id, addr: offset, length: 4 })).hex, '12345678');
    } finally { assert.equal((await peer.tool('write_memory', { region: region.id, addr: offset, hex_bytes: before.hex })).written, 4); }
    const pngFile = path.join(temporary, 'packaged screenshot.png');
    const capture = await peer.request('tools/call', { name: 'screenshot', arguments: { source: 'presented', path: pngFile, region: [0, 0, 2, 2], inline: true } });
    assert.equal(capture.result.isError, undefined);
    const image = capture.result.content.find(c => c.type === 'image');
    assert.equal(image.mimeType, 'image/png');
    const png = Buffer.from(image.data, 'base64');
    assert.equal(png.subarray(0, 8).toString('hex'), '89504e470d0a1a0a');
    assert.deepEqual(fs.readFileSync(pngFile), png, 'packaged worker and file publisher agree with inline image');
    const wavFile = path.join(temporary, 'packaged audio.wav');
    assert.equal((await peer.tool('audio_capture', { seconds: 0 })).cap, 0);
    const wav = await peer.tool('audio_save', { path: wavFile });
    assert.equal(wav.bytes, 44); assert.equal(wav.frames, 0);
    assert.equal(fs.readFileSync(wavFile).toString('ascii', 0, 12), 'RIFF$\0\0\0WAVE');
    assert.equal((await peer.tool('audio_status')).active, false);
    assert.equal((await peer.tool('video_start', { max_frames: 2 })).active, true);
    assert.equal((await peer.tool('resume')).paused, false);
    for (let n = 0; n < 100 && (await peer.tool('video_status')).frames < 2; n++) await new Promise(r => setTimeout(r, 10));
    assert.equal((await peer.tool('pause')).paused, true);
    const movieFile = path.join(temporary, 'packaged video.avi');
    const movie = await peer.tool('video_stop', { path: movieFile });
    assert.equal(movie.frames, 2); assert.equal(movie.frame_kind, 'unavailable');
    assert.equal(fs.readFileSync(movieFile).toString('ascii', 8, 12), 'AVI ');
    assert.equal((await peer.tool('resume')).paused, false);
    assert.equal((await closed)[0], 0, diagnostics);
    assert.ok(['app_not_running', 'app_disconnected'].includes((await peer.tool('emu_status')).error));
    assert.equal((await peer.close())[0], 0); peer = undefined;
    const measurement = JSON.parse(fs.readFileSync(report));
    assert.ok(measurement.frames > 5); assert.equal(measurement.glError, 0);
    assert.equal(fs.existsSync(file), false, 'Connection metadata removed on quit');
    const rpaths = execFileSync('/usr/bin/otool', ['-l', path.join(resources, 'native/bin/ares-electron-host')], { encoding: 'utf8' });
    assert.ok(!/path \/Users\//.test(rpaths), 'No developer rpath');
    const runtime = JSON.parse(fs.readFileSync(path.join(resources, 'runtime.json')));
    const result = { result: 'PACKAGE_MCP_PASS', runtime, system: status.system, frames: measurement.frames, protocol: 'real MCP stdio', checkoutReadDeniedFor: ['native loader', 'MCP adapter'] };
    fs.writeFileSync(path.resolve(__dirname, '../artifacts/package-mcp.json'), JSON.stringify(result, null, 2));
    console.log(result);
  } finally {
    clearTimeout(timeout); if (peer) await peer.close();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(temporary, { recursive: true, force: true });
  }
}
run().catch(error => { console.error(error); process.exitCode = 1; });
