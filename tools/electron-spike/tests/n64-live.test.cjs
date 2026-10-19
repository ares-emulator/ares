'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost, LocalServiceClient } = require('../dist/services/local-endpoint.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const { conditions, lossless } = require('../dist/mcp/n64-tools.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const fixture = process.env.ARES_CPU_FIXTURE || path.join(path.dirname(host), 'ares-electron-cpu-fixture');
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const identity = { session: 'n64-debug-fixture', epoch: 1 };
const unwrap = reply => { assert.equal(reply.ok, true, JSON.stringify(reply)); return reply.result; };
async function done(client, operation) {
  for (let i = 0; i < 200; ++i) {
    const result = unwrap(await client.request('execution.status', undefined, { operationId: operation.operation_id }));
    if (result.state === 'done') return result;
    await sleep(5);
  }
  throw new Error('N64 operation did not finish');
}
test('N64 contracts preserve 64-bit values and bound predicates and memory requests', () => {
  assert.equal(lossless('0xffffffff80002000'), '0xffffffff80002000');
  assert.throws(() => lossless(Number.MAX_SAFE_INTEGER + 1), /invalid_address/);
  assert.throws(() => lossless('0x10000000000000000'), /invalid_address/);
  const condition = conditions({ all: [{ field: 'gpr.t1', value: '0xffffffffffffffff', op: '>=', mask: '0xff' }, { field: 'pc', value: '0x80001000' }] });
  assert.equal(condition[0].index, 9); assert.equal(condition[0].op, 'ge');
  assert.equal(condition[1].value, '0xffffffff80001000');
  assert.throws(() => conditions({ all: Array(9).fill({ field: 'pc', value: 0 }) }), /invalid_condition/);
  assert.throws(() => conditions({ field: 'gpr.32', value: 0 }), /invalid_condition/);
  assert.throws(() => conditions({ field: 'pc', value: 0, unknown: 1 }), /invalid_condition/);
  assert.match(encodeRequest('x', identity, 'n64.debug.breakAdd', { address: '0x80001000', condition }), /0:0:0:0xffffffff80001000:0xffffffffffffffff/);
  assert.throws(() => encodeRequest('x', identity, 'n64.debug.breakAdd', { address: '0x80001000', condition: conditions({ field: 'value', value: 1 }) }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.memory.write', { address: '0x80000000', view: 'cpu', hex: '00'.repeat(1025) }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.memory.read', { address: '0x80000000', view: 'cpu', length: 0 }), /invalid_request/);
  assert.throws(() => decodeReply(JSON.stringify({ protocol: 1, id: 'x', ...identity, ok: true, result: { pc: 123 } }), 'n64.debug.registers'), /invalid_reply/);
});
test('real MIPS core fixture: delay slots, aliases, pre/post stops, semantic accesses and dirty cache', () => {
  assert.ok(fs.existsSync(fixture), 'Build the ares-electron-cpu-fixture CMake target first');
  assert.match(execFileSync(fixture, { encoding: 'utf8', timeout: 10000 }), /CPU_DEBUG_FIXTURE_PASS/);
});
test('isolated N64 host and real MCP stdio share instruction operations, registers and definitions', { timeout: 30000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-n64-debug-');
  const image = Buffer.alloc(0x1000); image.writeUInt32BE(0x80371240); image.write('CPU DEBUG FIXTURE', 0x20); image[0x3e] = 0x45;
  const rom = path.join(dir, 'fixture.z64'); fs.writeFileSync(rom, image);
  const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--silent', '--paused', '--save-dir', dir, '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log += b); child.stdout.resume();
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  const router = new SessionRouter(); router.attach(native); const endpoint = new LocalServiceHost(router);
  let client, peer;
  try {
    const initial = unwrap(await native.request('session.status'));
    assert.equal(initial.paused, true);
    const before = unwrap(await native.request('n64.debug.registers'));
    assert.equal(before.gpr.length, 32); assert.equal(before.gpr[0], '0x0'); assert.equal(before.cop0[1], null);
    assert.deepEqual(unwrap(await native.request('n64.debug.lastStop')), { valid: false });
    const stepped = await done(native, unwrap(await native.request('execution.stepInstructions', undefined, { count: 1, timeoutMs: 1000 })));
    assert.equal(stepped.stop_reason, 'instructions_done'); assert.equal(stepped.executed, 1); assert.equal(stepped.executor, 'interpreter');
    assert.equal(stepped.debug_stop.valid, true); assert.equal(stepped.debug_stop.registers.pc, unwrap(await native.request('n64.debug.registers')).pc);
    assert.deepEqual(unwrap(await native.request('n64.debug.lastStop')), stepped.debug_stop);
    assert.equal(unwrap(await native.request('session.status')).executor, initial.executor, 'transient stepping restores configured executor');
    assert.equal((await native.request('n64.memory.read', undefined, { address: '0xffffffffc0000000', length: 1, view: 'rdram' })).error, 'out_of_bounds');
    assert.equal((await native.request('n64.memory.read', undefined, { address: `0x${(unwrap(await native.request('memory.regions')).regions.find(r => r.name === 'RDRAM').size - 1).toString(16)}`, length: 2, view: 'cpu' })).error, 'out_of_bounds');
    assert.equal(unwrap(await native.request('n64.memory.read', undefined, { address: '0xa0001000', length: 4, view: 'cpu' })).cache_aware, false);
    assert.equal((await native.request('n64.memory.write', undefined, { address: '0x80001000', view: 'cpu', hex: '01020304' })).error, 'rdram_not_initialized');
    const brk = unwrap(await native.request('n64.debug.breakAdd', undefined, { address: '0xa0001000', condition: [] }));
    assert.equal(unwrap(await native.request('session.status')).executor, 'interpreter');
    assert.equal(unwrap(await native.request('n64.debug.breakList', undefined, { offset: 0 })).definitions[0].paddr, '0x1000');
    unwrap(await native.request('n64.debug.breakRemove', undefined, { id: brk.id }));
    assert.equal(unwrap(await native.request('session.status')).executor, initial.executor);
    await endpoint.listen(path.join(dir, 'ipc'));
    client = new LocalServiceClient(endpoint.metadataFile, endpoint.instance); await client.request('session.status');
    peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); await peer.initialize();
    const tools = (await peer.request('tools/list', {})).result.tools;
    assert.equal(tools.find(t => t.name === 'step_instructions').inputSchema.properties.n.default, 1);
    assert.equal(tools.find(t => t.name === 'watch_add').inputSchema.properties.kind.default, 'write');
    const mcpRegisters = await peer.tool('cpu_registers', { cop0: true, fpr: true });
    assert.equal(mcpRegisters.gpr.zero, '0x0'); assert.equal(mcpRegisters.cop0[1], null); assert.equal(mcpRegisters.fpr_format, 'raw64');
    const mcpStep = await peer.tool('step_instructions');
    assert.equal(mcpStep.executed, 1); assert.equal(mcpStep.stop_reason, 'instructions_done');
    const added = await peer.tool('break_add', { addr: '0x80001000', condition: { field: 'gpr.t0', value: '0x0' } });
    assert.ok(added.id > 0); assert.equal(added.breakpoints[0].condition.all[0].field, 'gpr.t0');
    assert.equal((await peer.tool('break_list')).breakpoints[0].id, added.id);
    assert.equal((await peer.tool('break_remove', { id: added.id })).removed, true);
    const watched = await peer.tool('watch_add', { addr: '0xffffffff80002000' });
    assert.ok(watched.id > 0); assert.equal((await peer.tool('watch_list')).watchpoints[0].kind, 'write');
    await peer.tool('watch_remove', { id: watched.id });
    assert.equal((await peer.tool('read_memory_va', { addr: '0x80001000', length: 4 })).hex.length, 8);
    assert.equal((await peer.tool('write_memory_va', { addr: '0x80001000', hex_bytes: '00' })).error, 'rdram_not_initialized');
    const op = unwrap(await client.request('execution.stepInstructions', undefined, { count: 1000000, timeoutMs: 10000 }));
    assert.equal((await native.request('n64.debug.breakAdd', undefined, { address: '0x80001000', condition: [] })).error, 'busy');
    assert.equal((await native.request('n64.memory.write', undefined, { address: '0x80001000', view: 'cpu', hex: '00' })).error, 'busy');
    client.disconnect();
    const disconnected = await done(native, op); assert.equal(disconnected.stop_reason, 'disconnected');
    assert.equal(unwrap(await native.request('session.status')).executor, initial.executor);
    const stale = { ...native.identity };
    unwrap(await native.request('session.reset'));
    assert.equal((await native.request('n64.debug.registers', stale)).error, 'stale_epoch');
    assert.deepEqual(unwrap(await native.request('n64.debug.lastStop')), { valid: false });
    await peer.close(); peer = undefined; client.close(); await endpoint.close(); native.close(); child.stdin.end('quit\n');
    assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); client?.close(); await endpoint.close(); router.detach(); native.close();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(dir, { recursive: true, force: true });
  }
});
