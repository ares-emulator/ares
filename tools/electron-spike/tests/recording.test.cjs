'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const { aviParts } = require('../dist/services/avi.js');
const { RecordingArtifacts } = require('../dist/services/recording-artifacts.js');
const { decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'avi-fixture', epoch: 1 };
const metadata = { active: false, available: true, frames: 2, fps: 50, width: 3, height: 2, epoch: 1, start_frame: 3, end_frame: 4, frame_kind: 'synthetic-vi', stop_reason: 'frames_limit', format: 'avi_raw_bgr24', audio: false, source: 'presented', max_frames: 2, max_bytes: 1048576, captured_bytes: 48 };
function clientFor() {
  // 3 pixels * 3 bytes, each row padded to 12 bytes; bottom-up BGR.
  const frame = Buffer.from('ff000000ff000000ff0000000000ff00ff00ff0000000000', 'hex');
  const bytes = Buffer.concat([frame, frame]), releases = [];
  return { identity, releases, request: async (method, _identity, p) => {
    let result;
    if (method === 'video.record.begin') result = { ...metadata, snapshot_id: 1 };
    else if (method === 'video.record.read') result = { snapshot_id: 1, offset: p.offset, length: p.length, hex: bytes.subarray(p.offset, p.offset + p.length).toString('hex') };
    else if (method === 'video.record.release') { releases.push(p.consume); result = { released: true }; }
    else result = metadata;
    return { protocol: 1, id: 'avi', ...identity, ok: true, result };
  } };
}
test('streamed AVI DIB headers, row padding and index decode with ffprobe; failed export retains native frames', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-avi-');
  try {
    const service = new RecordingArtifacts(), native = clientFor(), file = path.join(dir, 'movie.avi');
    const saved = await service.stop(native, identity, file, false, 'owner', () => true), avi = await fs.readFile(file), parts = aviParts(metadata);
    assert.equal(avi.readUInt32LE(4), avi.length - 8); assert.equal(avi.length, parts.size);
    assert.equal(avi.toString('ascii', 8, 12), 'AVI '); assert.equal(avi.toString('ascii', 224, 228), '00db');
    assert.equal(avi.readUInt32LE(228), 24); assert.equal(avi.toString('ascii', avi.length - 40, avi.length - 36), 'idx1');
    assert.equal(avi.readUInt32LE(avi.length - 24), 4); assert.equal(avi.readUInt32LE(avi.length - 8), 36);
    decodeReply(JSON.stringify({ protocol: 1, id: 'export', ...identity, ok: true, result: saved }), 'artifact.videoStop');
    assert.deepEqual(native.releases, [true]);
    const ffprobe = process.env.ARES_FFPROBE || '/opt/homebrew/bin/ffprobe';
    if (require('node:fs').existsSync(ffprobe)) {
      const decoded = JSON.parse(execFileSync(ffprobe, ['-v', 'error', '-show_streams', '-of', 'json', file], { encoding: 'utf8' })).streams[0];
      assert.equal(decoded.codec_name, 'rawvideo'); assert.equal(decoded.width, 3); assert.equal(decoded.height, 2); assert.equal(decoded.pix_fmt, 'bgr24'); assert.equal(decoded.nb_frames, '2'); assert.equal(decoded.r_frame_rate, '50/1');
      const ffmpeg = path.join(path.dirname(ffprobe), 'ffmpeg');
      const rgb = execFileSync(ffmpeg, ['-v', 'error', '-i', file, '-frames:v', '1', '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-']);
      assert.deepEqual([...rgb], [255,0,0, 0,255,0, 0,0,255, 0,0,255, 0,255,0, 255,0,0]);
    }
    await assert.rejects(service.stop(native, identity, file, false, 'owner', () => true), { code: 'file_exists' });
    assert.deepEqual(native.releases, [true, false]);
    const again = await service.stop(native, identity, file, true, 'owner', () => true); assert.equal(again.size, avi.length);
    assert.deepEqual((await fs.readdir(dir)), ['movie.avi']);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('recording export revocation removes temporary output and retains native frames', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-avi-cancel-');
  try {
    let enter, resume; const started = new Promise(r => enter = r), waiting = new Promise(r => resume = r);
    const native = clientFor(), original = native.request;
    native.request = async (...args) => { if (args[0] === 'video.record.read') { enter(); await waiting; } return original(...args); };
    const service = new RecordingArtifacts(), pending = service.stop(native, identity, path.join(dir, 'cancelled.avi'), false, 'owner', () => true);
    await started; service.release('owner'); resume(); await assert.rejects(pending, { code: 'artifact_cancelled' });
    assert.deepEqual(native.releases, [false]); assert.deepEqual(await fs.readdir(dir), []);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
