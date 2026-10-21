'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const { createHash } = require('node:crypto');
const { makeFixture } = require('../fixture.cjs');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost } = require('../dist/services/local-endpoint.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../build-electron-make/bin/ares-electron-host');
const unwrap = r => { assert.equal(r.ok, true, JSON.stringify(r)); return r.result; };
const pause = ms => new Promise(r => setTimeout(r, ms));
test('real GB post-mix audio reaches MCP WAV exports with host output muted; ownership/reset/disconnect revoke captures', { timeout: 25000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-native-audio-'), identity = { session: 'native-audio-fixture', epoch: 1 };
  const rom = path.join(dir, 'fixture.gb'), settings = path.join(dir, 'settings.bml'); fs.writeFileSync(rom, makeFixture()); fs.writeFileSync(settings, '');
  const child = spawn(host, ['--rom', rom, '--system', 'Game Boy', '--settings', settings, '--silent', '--paused', '--save-dir', path.join(dir, 'saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log += b); child.stdout.resume();
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  const router = new SessionRouter(); router.attach(native); const endpoint = new LocalServiceHost(router); let peer;
  try {
    unwrap(await native.request('session.status')); await endpoint.listen(path.join(dir, 'ipc'));
    peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); await peer.initialize();
    assert.equal((await peer.tool('audio_save', { path: path.join(dir, 'missing.wav') })).error, 'no_capture');
    const capture = await peer.tool('audio_capture', { seconds: 3600 }); assert.equal(capture.cap, 192000); assert.equal(capture.seconds, 4);
    assert.equal((await native.request('audio.capture', undefined, { frames: 1 }, 'foreign')).error, 'busy');
    await peer.tool('resume');
    for (let n = 0; n < 200; n++) { const status = await peer.tool('audio_status'); if (status.done) break; await pause(25); }
    await peer.tool('pause'); const full = await peer.tool('audio_status'); assert.equal(full.done, true); assert.equal(full.count, 192000);
    await pause(20); assert.equal((await peer.tool('audio_status')).count, 192000);
    const failed = await peer.tool('audio_save', { path: dir }); assert.equal(failed.error, 'write_failed'); assert.equal((await peer.tool('audio_status')).active, true);
    const file = path.join(dir, 'capture.wav'); fs.writeFileSync(file, 'replace');
    const saved = await peer.tool('audio_save', { path: file }), wav = fs.readFileSync(file);
    assert.equal(saved.frames, 192000); assert.equal(saved.rate, 48000); assert.equal(saved.seconds, 4); assert.equal(saved.epoch, 1);
    assert.equal(wav.length, 768044); assert.equal(wav.toString('ascii', 0, 4), 'RIFF'); assert.equal(wav.readUInt32LE(40), 768000);
    assert.equal(createHash('sha256').update(wav).digest('hex'), saved.sha256);
    assert.ok(wav.subarray(44).some(byte => byte !== 0), 'ROM-free GB tone produces real nonzero samples');
    assert.equal((await peer.tool('audio_status')).active, false);
    await peer.tool('audio_capture', { seconds: 0 }); const empty = await peer.tool('audio_save', { path: file }); assert.equal(empty.frames, 0); assert.equal(fs.statSync(file).size, 44);
    await peer.tool('audio_capture'); unwrap(await native.request('session.reset'));
    assert.equal((await peer.tool('audio_status')).active, false); assert.equal((await peer.tool('audio_save', { path: file })).error, 'no_capture');
    await peer.tool('audio_capture'); await peer.close(); peer = undefined;
    for (let n = 0; n < 50 && unwrap(await native.request('audio.status')).active; n++) await pause(10);
    assert.equal(unwrap(await native.request('audio.status')).active, false);
    await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); await endpoint.close(); router.detach(); native.close();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
