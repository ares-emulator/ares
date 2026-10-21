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
test('execution requests bound targets, deadlines, IDs and internal owner tokens', () => {
  assert.match(encodeRequest('op', identity, 'execution.runFrames', { count: 2, timeoutMs: 60000 }, 'connection-1'), /execution.runFrames 2 60000 owner=connection-1\n$/);
  for (const params of [{ count: -1, timeoutMs: 100 }, { count: 1000001, timeoutMs: 100 }, { count: 1, timeoutMs: 0 }, { count: 1, timeoutMs: 600001 }, { count: 1, timeoutMs: 1, owner: 'spoof' }])
    assert.throws(() => encodeRequest('op', identity, 'execution.runFrames', params), /invalid_request/);
  assert.throws(() => encodeRequest('op', identity, 'execution.status', { operationId: 0.5 }), /invalid_request/);
  assert.throws(() => encodeRequest('op', identity, 'execution.runFrames', { count: 1, timeoutMs: 100 }, 'owner\nquit'), /invalid_request/);
});
test('owner disconnects have reserved capacity and fail closed if revocation cannot be queued', async () => {
  const input = new PassThrough(), output = new PassThrough(); let failure;
  const client = new NativeSessionClient(input, output, identity, e => { failure = e; });
  const pending = Array.from({ length: 32 }, () => client.request('session.status').catch(e => e.code));
  await assert.rejects(client.request('session.status'), /busy/);
  const release = client.request('execution.release', undefined, undefined, 'departed-owner');
  output.write(JSON.stringify({ protocol: 1, id: 'req33', ...identity, ok: true, result: { released: true } }) + '\n');
  assert.equal((await release).result.released, true);
  for (let i = 0; i < 8; i++) pending.push(client.request('execution.release', undefined, undefined, `owner-${i}`).catch(e => e.code));
  await assert.rejects(client.request('execution.release', undefined, undefined, 'overflow-owner'), /owner_revocation_failed/);
  assert.equal(failure.code, 'owner_revocation_failed');
  assert.ok((await Promise.all(pending)).every(code => code === 'owner_revocation_failed'));
});

test('main observes a native CPU stop without a renderer request or frame', { timeout: 2000 }, async () => {
  const input = new PassThrough(), output = new PassThrough();
  const client = new NativeSessionClient(input, output, identity, () => {});
  let paused = false;
  input.on('data', wire => {
    const [, , id] = String(wire).trim().split(' ');
    output.write(JSON.stringify({ ...reply(id), result: { ...reply(id).result, paused } }) + '\n');
  });
  let observe;
  const observed = new Promise(resolve => { observe = resolve; });
  const router = new SessionRouter(state => { if (state.result.paused) observe(state); });
  router.attach(client);
  const deadline = setTimeout(() => observe(undefined), 1000);
  try {
    paused = true; // Core changes state; the renderer makes no request.
    assert.equal((await observed)?.result.paused, true);
  } finally { clearTimeout(deadline); router.detach(); }
});

// Migrated test_bridge.py wire-integrity assertions, on the new native protocol.
for(const [name,wire,code] of [
  ['mismatched reply ID',JSON.stringify(reply('wrong'))+'\n','unexpected_reply'],
  ['wrong session',JSON.stringify({...reply('req1'),session:'other'})+'\n','wrong_session'],
  ['oversized fragmented reply','x'.repeat(65537),'reply_too_large'],
  ['malformed JSON','{broken}\n',undefined],
]) test(`bridge integrity: ${name} drops all pending requests without replay`,async()=>{
  const input=new PassThrough(),output=new PassThrough();let sent='',failure;
  input.on('data',b=>sent+=b);const client=new NativeSessionClient(input,output,identity,e=>{failure=e;});
  const pending=client.request('session.pause');output.write(wire);
  await assert.rejects(pending);if(code)assert.equal(failure.code,code);
  await assert.rejects(client.request('session.pause'),/native_host_disconnected/);assert.equal(sent.split('\n').filter(Boolean).length,1);
});
test('bridge integrity: native error code is preserved without retry or connection loss',async()=>{
  const input=new PassThrough(),output=new PassThrough();let sent=0;input.on('data',()=>++sent);
  const client=new NativeSessionClient(input,output,identity,e=>assert.fail(e.message));
  const result=client.request('session.pause');output.write(JSON.stringify({protocol:1,id:'req1',...identity,ok:false,error:'busy'})+'\n');
  assert.equal((await result).error,'busy');assert.equal(sent,1);client.close();
});
