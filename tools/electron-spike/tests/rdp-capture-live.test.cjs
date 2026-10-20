'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost } = require('../dist/services/local-endpoint.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const fixture = process.env.ARES_CPU_FIXTURE || path.join(path.dirname(host), 'ares-electron-cpu-fixture');
const identity = { session: 'machine-live', epoch: 1 }, empty = { gpr: [], pc: null, hi: null, lo: null, status: null, llbit: null };
const unwrap = r => { assert.equal(r.ok, true, JSON.stringify(r)); return r.result; };
const sleep = ms => new Promise(r => setTimeout(r, ms));
test('real N64 exact RDP capture: Vulkan submissions, task snapshots, bounded pages and atomic export', { timeout: 30000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-rdp-live-'), image = Buffer.alloc(0x1000);
  image.writeUInt32BE(0x80371240); image.write('MACHINE FIXTURE', 0x20); image[0x3e] = 0x45;
  image.writeUInt32BE(0x24020007, 0x80); image.writeUInt32BE(0x1000ffff, 0x84); image.writeUInt32BE(0, 0x88);
  const rom = path.join(dir, 'fixture.z64'), settings = path.join(dir, 'settings.bml'); fs.writeFileSync(rom, image); fs.writeFileSync(settings, '');
  const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--settings', settings, '--silent', '--paused', '--save-dir', path.join(dir, 'saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log = (log + b).slice(-20000)); child.stdout.resume();
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {}), router = new SessionRouter(); router.attach(native);
  const endpoint = new LocalServiceHost(router); let peer;
  try {
    unwrap(await native.request('session.status')); await endpoint.listen(path.join(dir, 'ipc'));
    peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); await peer.initialize();
    const code = [0x3c08a440];
    for (const [offset, value] of [[4, 0x2000], [8, 4], [24, 525], [28, 3093], [32, (3093 << 16) | 3093], [36, (108 << 16) | 116], [40, (34 << 16) | 38], [48, 1024], [52, 1024], [0, 0x302]]) {
      code.push((0x3c090000 | value >>> 16) >>> 0, (0x35290000 | value & 0xffff) >>> 0, (0xad090000 | offset) >>> 0);
    }
    // Continuously submit two original no-op words from DMEM, then clear RSP
    // HALT (its IMEM starts with BREAK). Capture must precede Vulkan dispatch.
    const loop = code.length;
    code.push(0x3c08a410, 0x24090002, 0xad09000c, 0x24090300, 0xad090000, 0x24090310, 0xad090004,
      0x3c08a404, 0x24090001, 0xad090010, (0x10000000 | ((-11) & 0xffff)) >>> 0, 0);
    const bytes = Buffer.alloc(code.length * 4); code.forEach((w, i) => bytes.writeUInt32BE(w, i * 4));
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: 0x80, hex_bytes: bytes.toString('hex') });
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: 0x300, hex_bytes: '00112233445566770077665544332211' });
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: 0xfc0, hex_bytes: 'ab' });
    await peer.tool('write_memory', { region: 'RSP IMEM', addr: 0, hex_bytes: '0000000d' });
    await peer.tool('cpu_set_registers', { pc: '0xa4000080' });
    const output = path.join(dir, 'capture.json');
    const capture = await peer.tool('rdp_capture', { max_commands: 8, dmem_snapshots: true, max_seconds: 2, path: output, limit: 8 });
    assert.equal(capture.error, undefined, JSON.stringify(capture));
    assert.equal(capture.complete, true, JSON.stringify(capture)); assert.equal(capture.elapsed_frames, 1);
    assert.equal(capture.words.length, 8); assert.equal(capture.truncated, true); assert.ok(capture.dropped_words > 0);
    assert.equal(capture.words[0].value, '0x11223344556677'); assert.equal(capture.words[1].value, '0x77665544332211');
    const file = JSON.parse(fs.readFileSync(output, 'utf8'));
    assert.equal(file.words.length, 8); assert.ok(file.batches.length > 0); assert.ok(file.tasks.length > 0);
    assert.equal(file.batches[0].source, 'DMEM'); assert.equal(file.tasks[0].header.slice(0, 2), 'ab'); assert.equal(file.tasks[0].dmem.slice(0xfc0 * 2, 0xfc0 * 2 + 2), 'ab');
    assert.equal(capture.artifact.format, 'ares-rdp-capture-v1');
    const oldId = capture.operation_id;
    await peer.tool('run_frames', { n: 1 });
    assert.equal(unwrap(await native.request('n64.rdp.capturePage', undefined, { operationId: oldId, kind: 'words', offset: 0, limit: 64 })).complete, true, 'capture completion is immutable after later VI');
    assert.equal((await peer.tool('rdp_capture', { path: output, max_commands: 2, max_seconds: 2 })).file_error, 'file_exists');
    assert.deepEqual(JSON.parse(fs.readFileSync(output, 'utf8')), file, 'failed overwrite leaves artifact intact');
    assert.equal((await native.request('n64.rdp.capturePage', undefined, { operationId: oldId, kind: 'words', offset: 0, limit: 64 })).error, 'stale_capture');
    assert.equal((await peer.tool('rdp_capture', { next_frame: false })).error, 'legacy_capture_unavailable');
    // A CPU step leaves the VI boundary; exact capture rejects without running.
    await peer.tool('step_instructions');
    assert.equal((await peer.tool('rdp_capture', { max_seconds: 0.01 })).error, 'not_at_frame_boundary');
    await peer.close(); peer = undefined;
    await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); await endpoint.close(); router.detach(); native.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
