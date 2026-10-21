'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { StateLoading } = require('../dist/services/state-loading.js');
const { writeStateFixture } = require('./state-file-fixture.cjs');
const identity = { session: 'state-load-fixture', epoch: 1 };
function nativeFor() {
  const calls = []; let uploaded = Buffer.alloc(0), aborted = 0;
  return { identity: { ...identity }, calls, get uploaded() { return uploaded; }, get aborted() { return aborted; }, request: async (method, _identity, p) => {
    calls.push(method); let result;
    if (method === 'n64.state.loadBegin') result = { upload_id: 1 };
    else if (method === 'n64.state.format') result = { version: 'v153', signature: 0x31545342, header_size: 533, slots: [] };
    else if (method === 'n64.state.loadPrepare') { uploaded = Buffer.alloc(p.bytes); result = { prepared: true }; }
    else if (method === 'n64.state.loadWrite') { const bytes = Buffer.from(p.hex, 'hex'); bytes.copy(uploaded, p.offset); result = { written: bytes.length }; }
    else if (method === 'n64.state.loadAbort') { aborted++; result = { released: true }; }
    else return { ok: true, result: { epoch: 2, frame: 0, at_frame_boundary: false, paused: !p.resume, stopped: !p.resume, running: p.resume } };
    return { ok: true, result };
  } };
}
test('state loader uploads exact immutable bytes; preflight failures abort without committing', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-load-unit-');
  try {
    const file = path.join(dir, 'state.bs'); writeStateFixture(file);
    const loader = new StateLoading(), native = nativeFor();
    const loaded = await loader.load(native, identity, { path: file, resume: true, keepOverrides: true }, 'owner', () => true);
    assert.equal(loaded.epoch, 2); assert.equal(loaded.running, true); assert.equal(native.aborted, 0);
    assert.deepEqual(native.uploaded, await fs.readFile(file));
    assert.equal(native.calls.at(-1), 'n64.state.loadCommit');
    const changed = await fs.readFile(file); changed[548] ^= 1; await fs.writeFile(file, changed);
    await assert.rejects(loader.load(native, identity, { path: file, resume: false, keepOverrides: false }, 'owner', () => true), { code: 'state_metadata_mismatch' });
    assert.equal(native.calls.at(-1), 'n64.state.loadAbort'); assert.equal(native.aborted, 1);
    changed.write('v999', 5); await fs.writeFile(file, changed);
    await assert.rejects(loader.load(native, identity, { path: file, resume: false, keepOverrides: false }, 'owner', () => true), { code: 'unsupported_version' });
    assert.equal(native.calls.at(-1), 'n64.state.loadAbort');
    assert.equal(native.calls.filter(method => method === 'n64.state.loadCommit').length, 1);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('state loader cancellation aborts upload and never commits', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-load-cancel-');
  try {
    const file = path.join(dir, 'state.bs'); writeStateFixture(file);
    const native = nativeFor(), original = native.request; let enter, resume;
    const started = new Promise(r => enter = r), waiting = new Promise(r => resume = r);
    native.request = async (...args) => { if (args[0] === 'n64.state.loadWrite') { enter(); await waiting; } return original(...args); };
    const loader = new StateLoading(), pending = loader.load(native, identity, { path: file, resume: false, keepOverrides: false }, 'owner', () => true);
    await started; loader.release('owner'); resume(); await assert.rejects(pending, { code: 'artifact_cancelled' });
    assert.equal(native.aborted, 1); assert.ok(!native.calls.includes('n64.state.loadCommit'));
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
