'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const { createHash } = require('node:crypto');
const { inflateSync } = require('node:zlib');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost, LocalServiceClient } = require('../dist/services/local-endpoint.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const unwrap = r => { assert.equal(r.ok, true, JSON.stringify(r)); return r.result; };
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
async function runFrame(native) {
  const operation = unwrap(await native.request('execution.runFrames', undefined, { count: 1, timeoutMs: 1000 }));
  for (let n = 0; n < 100; n++) {
    const final = unwrap(await native.request('execution.status', undefined, { operationId: operation.operation_id }));
    if (final.state === 'done') { assert.equal(final.stop_reason, 'frames_done'); return final; } await pause(5);
  }
  throw new Error('VI fixture did not finish');
}
test('real native screenshot and MCP image content do not depend on canvas credits; reset and owner cleanup stay isolated', { timeout: 20000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-native-image-'), identity = { session: 'native-image-fixture', epoch: 1 };
  const child = spawn(host, ['--synthetic', '--silent', '--paused', '--debug-capture', '--save-dir', dir, '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log += b); child.stdout.resume(); // Never issue ack.
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  const router = new SessionRouter(); router.attach(native); const endpoint = new LocalServiceHost(router);
  let client, peer;
  const captureParams = { source: 'presented', path: null, inline: true, overwrite: false, region: [0, 0, 2, 2], scale: 1 };
  try {
    unwrap(await native.request('session.status')); await endpoint.listen(path.join(dir, 'ipc'));
    client = new LocalServiceClient(endpoint.metadataFile, endpoint.instance); unwrap(await client.request('session.status'));
    assert.equal((await client.request('artifact.screenshot', undefined, captureParams)).error, 'no_frame');
    await runFrame(native);
    const image = unwrap(await client.request('artifact.screenshot', undefined, captureParams));
    assert.equal(image.frame, 1); assert.equal(image.current_frame, 1); assert.equal(image.source, 'presented');
    const data = unwrap(await client.request('artifact.imageRead', undefined, { imageId: image.image_id, offset: 0, length: image.bytes }));
    const bytes = Buffer.from(data.base64, 'base64'); assert.equal(bytes.subarray(0, 8).toString('hex'), '89504e470d0a1a0a');
    assert.equal(createHash('sha256').update(bytes).digest('hex'), image.sha256);
    unwrap(await client.request('artifact.imageRelease', undefined, { imageId: image.image_id }));
    peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); await peer.initialize();
    const response = await peer.request('tools/call', { name: 'screenshot', arguments: { source: 'presented', region: [0, 0, 2, 2], inline: true } });
    assert.equal(response.result.isError, undefined);
    const imageContent = response.result.content.find(c => c.type === 'image'), metadata = JSON.parse(response.result.content.find(c => c.type === 'text').text);
    assert.equal(imageContent.mimeType, 'image/png'); assert.equal(metadata.presented_at_frame, 1); assert.equal(metadata.frame_kind, 'synthetic-vi');
    const png = Buffer.from(imageContent.data, 'base64');
    assert.equal(createHash('sha256').update(png).digest('hex'), metadata.sha256);
    const compressed = []; for (let off = 8; off < png.length;) { const length = png.readUInt32BE(off); if (png.toString('ascii', off + 4, off + 8) === 'IDAT') compressed.push(png.subarray(off + 8, off + 8 + length)); off += 12 + length; }
    assert.deepEqual([...inflateSync(Buffer.concat(compressed))], [0, 0,0,0, 1,0,0, 0, 0,1,0, 1,1,0]);
    assert.equal((await peer.tool('screenshot')).error, 'unavailable', 'synthetic source does not pretend to be an N64 framebuffer');
    await runFrame(native);
    const destination = path.join(dir, 'native capture.png');
    const saved = await peer.tool('screenshot', { source: 'presented', path: destination, region: [0, 0, 2, 2] });
    assert.equal(saved.frame, 2); assert.equal(fs.statSync(destination).size, saved.bytes);
    assert.equal((await peer.tool('screenshot', { source: 'presented', path: destination })).error, 'file_exists');
    assert.equal((await peer.tool('screenshot', { source: 'presented', region: [639, 479, 2, 2] })).error, 'invalid_region');
    const oldIdentity = { ...native.identity }; unwrap(await native.request('session.reset'));
    assert.equal((await client.request('artifact.screenshot', oldIdentity, captureParams)).error, 'stale_epoch');
    unwrap(await client.request('session.status'));
    assert.equal((await client.request('artifact.screenshot', undefined, captureParams)).error, 'no_frame', 'reset cannot reuse the old native frame');
    await runFrame(native);
    const fresh = unwrap(await client.request('artifact.screenshot', undefined, captureParams)); assert.equal(fresh.epoch, 2); assert.equal(fresh.frame, 1);
    client.disconnect(); await pause(30); unwrap(await client.request('session.status'));
    assert.equal((await client.request('artifact.imageRead', undefined, { imageId: fresh.image_id, offset: 0, length: 1 })).error, 'stale_image');
    await peer.close(); peer = undefined; client.close(); await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n');
    assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); client?.close(); await endpoint.close(); router.detach(); native.close();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
