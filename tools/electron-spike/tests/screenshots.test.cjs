'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { inflateSync } = require('node:zlib');
const { execFileSync } = require('node:child_process');
const { Screenshots } = require('../dist/services/screenshots.js');
const { decodeReply, encodeRequest } = require('../dist/services/contracts.js');
const identity = { session: 'screenshot-fixture', epoch: 1 };
const pixels = Buffer.from([255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255, 0,0,0,255, 255,128,0,255]);
function clientFor(data = pixels, width = 3, height = 2, waitRead = async () => {}) {
  let released = 0;
  return { identity: { ...identity }, get released() { return released; }, request: async (method, _identity, p) => {
    let result;
    if (method === 'video.begin') result = { snapshot_id: 1, width, height, bytes: data.length, epoch: 1, frame: 4, current_frame: 5, frame_kind: 'vi', sequence: 7, source: p.source, origin: p.source === 'framebuffer' ? '0x0' : null, color_depth: p.source === 'framebuffer' ? 3 : 0, nonblack_pixels: p.source === 'framebuffer' ? 5 : null, format: 'rgba8' };
    else if (method === 'video.read') { await waitRead(); result = { snapshot_id: 1, offset: p.offset, length: p.length, hex: data.subarray(p.offset, p.offset + p.length).toString('hex') }; }
    else { released++; result = { released: true }; }
    return { protocol: 1, id: 'image', ...identity, ok: true, result };
  } };
}
const params = { source: 'presented', path: null, region: null, scale: 1, inline: true, overwrite: false };
function pngPixels(bytes) {
  assert.equal(bytes.subarray(0, 8).toString('hex'), '89504e470d0a1a0a');
  const compressed = []; let width, height;
  for (let at = 8; at < bytes.length;) {
    const length = bytes.readUInt32BE(at), type = bytes.toString('ascii', at + 4, at + 8);
    const data = bytes.subarray(at + 8, at + 8 + length);
    if (type === 'IHDR') { width = data.readUInt32BE(0); height = data.readUInt32BE(4); assert.equal(data[8], 8); assert.equal(data[9], 2); }
    if (type === 'IDAT') compressed.push(data); at += 12 + length;
  }
  const raw = inflateSync(Buffer.concat(compressed)), rgb = [];
  for (let y = 0; y < height; y++) { assert.equal(raw[y * (width * 3 + 1)], 0); rgb.push(...raw.subarray(y * (width * 3 + 1) + 1, (y + 1) * (width * 3 + 1))); }
  return { width, height, rgb };
}
test('native screenshot pipeline: worker PNG is decodable, crop then nearest scale, image ownership/release and atomic files', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-png-');
  try {
    const service = new Screenshots(), native = clientFor(); const file = path.join(dir, 'capture.png');
    const capture = await service.capture(native, identity, { ...params, path: file, region: [1, 0, 2, 2], scale: 2 }, 'owner', () => true);
    const decoded = decodeReply(JSON.stringify({ protocol: 1, id: 'capture', ...identity, ok: true, result: capture }), 'artifact.screenshot').result;
    assert.equal(decoded.width, 4); assert.equal(decoded.height, 4); assert.equal(native.released, 1);
    assert.throws(() => service.read(capture.image_id, 0, capture.bytes, 'foreign', identity), { code: 'busy' });
    assert.throws(() => service.read(capture.image_id, 0, capture.bytes, 'owner', { ...identity, epoch: 2 }), { code: 'stale_epoch' });
    const image = Buffer.from(service.read(capture.image_id, 0, capture.bytes, 'owner', identity).base64, 'base64');
    const output = pngPixels(image);
    assert.deepEqual(output.rgb.slice(0, 12), [0,255,0, 0,255,0, 0,0,255, 0,0,255]);
    assert.deepEqual(output.rgb.slice(24, 36), [0,0,0, 0,0,0, 255,128,0, 255,128,0]);
    assert.deepEqual(await fs.readFile(file), image);
    if (process.platform === 'darwin') assert.match(execFileSync('/usr/bin/sips', ['-g', 'pixelWidth', '-g', 'pixelHeight', file], { encoding: 'utf8' }), /pixelWidth: 4[\s\S]*pixelHeight: 4/);
    assert.deepEqual(service.releaseImage(capture.image_id, 'owner'), { released: true });
    assert.throws(() => service.read(capture.image_id, 0, capture.bytes, 'owner', identity), { code: 'stale_image' });
    await assert.rejects(service.capture(native, identity, { ...params, inline: false, path: file }, 'owner', () => true), { code: 'file_exists' });
    assert.deepEqual(await fs.readFile(file), image, 'existing image survives refused overwrite');
    await assert.rejects(service.capture(native, identity, { ...params, region: [2, 1, 2, 1] }, 'owner', () => true), { code: 'invalid_region' });
    const large = clientFor(Buffer.alloc(1024 * 1024 * 4), 1024, 1024);
    await assert.rejects(service.capture(large, identity, { ...params, scale: 8 }, 'owner', () => true), { code: 'image_too_large' });
    assert.equal(large.released, 1);
    const small = await service.capture(native, identity, { ...params, scale: 0.125 }, 'owner', () => true);
    assert.equal(small.width, 1); assert.equal(small.height, 1); service.release('owner');
    assert.throws(() => service.read(small.image_id, 0, small.bytes, 'owner', identity), { code: 'stale_image' });
    assert.deepEqual(await fs.readdir(dir), ['capture.png']);
    assert.throws(() => encodeRequest('x', identity, 'artifact.screenshot', { ...params, scale: 9 }), /invalid_request/);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('screenshot disconnect cancels pending transfer and releases native pin without publishing', async () => {
  const service = new Screenshots(); let resume, entered;
  const waiting = new Promise(resolve => resume = resolve), started = new Promise(resolve => entered = resolve);
  const native = clientFor(pixels, 3, 2, async () => { entered(); await waiting; });
  const capture = service.capture(native, identity, params, 'owner', () => true);
  await started; service.release('owner'); resume();
  await assert.rejects(capture, { code: 'artifact_cancelled' }); assert.equal(native.released, 1);
});
