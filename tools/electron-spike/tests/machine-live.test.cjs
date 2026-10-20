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
test('machine contracts bound atomic register sets, aliases, observations and disassembly', () => {
  assert.match(encodeRequest('x', identity, 'n64.cpu.setRegisters', { ...empty, gpr: [{ index: 8, value: '0xffffffffffffffff' }], llbit: 1 }), /gpr:8:0xffffffffffffffff llbit:0x1/);
  for (const gpr of [[{ index: 0, value: '0x0' }], [{ index: 32, value: '0x0' }], [{ index: 8, value: '0x1' }, { index: 8, value: '0x2' }]]) assert.throws(() => encodeRequest('x', identity, 'n64.cpu.setRegisters', { ...empty, gpr }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.cpu.setRegisters', { ...empty, status: '0x100000000' }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.cpu.setRegisters', { ...empty, llbit: 2 }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.cpu.disassemble', { address: '0x0', count: 129, view: 'cpu' }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.rsp.registers', {}), /invalid_request/);
  const output = execFileSync(fixture, { encoding: 'utf8', timeout: 10000 });
  for (const [tag, method] of [['MACHINE_RSP', 'n64.rsp.registers'], ['MACHINE_RDP', 'n64.rdp.state'], ['MACHINE_PERIPHERALS', 'n64.peripheral.registers'], ['MACHINE_DISASSEMBLY', 'n64.cpu.disassemble'], ['MACHINE_MUTATION', 'n64.cpu.setRegisters']]) {
    const data = JSON.parse(output.split('\n').find(l => l.startsWith(tag + ' ')).slice(tag.length + 1));
    const reply = { protocol: 1, id: 'fixture', session: identity.session, epoch: 7, ok: true, result: data };
    decodeReply(JSON.stringify(reply), method);
    if (tag === 'MACHINE_RSP') { assert.equal(data.vu[4][7], 0xabcd); assert.equal(data.dma.curOriginPc, '0xfedcba9876543210'); assert.equal(data.status.semaphore, 0); data.vu[0][0] = 65536; }
    else if (tag === 'MACHINE_RDP') data.freeze = 2;
    else if (tag === 'MACHINE_PERIPHERALS') delete data.vi.VI_ORIGIN;
    else if (tag === 'MACHINE_DISASSEMBLY') data.instructions[0].word = 1;
    else data.changed.gpr.zero = '0x0';
    assert.throws(() => decodeReply(JSON.stringify(reply), method), /invalid_reply/);
  }
});
test('real N64 and MCP: atomic register mutation, PC pipeline/stops, observations and ownership gates', { timeout: 30000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-machine-live-'), image = Buffer.alloc(0x1000);
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
    const stopped = await peer.tool('step_instructions', { n: 2 }), before = await peer.tool('cpu_registers', { cop0: true });
    const originalOperation = unwrap(await native.request('execution.status', undefined, { operationId: stopped.operation_id }));
    const rsp1 = await peer.tool('rsp_registers'); assert.equal(rsp1.gpr.length, 32); assert.equal(rsp1.vu.length, 32); assert.equal(rsp1.debug.available, true);
    const rdp = await peer.tool('rdp_state'); assert.ok(['DMEM', 'RDRAM'].includes(rdp.source));
    const peripherals = await peer.tool('peripheral_registers'); assert.equal(typeof peripherals.vi.VI_WIDTH, 'number');
    assert.equal((await peer.tool('disassemble', { addr: '0x80001000', count: 128 })).instructions.length, 128);
    assert.deepEqual(await peer.tool('cpu_registers', { cop0: true }), before, 'observations leave CPU unchanged');
    assert.deepEqual(await peer.tool('rsp_registers'), rsp1, 'observations leave semaphore/DMA/vector state unchanged');
    assert.equal((await peer.tool('cpu_set_registers', { gpr: { t0: '0x1234' }, pc: '0xa4000081' })).error, 'unaligned_pc');
    assert.deepEqual(await peer.tool('cpu_registers', { cop0: true }), before);
    assert.equal((await peer.tool('cpu_set_registers', { gpr: { zero: 0 } })).error, 'invalid_register');
    assert.equal((await peer.tool('cpu_set_registers', { gpr: { t0: 1, '8': 2 } })).error, 'invalid_request');
    assert.equal((await peer.tool('cpu_set_registers', { status: '0x100000000' })).error, 'invalid_request');
    const unsafe = await peer.request('tools/call', { name: 'cpu_set_registers', arguments: { gpr: { t0: Number.MAX_SAFE_INTEGER + 1 } } });
    assert.equal(unsafe.result.isError, true);
    // IPL1 has not copied IPL3 after two CPU instructions; install this original
    // instruction fixture in the exposed DMEM region before redirecting the PC.
    assert.equal((await peer.tool('write_memory', { region: 'RSP DMEM', addr: 0x80, hex_bytes: '240200071000ffff00000000' })).written, 12);
    const changed = await peer.tool('cpu_set_registers', { gpr: { t0: '0xfedcba9876543210', fp: '0xffffffffffffffff' }, pc: '0xa4000080', hi: '0x123456789abcdef0', lo: '0xdeadbeef', llbit: 1, status: before.cop0[12] });
    assert.equal(changed.error, undefined, JSON.stringify(changed)); assert.equal(changed.pc, '0xffffffffa4000080'); assert.equal(changed.changed.gpr.s8, '0xffffffffffffffff');
    const current = await peer.tool('cpu_registers'); assert.equal(current.gpr.t0, '0xfedcba9876543210'); assert.equal(current.next_pc, '0xffffffffa4000084'); assert.equal(current.delay_slot, false);
    assert.equal((await peer.tool('last_stop')).valid, false);
    assert.deepEqual((await peer.tool('operation_status', { operation_id: stopped.operation_id })).debug_stop, originalOperation.debug_stop, 'completed execution result stays immutable after PC mutation');
    const step = await peer.tool('step_instructions', { n: 1 }); assert.equal(step.executed, 1); assert.equal((await peer.tool('cpu_registers')).gpr.v0, '0x7');
    await peer.tool('cpu_set_registers', { hi: '0x1' }); assert.equal((await peer.tool('cpu_registers')).hi, '0x1'); assert.equal((await peer.tool('last_stop')).registers.hi, '0x123456789abcdef0');
    await peer.tool('resume'); assert.equal((await peer.tool('cpu_set_registers', { lo: 1 })).error, 'not_stopped'); await peer.tool('pause');
    const run = unwrap(await native.request('execution.runFrames', undefined, { count: 1, timeoutMs: 1000 }));
    assert.equal((await peer.tool('cpu_set_registers', { lo: 1 })).error, 'busy'); assert.equal((await peer.tool('rsp_registers')).gpr.length, 32);
    unwrap(await native.request('execution.cancel', undefined, { operationId: run.operation_id }));
    const upload = unwrap(await native.request('n64.state.loadBegin'));
    assert.equal((await peer.tool('cpu_set_registers', { lo: 1 })).error, 'busy'); assert.equal((await peer.tool('rdp_state')).source, rdp.source);
    unwrap(await native.request('n64.state.loadAbort', undefined, { uploadId: upload.upload_id }));
    unwrap(await native.request('execution.batchBegin', undefined, { timeoutMs: 1000 }));
    assert.equal((await peer.tool('cpu_set_registers', { lo: 1 })).error, 'busy'); assert.equal((await peer.tool('peripheral_registers')).vi.VI_WIDTH, peripherals.vi.VI_WIDTH);
    unwrap(await native.request('execution.release'));
    assert.equal((await peer.tool('disassemble', { addr: '0x80000001', count: 1 })).error, 'unaligned_address');
    assert.equal((await peer.tool('disassemble', { addr: '0x7ffffc', count: 2 })).error, 'out_of_bounds');
    await peer.close(); peer = undefined; await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); await endpoint.close(); router.detach(); native.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
