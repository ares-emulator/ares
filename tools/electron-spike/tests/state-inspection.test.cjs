'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { createHash } = require('node:crypto');
const { execFileSync } = require('node:child_process');
const { StateInspection } = require('../dist/services/state-inspection.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'offline-state-fixture', epoch: 1 };
const format = { version: 'v153', signature: 0x31545342, header_size: 533, slots: [] };
const peek = { address: '0x80000001', length: 7, view: 'rdram', assumeCurrentConfiguration: false };
function clientFor() {
  const methods = [];
  return { identity, methods, request: async method => { methods.push(method); return { protocol: 1, id: 'state', ...identity, ok: true, result: method === 'n64.state.format' ? format : { ...format, offset: 600, size: 16, storage: 'lsb32' } }; } };
}
function state() {
  const b = Buffer.alloc(616); b.writeUInt32LE(format.signature); b[4] = 1; b.write('v153', 5);
  for (let i = 0; i < 16; i++) b[600 + (i ^ 3)] = i;
  return b;
}
async function sidecar(file, bytes, extra = {}) {
  await fs.writeFile(file + '.json', JSON.stringify({ metadata_version: 1, state_size: bytes.length, state_sha256: createHash('sha256').update(bytes).digest('hex'), serializer_version: 'v153', epoch: 9, frame: 123, rdram_layout: { version: 'v153', storage: 'lsb32', offset: 600, size: 16 }, ...extra }));
}
function inspect(service, native, method, p) { return service.inspect(native, identity, method, p, 'owner', () => true); }
test('offline state inspection: headers do not prove configuration; hash/layout/endian/aliases are explicit', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-state-inspect-');
  try {
    const file = path.join(dir, 'baseline.bs'), bytes = state(), service = new StateInspection(), native = clientFor();
    await fs.writeFile(file, bytes); await sidecar(file, bytes);
    const info = await inspect(service, native, 'state.info', { path: file });
    assert.equal(info.header_only, true); assert.equal(info.metadata_status, 'unchecked');
    assert.equal(info.compatibility, 'header_supported_configuration_unverified'); assert.equal(info.capture_epoch, null);
    decodeReply(JSON.stringify({ protocol: 1, id: 'info', ...identity, ok: true, result: info }), 'state.info');
    const saved = await inspect(service, native, 'state.peek', { path: file, ...peek });
    assert.equal(saved.hex, '01020304050607'); assert.equal(saved.addr, '0xffffffff80000001'); assert.equal(saved.paddr, '0x1');
    assert.equal(saved.capture_epoch, 9); assert.equal(saved.capture_frame, 123); assert.equal(saved.active_machine_unchanged, true);
    for (const address of ['0x1', '0xa0000001', '0xffffffffa0000001']) assert.equal((await inspect(service, native, 'state.peek', { path: file, ...peek, address })).hex, saved.hex);
    for (const address of ['0x10', '0xc0000001', '0x180000001', '0xffffffffffffffff']) await assert.rejects(inspect(service, native, 'state.peek', { path: file, ...peek, address }), { code: 'out_of_bounds' });
    await assert.rejects(inspect(service, native, 'state.peek', { path: file, ...peek, address: '0xf', length: 2 }), { code: 'out_of_bounds' });
    assert.deepEqual([...new Set(native.methods)], ['n64.state.format'], 'hash-validated inspection never visits live layout');
    const listed = await inspect(service, native, 'state.list', { directory: dir, offset: 0, limit: 100 });
    assert.equal(listed.total, 1); assert.equal(listed.entries[0].metadata_status, 'validated'); assert.equal(listed.entries[0].capture_frame, 123);
    await fs.writeFile(file + '.json', '{}');
    await assert.rejects(inspect(service, native, 'state.peek', { path: file, ...peek }), { code: 'layout_unavailable' });
    assert.equal((await inspect(service, native, 'state.peek', { path: file, ...peek, assumeCurrentConfiguration: true })).layout_source, 'current_configuration_assumed');
    assert.equal(native.methods.at(-1), 'n64.state.layout');
    await sidecar(file, bytes, { rdram_layout: { version: 'v153', storage: 'lsb32', offset: 615, size: 16 } });
    await assert.rejects(inspect(service, native, 'state.peek', { path: file, ...peek }), { code: 'invalid_layout' });
    await sidecar(file, bytes); bytes[615] ^= 1; await fs.writeFile(file, bytes);
    await assert.rejects(inspect(service, native, 'state.peek', { path: file, ...peek }), { code: 'layout_unavailable' });
    bytes.write('v999', 5); await fs.writeFile(file, bytes);
    assert.equal((await inspect(service, native, 'state.info', { path: file })).compatibility, 'unsupported_version');
    await assert.rejects(inspect(service, native, 'state.peek', { path: file, ...peek }), { code: 'unsupported_version' });
    bytes[0] = 0; await fs.writeFile(file, bytes);
    assert.equal((await inspect(service, native, 'state.info', { path: file })).compatibility, 'bad_signature');
    await fs.writeFile(file, Buffer.alloc(532)); await assert.rejects(inspect(service, native, 'state.info', { path: file }), { code: 'state_truncated' });
    assert.throws(() => encodeRequest('bad', identity, 'state.peek', { path: file, ...peek, length: 1025 }), /invalid_request/);
    assert.throws(() => encodeRequest('bad', identity, 'state.info', { path: 'nul\0file' }), /invalid_request/);
    assert.ok(native.methods.every(method => ['n64.state.format', 'n64.state.layout'].includes(method)));
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('state inventory bounds response bytes, scanning, missing slots, huge files and nonregular inputs', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-state-inventory-');
  try {
    const service = new StateInspection(), native = clientFor(), bytes = state();
    for (let start = 0; start < 4097; start += 128) await Promise.all(Array.from({ length: Math.min(128, 4097 - start) }, (_, n) => fs.writeFile(path.join(dir, `${String(start+n).padStart(5,'0')}${'x'.repeat(150)}.bs`), bytes)));
    const first = await inspect(service, native, 'state.list', { directory: dir, offset: 0, limit: 1000 });
    assert.equal(first.truncated, true); assert.equal(first.total, 4096); assert.ok(first.entries.length < 1000); assert.equal(first.next_offset, first.entries.length);
    assert.ok(Buffer.byteLength(JSON.stringify(first)) < 49000);
    const second = await inspect(service, native, 'state.list', { directory: dir, offset: first.next_offset, limit: 2 });
    assert.equal(second.entries.length, 2); assert.notEqual(first.entries.at(-1).path, second.entries[0].path);
    const huge = path.join(dir, 'huge.bs'), h = await fs.open(huge, 'w'); await h.truncate(67108865); await h.close();
    await assert.rejects(inspect(service, native, 'state.info', { path: huge }), { code: 'state_too_large' });
    const pipe = path.join(dir, 'pipe.bs'); execFileSync('/usr/bin/mkfifo', [pipe]);
    await assert.rejects(inspect(service, native, 'state.info', { path: pipe }), { code: 'not_regular_file' });
    format.slots = [path.join(dir, 'missing.bs1')];
    const slots = await inspect(service, native, 'state.list', { directory: null, offset: 0, limit: 100 });
    assert.deepEqual(slots.entries, [{ path: format.slots[0], exists: false, slot: 1 }]); format.slots = [];
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('state inspection revocation cancels work and frees the owner slot', async () => {
  const service = new StateInspection(), native = clientFor(); let resume, enter;
  const waiting = new Promise(r => resume = r), started = new Promise(r => enter = r), original = native.request;
  native.request = async (...args) => { enter(); await waiting; return original(...args); };
  const pending = inspect(service, native, 'state.info', { path: '/tmp/not-read-after-cancel.bs' });
  await started; service.release('owner'); resume(); await assert.rejects(pending, { code: 'artifact_cancelled' });
  await assert.rejects(inspect(service, native, 'state.info', { path: '/tmp/missing-state-fixture.bs' }), { code: 'state_read_failed' });
});
