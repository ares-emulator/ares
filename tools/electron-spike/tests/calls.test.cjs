'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const fixture = process.env.ARES_CPU_FIXTURE || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-cpu-fixture');
const identity = { session: 'call-fixture', epoch: 9 };
const params = { address: '0x33000', args: ['0xffffffffffffffff'], maxInstructions: 100000, maxFrames: 60, timeoutMs: 5000 };
test('call contracts bound arguments/instructions/deadlines and validate actual immutable completion/recovery', () => {
  assert.match(encodeRequest('x', identity, 'n64.cpu.call', params), /0x33000 100000 60 5000 0xffffffffffffffff/);
  for (const update of [{ args: Array(5).fill('0x0') }, { args: ['0x10000000000000000'] }, { maxInstructions: 1000001 }, { maxFrames: 0 }, { timeoutMs: 600001 }]) assert.throws(() => encodeRequest('x', identity, 'n64.cpu.call', { ...params, ...update }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.cpu.callRecover', { restore: 1 }), /invalid_request/);
  const output = execFileSync(fixture, { encoding: 'utf8', timeout: 10000 });
  for (const [tag, method] of [['CALL_SUCCESS', 'execution.status'], ['CALL_INTERRUPTED', 'execution.status'], ['CALL_RECOVERED', 'n64.cpu.callRecover']]) {
    const result = JSON.parse(output.split('\n').find(l => l.startsWith(tag + ' ')).slice(tag.length + 1));
    const reply = { protocol: 1, id: 'x', ...identity, ok: true, result };
    decodeReply(JSON.stringify(reply), method);
    if (tag === 'CALL_SUCCESS') { assert.equal(result.call.caller_context_restored, true); result.call.recovery_available = true; }
    else if (tag === 'CALL_INTERRUPTED') { assert.equal(result.call.recovery_available, true); result.call.caller_snapshot.gpr[0] = 0; }
    else result.memory_device_effects_retained = false;
    assert.throws(() => decodeReply(JSON.stringify(reply), method), /invalid_reply/);
  }
});
