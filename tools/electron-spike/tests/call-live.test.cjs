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
test('real N64 calls: private returns, bounded interruptions, explicit recovery and disconnect ownership', { timeout: 30000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-call-live-'), image = Buffer.alloc(0x1000);
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
    const rawFile = path.join(dir, 'raw.bs');
    assert.equal((await peer.tool('state_save', { path: rawFile })).error, undefined);
    const layout = unwrap(await native.request('n64.state.layout')), initialized = fs.readFileSync(rawFile);
    initialized[layout.offset + layout.size] = 1; // original fixture's RDRAM map
    const initializedFile = path.join(dir, 'initialized.bs'); fs.writeFileSync(initializedFile, initialized);
    assert.equal((await peer.tool('state_load', { path: initializedFile })).error, undefined);
    const functionWords = [0x0085102d, 0xfcc20000, 0x03e00008, 0x24030055];
    const writeCode = async words => {
      const bytes = Buffer.alloc(words.length * 4); words.forEach((w, i) => bytes.writeUInt32BE(w, i * 4));
      assert.equal((await peer.tool('write_memory_va', { addr: '0x33000', hex_bytes: bytes.toString('hex'), view: 'cpu' })).written, bytes.length);
      // Debug instruction fetch observes resident I-cache. A state restore below
      // flushes prior translations/cache before changing fixture code variants.
    };
    await writeCode(functionWords);
    await peer.tool('cpu_set_registers', { pc: '0x80032000', status: '0x34000000', gpr: { sp: '0xffffffff80034000', ra: '0xffffffff80032008', a0: '0x123', a1: '0x456' }, hi: '0x1234', lo: '0x5678', llbit: 1 });
    const baselineFile = path.join(dir, 'baseline.bs');
    assert.equal((await peer.tool('state_save', { path: baselineFile })).error, undefined);
    const baseline = await peer.tool('cpu_registers', { fpr: true, cop0: true });
    const success = await peer.tool('call_function', { addr: '0x33000', args: ['0x1234567890000000', 7, '0xffffffff80035000'], max_instructions: 4 });
    assert.equal(success.error, undefined, JSON.stringify(success)); assert.equal(success.stop_reason, 'function_return');
    assert.equal(success.executed, 4); assert.equal(success.v0, '0x1234567890000007'); assert.equal(success.v1, '0x55'); assert.equal(success.caller_context_restored, true);
    const after = await peer.tool('cpu_registers', { fpr: true, cop0: true });
    for (const key of ['pc', 'next_pc', 'delay_slot', 'gpr', 'hi', 'lo', 'fpr', 'fcr31']) assert.deepEqual(after[key], baseline[key], key);
    assert.equal((await peer.tool('read_memory_va', { addr: '0x35000', length: 8, view: 'cpu' })).hex, '1234567890000007', 'function memory effects retained');
    assert.equal((await peer.tool('call_recover')).error, 'no_recovery');
    assert.equal((await peer.tool('call_function', { addr: '0x33001' })).error, 'unaligned_address');
    const restore = async words => { assert.equal((await peer.tool('state_load', { path: baselineFile })).error, undefined); await writeCode(words); };
    await restore([0x1000ffff, 0]);
    const limited = await peer.tool('call_function', { addr: '0x33000', max_instructions: 2 });
    assert.equal(limited.stop_reason, 'instruction_limit', JSON.stringify(limited)); assert.equal(limited.recovery_available, true); assert.equal(limited.caller_context_restored, false);
    assert.equal((await peer.tool('call_function', { addr: '0x33000' })).error, 'recovery_pending');
    assert.equal((await native.request('n64.cpu.callRecover', undefined, { restore: true }, 'foreign')).error, 'busy');
    const recovered = await peer.tool('call_recover'); assert.equal(recovered.restored, true); assert.equal(recovered.pc, baseline.pc);
    assert.deepEqual((await peer.tool('operation_status', { operation_id: limited.operation_id })).call, limited.call, 'completed call survives explicit recovery unchanged');
    await restore([0x0000000c]);
    const trapped = await peer.tool('call_function', { addr: '0x33000' });
    assert.equal(trapped.stop_reason, 'exception', JSON.stringify(trapped)); assert.equal(trapped.exception_code, 8); assert.equal(trapped.recovery_available, true);
    const exceptionCause = (await peer.tool('cpu_registers', { cop0: true })).cop0[13];
    await peer.tool('call_recover');
    assert.equal((await peer.tool('cpu_registers', { cop0: true })).cop0[13], exceptionCause, 'Cause is retained after recovery');
    await restore([0x27bdfff8, 0x03e00008, 0]);
    const invalid = await peer.tool('call_function', { addr: '0x33000', max_instructions: 100 });
    assert.equal(invalid.stop_reason, 'invalid_return', JSON.stringify(invalid));
    assert.equal((await peer.tool('call_recover', { restore: false })).restored, false);
    assert.equal((await peer.tool('call_recover')).error, 'no_recovery');
    await restore([0x1000ffff, 0]);
    const timeout = await peer.tool('call_function', { addr: '0x33000', max_instructions: 1000000, max_seconds: 0.001 });
    assert.equal(timeout.stop_reason, 'timeout', JSON.stringify(timeout)); await peer.tool('call_recover');
    // Native start acknowledges immediately; disconnect transfers recovery claim
    // while retaining the actual stopped callee rather than restoring implicitly.
    const connection = new (require('../dist/services/local-endpoint.js').LocalServiceClient)(endpoint.metadataFile, endpoint.instance);
    unwrap(await connection.request('session.status'));
    const pending = unwrap(await connection.request('n64.cpu.call', undefined, { address: '0x33000', args: [], maxInstructions: 1000000, maxFrames: 60, timeoutMs: 5000 }));
    connection.close();
    for (let i = 0; i < 100 && unwrap(await native.request('session.status')).operation_id; ++i) await sleep(10);
    const interrupted = unwrap(await native.request('execution.status', undefined, { operationId: pending.operation_id }));
    assert.equal(interrupted.stop_reason, 'disconnected'); assert.equal(interrupted.call.recovery_available, true);
    assert.equal((await peer.tool('call_recover')).restored, true, 'new authenticated connection can claim disconnected recovery');
    await peer.close(); peer = undefined; await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); await endpoint.close(); router.detach(); native.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
