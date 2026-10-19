'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const { mkdtempSync, rmSync } = require('node:fs');
const { tmpdir } = require('node:os');
const path = require('node:path');
const { METHODS } = require('../dist/services/contracts.js');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { FrameParser } = require('../protocol.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

test('native safe-point control, stale identities and stalled video', { timeout: 15000 }, async () => {
  const saves = mkdtempSync(path.join(tmpdir(), 'ares-session-'));
  const identity = { session: 'isolated-session', epoch: 1 };
  const child = spawn(host, ['--synthetic', '--silent', '--save-dir', saves, '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close');
  let log = '', frames = [];
  child.stderr.on('data', b => log += b);
  child.stdout.on('data', b => parser.push(b));
  const parser = new FrameParser(frame => frames.push(frame));
  let failure;
  const client = new NativeSessionClient(child.stdin, child.stdio[3], identity, error => { failure = error; });
  try {
    const status = await client.request('session.status');
    assert.equal(status.result.paused, false);
    assert.deepEqual(status.result.capabilities, METHODS);
    assert.equal(status.protocol, 1);
    assert.equal(status.result.system, 'Synthetic');
    // No canvas credits have ever been issued. Control must still complete.
    assert.equal(frames.length, 0);
    assert.equal((await client.request('session.pause')).result.paused, true);
    assert.equal((await client.request('session.status')).result.paused, true);
    const stale = await client.request('session.resume', { session: 'old-session', epoch: 1 });
    assert.equal(stale.error, 'stale_session');
    assert.equal((await client.request('session.status')).result.paused, true);
    // Drain the one pre-pause latest frame; then no new frames are produced.
    child.stdin.write('ack\n');
    for (let i = 0; i < 50 && frames.length === 0; i++) await sleep(10);
    assert.equal(frames.length, 1);
    child.stdin.write('ack\n'); await sleep(100);
    assert.equal(frames.length, 1);
    assert.equal((await client.request('session.reset')).epoch, 2);
    assert.equal((await client.request('session.resume', identity)).error, 'stale_epoch');
    assert.equal((await client.request('session.status')).result.paused, true);
    assert.equal((await client.request('session.resume')).result.paused, false);
    for (let i = 0; i < 50 && frames.length === 1; i++) await sleep(10);
    assert.equal(frames.length, 2);
    // No acknowledgements: native execution continues without reply coupling.
    await sleep(100);
    assert.equal((await client.request('session.pause')).result.paused, true);
    assert.equal(frames.length, 2);
    assert.equal(failure, undefined);
    client.close();
    child.stdin.end('quit\n');
    const [code] = await closed;
    assert.equal(code, 0, log);
    await assert.rejects(client.request('session.resume'), /native_host_disconnected/);
  } finally {
    client.close();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    rmSync(saves, { recursive: true, force: true });
  }
});

test('native boundary rejects invalid versions, arguments and methods', { timeout: 10000 }, async () => {
  const saves = mkdtempSync(path.join(tmpdir(), 'ares-invalid-'));
  const child = spawn(host, ['--synthetic', '--silent', '--save-dir', saves, '--control-session', 'validation'], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close');
  child.stdout.resume(); child.stderr.resume();
  let buffer = '', replies = [];
  child.stdio[3].on('data', data => {
    buffer += data;
    let end;
    while ((end = buffer.indexOf('\n')) >= 0) {
      replies.push(JSON.parse(buffer.slice(0, end))); buffer = buffer.slice(end + 1);
    }
  });
  try {
    const requests = [
      'control 99 a validation 1 session.pause',
      'control 1 b validation 1 session.pause extra',
      'control 1 c validation 1 memory.unimplemented',
      'control 1 d validation 1',
      'wrong-prefix 1 e validation 1 session.pause',
      'control 1 f validation 1 session.status',
    ];
    // Deliberately fragment a request across pipe writes.
    child.stdin.write('con'); child.stdin.write(requests.join('\n').slice(3) + '\n');
    for (let i = 0; i < 500 && replies.length < requests.length; i++) await sleep(10);
    assert.deepEqual(replies.map(r => r.error), ['invalid_request', 'invalid_request', 'unavailable', 'invalid_request', 'invalid_request', undefined]);
    assert.equal(replies.at(-1).result.paused, false);
    child.stdin.end('quit\n');
    assert.equal((await closed)[0], 0);
  } finally {
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    rmSync(saves, { recursive: true, force: true });
  }
});
