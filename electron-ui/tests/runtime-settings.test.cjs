const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const { makeFixture } = require('../fixture.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../build-electron-make/bin/ares-electron-host');
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
const unwrap = reply => { assert.equal(reply.ok, true, JSON.stringify(reply)); return reply.result; };

test('native live settings affect GB pixels and PCM, and preserve session', { timeout: 15000 }, async () => {
  const { NativeSessionClient } = await import('../dist/services/native-client.js');
  const directory = fs.mkdtempSync('/tmp/ares-runtime-settings-');
  const rom = path.join(directory, 'fixture.gb'); fs.writeFileSync(rom, makeFixture());
  const identity = { session: 'runtime-settings', epoch: 1 };
  const child = spawn(host, ['--rom', rom, '--system', 'Game Boy', '--silent', '--paused', '--save-dir', path.join(directory, 'saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', data => { log += data; }); child.stdout.resume();
  const client = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  try {
    const defaults = unwrap(await client.request('runtime.settings'));
    assert.deepEqual(defaults, { volume: 0.4, mute: false, luminance: 1, saturation: 1, gamma: 1 });
    const desired = { volume: 0, mute: true, luminance: 0, saturation: 0.5, gamma: 1.5 };
    assert.deepEqual(unwrap(await client.request('runtime.configure', undefined, desired)), desired);
    assert.deepEqual(unwrap(await client.request('runtime.settings')), desired);
    const snapshot = async () => {
      const frame = unwrap(await client.request('video.begin', undefined, { source: 'presented' }));
      const read = unwrap(await client.request('video.read', undefined, { snapshotId: frame.snapshot_id, offset: 0, length: 4096 }));
      unwrap(await client.request('video.release', undefined, { snapshotId: frame.snapshot_id }));
      return Buffer.from(read.hex, 'hex');
    };
    const capture = async () => {
      unwrap(await client.request('audio.capture', undefined, { frames: 1024 }));
      unwrap(await client.request('session.resume'));
      for (let i = 0; i < 200; i++) { if (unwrap(await client.request('audio.status')).done) break; await pause(5); }
      unwrap(await client.request('session.pause'));
      const info = unwrap(await client.request('audio.begin'));
      assert.equal(info.frames, 1024);
      const read = unwrap(await client.request('audio.read', undefined, { snapshotId: info.snapshot_id, offset: 0, length: info.bytes }));
      unwrap(await client.request('audio.release', undefined, { snapshotId: info.snapshot_id, consume: true }));
      return Buffer.from(read.hex, 'hex');
    };
    assert.ok((await capture()).every(byte => byte === 0), 'volume zero silences actual post-mix PCM');
    let dark;
    for (let i = 0; i < 50; i++) {
      try { dark = await snapshot(); break; } catch { await client.request('session.resume'); await pause(10); await client.request('session.pause'); }
    }
    assert.ok(dark, 'a frame was presented');
    assert.ok([...dark].every((byte, i) => i % 4 === 3 || byte === 0), 'zero luminance produces black RGB pixels');
    const audible = { ...defaults, mute: true };
    unwrap(await client.request('runtime.configure', undefined, audible));
    assert.ok((await capture()).some(byte => byte !== 0), 'restored volume yields PCM even when speaker mute is set');
    const light = await snapshot();
    assert.ok([...light].some((byte, i) => i % 4 !== 3 && byte !== 0), 'palette update changes actual rendered pixels');
    const stale = await client.request('runtime.configure', { session: 'old', epoch: 1 }, desired);
    assert.equal(stale.error, 'stale_session');
    assert.deepEqual(unwrap(await client.request('runtime.settings')), audible);
    const status = await client.request('session.status');
    assert.equal(status.session, identity.session); assert.equal(status.epoch, 1);
    await assert.rejects(client.request('runtime.configure', undefined, { ...defaults, gamma: NaN }), /invalid_request/);
    client.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    client.close(); if (child.exitCode === null && child.signalCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(directory, { recursive: true, force: true });
  }
});


test('native settings refuse changes during reserved or active execution without partial effects', { timeout: 5000 }, async () => {
  const { NativeSessionClient } = await import('../dist/services/native-client.js');
  const directory = fs.mkdtempSync('/tmp/ares-runtime-settings-busy-');
  const identity = { session: 'runtime-settings-busy', epoch: 1 };
  const child = spawn(host, ['--synthetic', '--silent', '--paused', '--save-dir', path.join(directory, 'saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); child.stdout.resume(); child.stderr.resume();
  const client = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  try {
    const original = unwrap(await client.request('runtime.settings'));
    const desired = { ...original, volume: 0.8, gamma: 2 };
    unwrap(await client.request('execution.batchBegin', undefined, { timeoutMs: 2000 }, 'batch-settings'));
    assert.equal((await client.request('runtime.configure', undefined, desired)).error, 'busy');
    assert.equal((await client.request('runtime.configure', undefined, desired, 'batch-settings')).error, 'busy');
    assert.deepEqual(unwrap(await client.request('runtime.settings')), original);
    unwrap(await client.request('execution.release', undefined, undefined, 'batch-settings'));
    unwrap(await client.request('execution.runFrames', undefined, { count: 100000, timeoutMs: 2000 }, 'runner'));
    assert.equal((await client.request('runtime.configure', undefined, desired)).error, 'busy');
    assert.deepEqual(unwrap(await client.request('runtime.settings')), original);
    unwrap(await client.request('execution.cancel', undefined, { operationId: 0 }));
    assert.deepEqual(unwrap(await client.request('runtime.configure', undefined, desired)), desired);
    client.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0);
  } finally {
    client.close(); if (child.exitCode === null && child.signalCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(directory, { recursive: true, force: true });
  }
});

test('persisted core descriptors control the actual N64 executor and Expansion Pak at load', { timeout: 10000 }, async () => {
  const { NativeSessionClient } = await import('../dist/services/native-client.js');
  const { SettingsStore } = await import('../dist/settings/store.js');
  const { coreDescriptors, nativeSettingsBml } = await import('../dist/settings/native.js');
  const directory = fs.mkdtempSync('/tmp/ares-core-settings-');
  try {
    const image = Buffer.alloc(4096); image.writeUInt32BE(0x80371240); image[0x3e] = 0x45;
    image.writeUInt32BE(0x1000ffff, 0x40); // Original IPL3 loop: branch to itself; nop.
    const rom = path.join(directory, 'fixture.z64'); fs.writeFileSync(rom, image);
    const descriptors = coreDescriptors({ firmware: [], imported: {}, features: { n64Gpu: false } });
    assert.equal(descriptors.some(d => d.id === 'n64.quality'), false, 'GPU-only options absent from software build controls');
    assert.equal(coreDescriptors({ firmware: [], imported: {}, features: { n64Gpu: true } }).some(d => d.id === 'n64.quality'), true);
    const file = path.join(directory, 'preferences.json');
    const store = new SettingsStore(file, descriptors);
    await store.set('core.forceInterpreter', true); await store.set('n64.expansionPak', false);
    const restarted = new SettingsStore(file, descriptors);
    const overlay = path.join(directory, 'launch.bml'); fs.writeFileSync(overlay, nativeSettingsBml(restarted.catalog()));
    const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--settings-overlay', overlay, '--silent', '--paused', '--save-dir', path.join(directory, 'saves'), '--control-session', 'core-settings'], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
    const closed = once(child, 'close'); child.stdout.resume(); let log = ''; child.stderr.on('data', data => { log += data; });
    const client = new NativeSessionClient(child.stdin, child.stdio[3], { session: 'core-settings', epoch: 1 }, () => {});
    try {
      const status = unwrap(await client.request('session.status'));
      assert.equal(status.debug_mode.recompiler_enabled, false);
      assert.equal(status.debug_mode.expansion_pak, false);
      assert.equal(status.debug_mode.rdram_size, 4 * 1024 * 1024);
      client.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
    } finally {
      client.close(); if (child.exitCode === null && child.signalCode === null) { child.kill('SIGKILL'); await closed; }
    }
  } finally { fs.rmSync(directory, { recursive: true, force: true }); }
});
