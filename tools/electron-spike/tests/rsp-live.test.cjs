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
test('RSP service contracts reject unbounded grants, misaligned breakpoints and malformed progress', () => {
  for (const count of [0, 1000001, 1.5]) assert.throws(() => encodeRequest('rsp', identity, 'n64.rsp.step', { count }), /invalid_request/);
  for (const address of ['0x1', '0x1000', 'not-hex']) assert.throws(() => encodeRequest('rsp', identity, 'n64.rsp.break', { action: 'add', address }), /invalid_request/);
  assert.throws(() => encodeRequest('rsp', identity, 'n64.rsp.debug', { enable: 1 }), /invalid_request/);
  const state = { available: true, active: true, stopped: true, curPc: '0x4', instrCount: 1, stepsRemaining: 0, runToBp: false, breakpoints: ['0x4'], stop_reason: 'rsp_stop', executor: 'interpreter' };
  const reply = { protocol: 1, id: 'rsp', session: identity.session, epoch: 1, ok: true, result: state };
  decodeReply(JSON.stringify(reply), 'n64.rsp.debug');
  for (const mutate of [v => { v.executor = 'recompiler'; }, v => { v.stepsRemaining = 1000001; }, v => { v.breakpoints = ['0x1']; }, v => { v.instrCount = Number.MAX_SAFE_INTEGER + 1; }]) {
    const result = structuredClone(state); mutate(result);
    assert.throws(() => decodeReply(JSON.stringify({ ...reply, result }), 'n64.rsp.debug'), /invalid_reply/);
  }
});
test('real N64 RSP debugger: instruction grants, breakpoints, hardware HALT and owner cleanup', { timeout: 30000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-rsp-live-'), image = Buffer.alloc(0x1000);
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
    const done = async op => {
      for (let i = 0; i < 700; ++i) {
        const r = await peer.tool('operation_status', { operation_id: op.operation.operation_id });
        if (r.state !== 'running') return r;
        await sleep(10);
      }
      assert.fail('RSP operation failed to stop');
    };
    // CPU code clears SP HALT then loops; RSP code executes its own branch delay
    // slot before stopping at a breakpoint. Both programs are original fixtures.
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: 0x80, hex_bytes: '3c08a40424090001ad0900101000ffff00000000' });
    await peer.tool('write_memory', { region: 'RSP IMEM', addr: 0, hex_bytes: '24080001100000022508000125080040250800020000000d' });
    await peer.tool('cpu_set_registers', { pc: '0xa4000080' });
    assert.equal((await peer.tool('rsp_step')).error, 'not_active');
    const enabled = await peer.tool('rsp_debug');
    assert.equal(enabled.active, true); assert.equal(enabled.stopped, true); assert.equal(enabled.instrCount, 0);
    assert.equal((await peer.tool('rsp_registers')).status.halted, 1, 'enable preserves hardware HALT');
    assert.equal((await native.request('n64.rsp.debug', undefined, { enable: false }, 'foreign')).error, 'busy');
    assert.equal((await native.request('n64.rsp.step', undefined, { count: 1 }, 'foreign')).error, 'busy');
    assert.equal((await peer.tool('step_instructions', { n: 1 })).error, 'busy');
    const first = await done(await peer.tool('rsp_step', { count: 1 }));
    assert.equal(first.rsp.executed, 1); assert.equal(first.rsp.pc, '0x4'); assert.equal(first.rsp.stop_reason, 'rsp_instructions_done');
    assert.equal((await peer.tool('rsp_registers')).gpr[8], '0x1');
    assert.deepEqual((await peer.tool('rsp_break', { action: 'add', addr: '0x4' })).breakpoints, ['0x4']);
    await peer.tool('rsp_break', { action: 'add', addr: '0x10' });
    const run = await done(await peer.tool('rsp_run'));
    assert.equal(run.rsp.executed, 2); assert.equal(run.rsp.pc, '0x10'); assert.equal(run.rsp.stop_reason, 'rsp_breakpoint');
    assert.equal((await peer.tool('rsp_registers')).gpr[8], '0x2');
    const finish = await done(await peer.tool('rsp_step', { count: 100 }));
    assert.equal(finish.rsp.executed, 2); assert.equal(finish.rsp.stop_reason, 'rsp_halted'); assert.equal(finish.rsp.hardware_halted, true);
    const waiting = await peer.tool('rsp_run');
    assert.equal((await peer.tool('write_memory', { region: 'RSP IMEM', addr: 0, hex_bytes: '00000000' })).error, 'busy');
    await peer.tool('rsp_stop'); const stop = await done(waiting);
    assert.equal(stop.rsp.executed, 0); assert.equal(stop.rsp.stop_reason, 'rsp_stop');
    const timeout = await done(await peer.tool('rsp_step', { count: 1 }));
    assert.equal(timeout.rsp.executed, 0); assert.equal(timeout.rsp.stop_reason, 'timeout');
    assert.equal((await peer.tool('rsp_debug', { enable: false })).active, false);
    assert.equal((await peer.tool('operation_status', { operation_id: first.operation_id })).rsp.executed, 1, 'completed counters are immutable');
    await peer.tool('rsp_debug'); await peer.tool('pause');
    assert.equal((await peer.tool('rsp_registers')).debug.active, false, 'pause releases idle debugger');
    await peer.tool('rsp_debug');
    await peer.close(); peer = undefined;
    for (let i = 0; i < 100 && unwrap(await native.request('n64.rsp.registers')).debug.active; ++i) await sleep(10);
    assert.equal(unwrap(await native.request('n64.rsp.registers')).debug.active, false, 'disconnect releases idle gate');
    await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); await endpoint.close(); router.detach(); native.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
