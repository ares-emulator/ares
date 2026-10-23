'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { decodeUnsigned, encodeUnsigned } = require('../dist/services/memory.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'validation', epoch: 1 };
test('explicit endian helpers decode unaligned slices without platform byte-order assumptions', () => {
  const bytes = Uint8Array.from([9, 0x12, 0x34, 0x56, 0x78, 9]).subarray(1, 5);
  assert.equal(decodeUnsigned(bytes, 4, 'big'), 0x12345678);
  assert.equal(decodeUnsigned(bytes, 4, 'little'), 0x78563412);
  assert.equal(decodeUnsigned(bytes, 2, 'big', 1), 0x3456);
  assert.deepEqual([...encodeUnsigned(0x12345678, 4, 'little')], [0x78, 0x56, 0x34, 0x12]);
  assert.deepEqual([...encodeUnsigned(0x1234, 2, 'big')], [0x12, 0x34]);
  assert.throws(() => encodeUnsigned(256, 1, 'big'), /integer_out_of_range/);
  assert.throws(() => decodeUnsigned(bytes, 4), /invalid_integer_format/);
  assert.throws(() => decodeUnsigned(bytes, 4, 'big', 1), /out_of_bounds/);
});
test('memory contract rejects missing/extra arguments, wrapping offsets, malformed hex and unbounded transfers', () => {
  for (const p of [undefined, { region: 'r', offset: -1, length: 1 }, { region: 'r', offset: 0, length: 0 }, { region: 'r', offset: 0, length: 1025 }, { region: 'r', offset: 0, length: 1, address: 0 }])
    assert.throws(() => encodeRequest('req', identity, 'memory.read', p), /invalid_request/);
  for (const hex of ['f', 'zz', '', 'ab'.repeat(1025), '01\nquit'])
    assert.throws(() => encodeRequest('req', identity, 'memory.write', { region: 'r', offset: 0, hex }), /invalid_request/);
  const reply = { protocol: 1, id: 'r', ...identity, ok: true, result: { region: 'r', offset: 0, length: 2, hex: 'ff' } };
  assert.throws(() => decodeReply(JSON.stringify(reply), 'memory.read'), /invalid_reply/);
});
test('atomic ranges enforce exact count and aggregate byte limits and retain overlaps', () => {
  const range = { region: 'r', offset: 0, length: 1024 };
  for (const method of ['memory.readMany', 'n64.memory.readMany']) {
    const r = method.startsWith('n64') ? { address: '0x80000000', view: 'cpu', length: 1024 } : range;
    assert.doesNotThrow(() => encodeRequest('req', identity, method, { reads: Array(64).fill(r) }));
    assert.doesNotThrow(() => encodeRequest('req', identity, method, { reads: [{ ...r, length: 65536 }] }));
    for (const reads of [[], Array(65).fill(r), [{ ...r, length: 65537 }], [{ ...r, length: 65536 }, r], [r, { ...r, length: 0 }]])
      assert.throws(() => encodeRequest('req', identity, method, { reads }), /invalid_request/);
  }
});
