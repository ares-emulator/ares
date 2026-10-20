'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { AudioArtifacts } = require('../dist/services/audio-artifacts.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'audio-fixture', epoch: 1 };
function clientFor(wait = async () => {}) {
  const pcm = Buffer.from('ff7f0180ff3f01c0', 'hex'); const releases = [];
  return { identity: { ...identity }, releases, request: async (method, _identity, p) => {
    let result;
    if (method === 'audio.begin') result = { snapshot_id: 1, frames: 2, bytes: pcm.length, rate: 48000 };
    else if (method === 'audio.read') { await wait(); result = { snapshot_id: 1, offset: p.offset, length: p.length, hex: pcm.toString('hex') }; }
    else if (method === 'audio.release') { releases.push(p.consume); result = { released: true }; }
    else result = { available: true, active: true, count: 2, cap: 2, rate: 48000, done: true };
    return { protocol: 1, id: 'audio', ...identity, ok: true, result };
  } };
}
test('WAV exports preserve stereo PCM, atomically replace files and consume only successful saves', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-wav-');
  try {
    const service = new AudioArtifacts(), native = clientFor(), file = path.join(dir, 'output.wav');
    await fs.writeFile(file, 'old');
    const saved = await service.save(native, identity, file, 'owner', () => true);
    const wav = await fs.readFile(file);
    assert.equal(wav.toString('ascii', 0, 4), 'RIFF'); assert.equal(wav.readUInt32LE(4), 44);
    assert.equal(wav.toString('ascii', 8, 16), 'WAVEfmt '); assert.equal(wav.readUInt16LE(22), 2);
    assert.equal(wav.readUInt32LE(24), 48000); assert.equal(wav.readUInt16LE(34), 16);
    assert.equal(wav.readUInt32LE(40), 8); assert.equal(wav.subarray(44).toString('hex'), 'ff7f0180ff3f01c0');
    assert.deepEqual(native.releases, [true]); assert.equal(saved.bytes, 52);
    decodeReply(JSON.stringify({ protocol: 1, id: 'save', ...identity, ok: true, result: saved }), 'artifact.audioSave');
    await assert.rejects(service.save(native, identity, dir, 'owner', () => true));
    assert.deepEqual(native.releases, [true, false]);
    const committed = path.join(dir, 'committed.wav');
    assert.equal((await service.save(native, identity, committed, 'owner', () => !require('node:fs').existsSync(committed))).path, committed);
    assert.throws(() => encodeRequest('bad', identity, 'audio.capture', { frames: 192001 }), /invalid_request/);
    assert.throws(() => encodeRequest('bad', identity, 'artifact.audioSave', { path: 'nul\0path' }), /invalid_request/);
    assert.deepEqual((await fs.readdir(dir)).sort(), ['committed.wav', 'output.wav']);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('audio export revocation releases its pin without consuming or publishing', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-wav-cancel-');
  try {
    let enter, resume; const started = new Promise(r => enter = r), waiting = new Promise(r => resume = r);
    const native = clientFor(async () => { enter(); await waiting; }), service = new AudioArtifacts();
    const pending = service.save(native, identity, path.join(dir, 'cancelled.wav'), 'owner', () => true);
    await started; service.release('owner'); resume();
    await assert.rejects(pending, { code: 'artifact_cancelled' }); assert.deepEqual(native.releases, [false]); assert.deepEqual(await fs.readdir(dir), []);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
