'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { createHash } = require('node:crypto');
const { StateArtifacts } = require('../dist/services/state-artifacts.js');
const { StateInspection } = require('../dist/services/state-inspection.js');
const { decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'state-save-fixture', epoch: 1 };
function clientFor() {
  const bytes = Buffer.alloc(616); bytes.writeUInt32LE(0x31545342); bytes[4] = 1; bytes.write('v153', 5);
  for (let n = 0; n < 16; n++) bytes[600 + (n ^ 3)] = n + 0x20;
  let released = 0;
  return { identity, get released() { return released; }, request: async (method, _identity, p) => {
    let result;
    if (method === 'n64.state.begin') result = { snapshot_id: 1, bytes: bytes.length, version: 'v153', epoch: 1, frame: 7, offset: 600, size: 16, storage: 'lsb32', rom_sha256: 'a'.repeat(64), pc: '0x80001000', stopped: true, mid_frame_cpu_stop: true };
    else if (method === 'n64.state.read') result = { snapshot_id: 1, offset: p.offset, length: p.length, hex: bytes.subarray(p.offset, p.offset + p.length).toString('hex') };
    else if (method === 'n64.state.release') { released++; result = { released: true }; }
    else result = { signature: 0x31545342, version: 'v153', header_size: 533, slots: [] };
    return { protocol: 1, id: 'save', ...identity, ok: true, result };
  } };
}
test('state save publishes native bytes and hash/layout sidecar; sidecar failure keeps successful file metadata', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-state-save-');
  try {
    const service = new StateArtifacts(), native = clientFor(), file = path.join(dir, 'baseline.bs');
    const saved = await service.save(native, identity, { path: file, overwrite: false, metadata: true }, 'owner', () => true);
    assert.equal(saved.size, 616); assert.equal(saved.frame, 7); assert.equal(native.released, 1);
    assert.equal(saved.sha256, createHash('sha256').update(await fs.readFile(file)).digest('hex'));
    const sidecar = JSON.parse(await fs.readFile(saved.metadata_path)); assert.equal(sidecar.rdram_layout.offset, 600);
    decodeReply(JSON.stringify({ protocol: 1, id: 'save', ...identity, ok: true, result: saved }), 'state.save');
    const offline = await new StateInspection().inspect(native, identity, 'state.peek', { path: file, address: '0x80000001', length: 7, view: 'rdram', assumeCurrentConfiguration: false }, 'owner', () => true);
    assert.equal(offline.hex, '21222324252627'); assert.equal(offline.capture_frame, 7);
    await assert.rejects(service.save(native, identity, { path: file, overwrite: false, metadata: true }, 'owner', () => true), { code: 'file_exists' });
    assert.equal(native.released, 2);
    const partial = path.join(dir, 'state-only.bs'); await fs.mkdir(partial + '.json');
    const savedPartial = await service.save(native, identity, { path: partial, overwrite: false, metadata: true }, 'owner', () => true);
    assert.equal(savedPartial.path, partial); assert.ok(savedPartial.metadata_error); assert.equal((await fs.stat(partial)).size, 616);
    const noMeta = path.join(dir, 'no-metadata.bs'); await service.save(native, identity, { path: noMeta, overwrite: false, metadata: false }, 'owner', () => true);
    await assert.rejects(fs.stat(noMeta + '.json'), { code: 'ENOENT' });
    const committed = path.join(dir, 'committed.bs');
    const result = await service.save(native, identity, { path: committed, overwrite: false, metadata: true }, 'owner', () => !require('node:fs').existsSync(committed));
    assert.equal(result.path, committed); assert.equal(result.metadata_error, 'artifact_cancelled');
    assert.ok((await fs.readdir(dir)).every(name => !name.startsWith('.ares-artifact-')));
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
