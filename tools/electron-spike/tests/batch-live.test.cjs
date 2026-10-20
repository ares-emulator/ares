'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const { inflateSync } = require('node:zlib');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost, LocalServiceClient } = require('../dist/services/local-endpoint.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const sleep = ms => new Promise(r => setTimeout(r, ms));
const unwrap = r => { assert.equal(r.ok, true, JSON.stringify(r)); return r.result; };
async function waitRun(native) {
  const deadline = Date.now() + 5000;
  while (Date.now() < deadline) { const reply = await native.request('session.status'); if (!reply.ok && reply.error === 'stale_epoch') continue; if (unwrap(reply).operation_id) return; await sleep(5); }
  throw new Error('batch child did not start');
}
function imageFixture() {
  // Original hand-assembled IPL3 fixture configures a 4x2 RGBA5551 VI and loops.
  // It does not use a commercial ROM, game data or a user emulator instance.
  const image = Buffer.alloc(0x1000); image.writeUInt32BE(0x80371240); image.write('BATCH VI FIXTURE', 0x20); image[0x3e] = 0x45;
  const code = [0x3c08a440]; // lui t0, 0xa440
  for (const [offset, value] of [[4, 0x2000], [8, 4], [24, 525], [28, 3093], [32, (3093 << 16) | 3093], [36, (108 << 16) | 116], [40, (34 << 16) | 38], [48, 1024], [52, 1024], [0, 0x302]]) {
    code.push((0x3c090000 | value >>> 16) >>> 0, (0x35290000 | value & 0xffff) >>> 0, (0xad090000 | offset) >>> 0);
  }
  code.push(0x1000ffff, 0); // beq zero, zero, self; nop
  code.forEach((word, i) => image.writeUInt32BE(word, 0x40 + i * 4)); return { image, instructions: code.length };
}
function pngPixels(file) {
  const bytes = fs.readFileSync(file), chunks = []; let width, height;
  assert.equal(bytes.subarray(0, 8).toString('hex'), '89504e470d0a1a0a');
  for (let at = 8; at < bytes.length;) {
    const length = bytes.readUInt32BE(at), type = bytes.toString('ascii', at + 4, at + 8), data = bytes.subarray(at + 8, at + 8 + length);
    if (type === 'IHDR') { width = data.readUInt32BE(0); height = data.readUInt32BE(4); }
    if (type === 'IDAT') chunks.push(data); at += 12 + length;
  }
  const raw = inflateSync(Buffer.concat(chunks)), rgb = [];
  for (let y = 0; y < height; y++) { assert.equal(raw[y * (width * 3 + 1)], 0); rgb.push(...raw.subarray(y * (width * 3 + 1) + 1, (y + 1) * (width * 3 + 1))); }
  return { width, height, rgb };
}
test('real N64 batch: baseline/load/write/active-VI-run/crop sweep, partial effects, ownership and revocation', { timeout: 90000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-batch-live-'), identity = { session: 'batch-live', epoch: 1 }, { image, instructions } = imageFixture();
  const rom = path.join(dir, 'fixture.z64'), settings = path.join(dir, 'settings.bml'); fs.writeFileSync(rom, image); fs.writeFileSync(settings, '');
  const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--settings', settings, '--silent', '--paused', '--save-dir', path.join(dir, 'saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log = (log + b).slice(-20000)); child.stdout.resume();
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {}), router = new SessionRouter(); router.attach(native);
  const endpoint = new LocalServiceHost(router); let peer, other;
  try {
    unwrap(await native.request('session.status')); await endpoint.listen(path.join(dir, 'ipc'));
    peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); await peer.initialize();
    other = new LocalServiceClient(endpoint.metadataFile, endpoint.instance); unwrap(await other.request('session.status'));
    const stepped = await peer.tool('step_instructions', { n: instructions }); assert.equal(stepped.stop_reason, 'instructions_done', JSON.stringify(stepped));
    const rawFile = path.join(dir, 'raw.bs'); assert.equal((await peer.tool('state_save', { path: rawFile })).error, undefined);
    const layout = unwrap(await native.request('n64.state.layout')), initialized = fs.readFileSync(rawFile);
    initialized[layout.offset + layout.size] = 1; // serialized RDRAM identity mapping, fixture only
    const initializedFile = path.join(dir, 'initialized.bs'); fs.writeFileSync(initializedFile, initialized);
    assert.equal((await peer.tool('state_load', { path: initializedFile })).error, undefined);
    const baseline = path.join(dir, 'baseline.bs'); assert.equal((await peer.tool('state_save', { path: baseline })).error, undefined);
    for (const [candidate, hex, rgb] of [['red', 'f801', [255, 0, 0]], ['green', '07c1', [0, 255, 0]]]) {
      const output = path.join(dir, `${candidate}.png`);
      const result = await peer.tool('batch', { steps: [
        { kind: 'state_load', path: baseline }, { kind: 'write', addr: '0x2000', hex: hex.repeat(8), view: 'rdram' },
        { kind: 'run', n: 2 }, { kind: 'screenshot', path: output, region: [1, 0, 2, 2], scale: 2 },
        { kind: 'read', addr: '0x2000', length: 16 }
      ] });
      assert.equal(result.error, undefined, JSON.stringify(result)); assert.equal(result.stop_reason, 'batch_done', JSON.stringify(result)); assert.equal(result.steps_completed, 5);
      assert.equal(result.side_effects_rolled_back, false); assert.equal(result.results[2].data.elapsed_frames, 2); assert.equal(result.results[2].data.stop_reason, 'frames_done');
      assert.equal(result.results[4].data.hex, hex.repeat(8));
      const pixels = pngPixels(output); assert.equal(pixels.width, 4); assert.equal(pixels.height, 4); assert.deepEqual(pixels.rgb, Array.from({ length: 16 }, () => rgb).flat());
    }
    const beforeOffline = unwrap(await native.request('n64.debug.registers')), epochBeforeOffline = native.identity.epoch;
    const offline = await peer.tool('state_peek', { path: baseline, addr: '0x2000', length: 16 });
    assert.equal(offline.hex, '00'.repeat(16)); assert.equal(native.identity.epoch, epochBeforeOffline);
    assert.deepEqual(unwrap(await native.request('n64.debug.registers')), beforeOffline);
    const incompatible = path.join(dir, 'incompatible.bs'), incompatibleBytes = fs.readFileSync(baseline); incompatibleBytes.write('v999', 5); fs.writeFileSync(incompatible, incompatibleBytes);
    const incompatibleBatch = await peer.tool('batch', { steps: [{ kind: 'state_load', path: incompatible }, { kind: 'write', addr: '0x2000', hex: 'ff', view: 'rdram' }] });
    assert.equal(incompatibleBatch.stop_reason, 'step_error'); assert.equal(incompatibleBatch.results[0].error, 'unsupported_version'); assert.equal(incompatibleBatch.steps_completed, 0); assert.equal(incompatibleBatch.steps_attempted, 1);
    assert.equal(native.identity.epoch, epochBeforeOffline);
    const failed = await peer.tool('batch', { steps: [{ kind: 'write', addr: '0x2000', hex: 'aa', view: 'rdram' }, { kind: 'read', addr: '0x7fffff', length: 2 }, { kind: 'write', addr: '0x2000', hex: 'bb', view: 'rdram' }] });
    assert.equal(failed.stop_reason, 'step_error'); assert.equal(failed.results.length, 2); assert.equal(failed.results[1].error, 'out_of_bounds');
    assert.equal((await peer.tool('read_memory_va', { addr: '0x2000', length: 1 })).hex, 'aa');
    const keptGoing = await peer.tool('batch', { stop_on_error: false, steps: [{ kind: 'state_load', path: path.join(dir, 'missing.bs') }, { kind: 'read', addr: '0x2000', length: 1 }] });
    assert.equal(keptGoing.stop_reason, 'batch_done'); assert.equal(keptGoing.results[0].ok, false); assert.equal(keptGoing.results[1].data.hex, 'aa');
    const timed = await peer.tool('batch', { max_seconds: 0.05, steps: [{ kind: 'run', n: 1000000 }, { kind: 'write', addr: '0x2000', hex: 'bb', view: 'rdram' }] });
    assert.equal(timed.stop_reason, 'timeout', JSON.stringify(timed)); assert.equal(timed.results.length, 1); assert.equal(timed.results[0].data.state, 'done'); assert.ok(timed.results[0].data.elapsed_frames < 1000000);
    // Interrupt a long run through UI; foreign mutations must remain excluded.
    const pending = peer.tool('batch', { steps: [{ kind: 'run', n: 1000000 }, { kind: 'write', addr: '0x2000', hex: 'bb', view: 'rdram' }] });
    await waitRun(native);
    const activeBeforeOldCancel = unwrap(await native.request('session.status')).operation_id;
    await peer.tool('cancel_operation', { operation_id: timed.results[0].data.operation_id });
    assert.equal(unwrap(await native.request('session.status')).operation_id, activeBeforeOldCancel, 'cancelling an immutable old result must not revoke the new batch');
    unwrap(await other.request('session.status'));
    assert.equal((await other.request('n64.memory.write', undefined, { address: '0x2000', view: 'rdram', hex: 'cc' })).error, 'busy');
    unwrap(await native.request('session.pause'));
    const paused = await pending; assert.equal(paused.stop_reason, 'paused', JSON.stringify(paused)); assert.equal(paused.results.length, 1); assert.equal(paused.results[0].data.stop_reason, 'paused');
    assert.equal((await peer.tool('read_memory_va', { addr: '0x2000', length: 1 })).hex, 'aa');
    // The authenticated parent can cancel its active child through the existing tool.
    const cancelledBatch = peer.tool('batch', { steps: [{ kind: 'state_load', path: baseline }, { kind: 'write', addr: '0x2000', hex: 'aa', view: 'rdram' }, { kind: 'run', n: 1000000 }, { kind: 'write', addr: '0x2000', hex: 'bb', view: 'rdram' }] });
    cancelledBatch.catch(() => {});
    await waitRun(native);
    const cancel = await peer.tool('cancel_operation'); assert.equal(cancel.stop_reason, 'cancelled', JSON.stringify(cancel));
    const cancelled = await cancelledBatch; assert.equal(cancelled.stop_reason, 'cancelled'); assert.equal(cancelled.results.length, 3);
    // Human takeover revokes reservation even without an input override.
    const human = peer.tool('batch', { steps: [{ kind: 'run', n: 1000000 }, { kind: 'write', addr: '0x2000', hex: 'bb', view: 'rdram' }] });
    await waitRun(native);
    child.stdin.write('key A 1\n'); const taken = await human; assert.equal(taken.stop_reason, 'host_input', JSON.stringify(taken)); assert.equal(taken.results.length, 1);
    child.stdin.write('key A 0\n'); unwrap(await native.request('session.pause'));
    // Disconnect a parent connection while its separately owned child is running.
    unwrap(await other.request('session.status'));
    const disconnected = other.request('experiment.batch', undefined, { steps: [{ kind: 'run', n: 1000000 }, { kind: 'write', address: '0x2000', hex: 'dd', view: 'rdram' }], timeoutMs: 20000, stopOnError: true });
    const rejected = assert.rejects(disconnected, /app_disconnected/);
    await waitRun(native);
    other.disconnect(); await rejected;
    for (let i = 0; i < 100; i++) { const status = unwrap(await native.request('session.status')); if (!status.batch_id && !status.operation_id) break; await sleep(5); }
    const afterDisconnect = unwrap(await native.request('session.status')); assert.equal(afterDisconnect.batch_id, null); assert.equal(afterDisconnect.operation_id, null); assert.equal(afterDisconnect.paused, true);
    assert.equal((await peer.tool('read_memory_va', { addr: '0x2000', length: 1 })).hex, 'aa');
    // The native lease also excludes mutations in an idle file-I/O gap.
    const idle = unwrap(await native.request('execution.batchBegin', undefined, { timeoutMs: 1000 }, 'batch-idle'));
    assert.equal(unwrap(await native.request('session.status')).batch_id, idle.batch_id);
    assert.equal((await native.request('n64.memory.write', undefined, { address: '0x2000', view: 'rdram', hex: 'ee' }, 'foreign')).error, 'busy');
    unwrap(await native.request('session.pause'));
    assert.equal((await native.request('n64.memory.write', undefined, { address: '0x2000', view: 'rdram', hex: 'ee' }, 'batch-idle')).error, 'busy');
    unwrap(await native.request('execution.release', undefined, undefined, 'batch-idle'));
    await peer.close(); peer = undefined; other.close(); await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); other?.close(); await endpoint.close(); router.detach(); native.close();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
