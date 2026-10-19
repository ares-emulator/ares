'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { PassThrough } = require('node:stream');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { decodeReply, encodeRequest } = require('../dist/services/contracts.js');
const identity = { session: 'test-session', epoch: 1 };
const reply = id => ({ protocol: 1, id, ...identity, ok: true, result: { paused: false, system: 'Synthetic', synthetic: true, capabilities: ['session.status'] } });

test('correlates fragmented/coalesced replies and out-of-order request IDs', async () => {
  const input = new PassThrough(), output = new PassThrough();
  const client = new NativeSessionClient(input, output, identity, error => assert.fail(error.message));
  const a = client.request('session.status'), b = client.request('session.pause');
  const lines = JSON.stringify(reply('req2')) + '\n' + JSON.stringify(reply('req1')) + '\n';
  output.write(lines.slice(0, 15)); output.write(lines.slice(15));
  assert.equal((await a).id, 'req1'); assert.equal((await b).id, 'req2'); client.close();
});
test('timeout rejects all pending work, closes the connection and never replays', async () => {
  const input = new PassThrough(), output = new PassThrough(); let sent = '', failure;
  input.on('data', b => sent += b);
  const client = new NativeSessionClient(input, output, identity, error => { failure = error; }, 20);
  await assert.rejects(client.request('session.pause'), /request_timeout/);
  await assert.rejects(client.request('session.pause'), /native_host_disconnected/);
  assert.equal(sent.split('\n').filter(Boolean).length, 1);
  assert.equal(failure.code, 'request_timeout');
});
test('disconnection rejects pending requests; router detach revokes old connection', async () => {
  const input = new PassThrough(), output = new PassThrough();
  const client = new NativeSessionClient(input, output, identity, () => {});
  const router = new SessionRouter(); router.attach(client);
  const pending = router.request('session.status'); output.end();
  await assert.rejects(pending, /native_host_disconnected/);
  router.detach();
  await assert.rejects(router.request('session.pause'), /native_host_disconnected/);
  await assert.rejects(router.request('n64.debug'), /invalid_request/);
});
test('schema validation rejects malformed envelopes, identities and methods', () => {
  for (const invalid of ['null', '{}', '{', JSON.stringify({ ...reply('a'), protocol: 9 }), JSON.stringify({ ...reply('a'), result: { paused: 'yes' } })])
    assert.throws(() => decodeReply(invalid), /invalid_reply/);
  assert.throws(() => encodeRequest('a', { ...identity, session: 'injected\nquit' }, 'session.pause'), /invalid_request/);
  assert.throws(() => encodeRequest('a', identity, 'unknown'), /invalid_request/);
});
