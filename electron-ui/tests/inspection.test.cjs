'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { MemoryDumps } = require('../dist/services/memory-dumps.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'inspection-fixture', epoch: 7 };
const dump = { path: '', overwrite: false, all: false, address: '0x1000', length: 33001, view: 'rdram', expectSize: null };
const fixture = process.env.ARES_CPU_FIXTURE || path.resolve(__dirname, '../../build-electron-make/bin/ares-electron-cpu-fixture');
test('inspection contracts decode real native stack/dump/probe and reject invalid bounds', () => {
  const output = execFileSync(fixture, { encoding: 'utf8', timeout: 10000 });
  for (const [tag, method] of [['INSPECTION_STACK', 'n64.cpu.stack'], ['INSPECTION_DUMP', 'n64.dump.begin'], ['INSPECTION_PROBE', 'n64.render.probe']]) {
    const result = JSON.parse(output.split('\n').find(l => l.startsWith(tag + ' ')).slice(tag.length + 1));
    const reply = { protocol: 1, id: 'fixture', ...identity, ok: true, result };
    decodeReply(JSON.stringify(reply), method);
    if (tag === 'INSPECTION_STACK') result.frames[0].confidence = 'certain';
    else if (tag === 'INSPECTION_DUMP') result.bytes = 8388609;
    else result.frameAdvanced = 1;
    assert.throws(() => decodeReply(JSON.stringify(reply), method), /invalid_reply/);
  }
  for (const length of [0, 1048577, 1.5]) assert.throws(() => encodeRequest('x', identity, 'artifact.dump', { ...dump, path: '/tmp/fixture', length }), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'artifact.dump', { ...dump, path: 'bad\0path' }), /invalid_request/);
});
test('memory dumps retain byte order across pages and release pins on cancellation/stale reads', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-dump-fixture-'), destination = path.join(dir, 'dump.bin');
  const bytes = Buffer.from(Array.from({ length: dump.length }, (_, i) => (i * 13 + (i >>> 8)) & 255));
  const snapshot = { snapshot_id: 4, bytes: bytes.length, ...identity, frame: 10, addr: '0x1000', paddr: '0x1000', view: 'rdram', cache_aware: false, stopped: true };
  const service = new MemoryDumps(); let released = 0, hook = async () => {}, wrong = false;
  const client = { identity: { ...identity }, request: async (method, _identity, p) => {
    let result;
    if (method === 'n64.dump.begin') result = snapshot;
    else if (method === 'n64.dump.release') { released++; result = { released: true }; }
    else { await hook(p); result = { snapshot_id: wrong ? 99 : 4, offset: p.offset, length: p.length, hex: bytes.subarray(p.offset, p.offset + p.length).toString('hex') }; }
    return { protocol: 1, id: 'fake', ...identity, ok: true, result };
  } };
  try {
    const result = await service.save(client, identity, { ...dump, path: destination }, 'owner', () => true);
    assert.deepEqual(await fs.readFile(destination), bytes); assert.equal(released, 1);
    decodeReply(JSON.stringify({ protocol: 1, id: 'fake', ...identity, ok: true, result }), 'artifact.dump');
    let release, entered; const wait = new Promise(r => release = r), start = new Promise(r => entered = r);
    hook = async () => { entered(); await wait; };
    const pending = service.save(client, identity, { ...dump, path: destination, overwrite: true }, 'owner', () => true);
    await start; service.release('owner'); release(); await assert.rejects(pending, { code: 'artifact_cancelled' }); assert.equal(released, 2);
    assert.deepEqual(await fs.readFile(destination), bytes);
    hook = async () => {}; wrong = true;
    await assert.rejects(service.save(client, identity, { ...dump, path: destination, overwrite: true }, 'owner', () => true), { code: 'invalid_reply' }); assert.equal(released, 3);
    assert.deepEqual(await fs.readdir(dir), ['dump.bin']);
  } finally { service.close(); await fs.rm(dir, { recursive: true, force: true }); }
});
