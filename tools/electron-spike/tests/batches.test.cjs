'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const { Batches } = require('../dist/services/batches.js');
const identity = { session: 'batch-contract', epoch: 1 };
const capture = { kind: 'screenshot', path: '/tmp/test.png', inline: false, overwrite: false, source: 'framebuffer', region: null, scale: 1 };
const params = { steps: [{ kind: 'run', n: 0 }], timeoutMs: 1000, stopOnError: true };
test('batch contracts reject nesting, extra keys, malformed later payloads and aggregate bounds', () => {
  assert.match(encodeRequest('x', identity, 'experiment.batch', { ...params, steps: [capture] }), /experiment.batch owner=ui/);
  for (const steps of [[{ kind: 'batch', steps: [] }], [{ kind: 'run', n: 1, parallel: true }], [capture, { kind: 'write', address: '0x2000', view: 'cpu', hex: 'zz' }],
    Array(33).fill(params.steps[0]), Array(2).fill({ kind: 'run', n: 1000000 }), Array(17).fill({ kind: 'read', address: '0x2000', view: 'rdram', length: 1024 }), Array(9).fill(capture), [{ ...capture, inline: true }]]) {
    assert.throws(() => encodeRequest('x', identity, 'experiment.batch', { ...params, steps }), /invalid_request/);
  }
  assert.doesNotThrow(() => encodeRequest('x',identity,'experiment.batch',{...params,timeoutMs:600000}));
  assert.throws(() => encodeRequest('x', identity, 'experiment.batch', { ...params, timeoutMs: 600001 }), /invalid_request/);
  assert.throws(() => decodeReply(JSON.stringify({ protocol: 1, id: 'x', ...identity, ok: true, result: { side_effects_rolled_back: true } }), 'experiment.batch'), /invalid_reply/);
});
function nativeFixture() {
  let reserved = false, released = 0;
  const native = { identity: { ...identity }, get released() { return released; }, async request(method) {
    let result;
    if (method === 'execution.batchBegin') { reserved = true; result = { batch_id: 1 }; }
    else if (method === 'execution.batchCheck') { if (!reserved) return { protocol: 1, id: 'x', ...native.identity, ok: false, error: 'batch_interrupted' }; result = { active: true }; }
    else if (method === 'session.status') result = { frame: 0 };
    else if (method === 'execution.release') { reserved = false; released++; result = { released: true }; }
    else throw new Error(method);
    return { protocol: 1, id: 'x', ...native.identity, ok: true, result };
  } }; return native;
}
test('batch records a committed artifact even if disconnect wins its continuation; later steps never execute', async () => {
  const batches = new Batches(), native = nativeFixture(); let started, complete, calls = 0;
  const ready = new Promise(r => started = r), delayed = new Promise(r => complete = r);
  const invoke = { async request() { calls++; started(); await delayed; return { protocol: 1, id: 'file', ...identity, ok: true, result: { path: '/tmp/committed.png' } }; } };
  const running = batches.run(native, invoke, identity, { ...params, steps: [capture, { kind: 'run', n: 1 }] }, 'parent', () => true, () => {});
  await ready; batches.release('parent'); complete(); const result = await running;
  assert.equal(result.stop_reason, 'disconnected'); assert.equal(result.results[0].data.path, '/tmp/committed.png'); assert.equal(result.steps_completed, 1); assert.equal(calls, 1); assert.equal(native.released, 1);
  decodeReply(JSON.stringify({ protocol: 1, id: 'result', ...identity, ok: true, result }), 'experiment.batch');
});
test('batch follows only its own successful load epoch and stops after an external reset', async () => {
  const native = nativeFixture(), batches = new Batches(); const seen = [];
  const invoke = { async request(method, id) {
    seen.push({ method, ...id });
    if (method === 'state.load') native.identity.epoch = 2;
    else native.identity.epoch = 3; // concurrent reset after this completed read
    return { protocol: 1, id: 'step', ...native.identity, ok: true, result: method === 'state.load' ? { frame: 0, epoch: 2, path: '/tmp/baseline.bs' } : { hex: '00' } };
  } };
  const read = { kind: 'read', address: '0x2000', length: 1, view: 'rdram' };
  const result = await batches.run(native, invoke, identity, { ...params, stopOnError: false, steps: [{ kind: 'state_load', path: '/tmp/baseline.bs' }, read, read] }, 'parent', () => true, () => {});
  assert.equal(seen[1].epoch, 2); assert.equal(seen.length, 2); assert.equal(result.stop_reason, 'stale_epoch'); assert.equal(result.side_effects_rolled_back, false);
});
test('batch result-space exhaustion stops before a following mutation and preserves bounded read results', async () => {
  const batches = new Batches(), native = nativeFixture(); let writes = 0;
  const invoke = { async request(method) {
    if (method === 'n64.memory.write') writes++;
    return { protocol: 1, id: 'read', ...identity, ok: true, result: { addr: '0x2000', paddr: '0x2000', view: 'rdram', cache_aware: false, length: 1024, hex: 'ab'.repeat(1024) } };
  } };
  const read = { kind: 'read', address: '0x2000', length: 1024, view: 'rdram' };
  const result = await batches.run(native, invoke, identity, { ...params, steps: [...Array(15).fill(read), { kind: 'write', address: '0x2000', view: 'rdram', hex: 'ff' }] }, 'parent', () => true, () => {});
  assert.equal(result.stop_reason, 'result_limit'); assert.equal(writes, 0); assert.equal(result.results.length, 15);
  assert.ok(Buffer.byteLength(JSON.stringify(result)) <= 49152);
  decodeReply(JSON.stringify({ protocol: 1, id: 'result', ...identity, ok: true, result }), 'experiment.batch');
});
test('batch cancellation token cannot revoke a foreign or later job',async()=>{
  const batches=new Batches(),native=nativeFixture();let begin,finish;
  const started=new Promise(r=>begin=r),waiting=new Promise(r=>finish=r);
  const invoke={async request(){begin();await waiting;return {protocol:1,id:'read',...identity,ok:true,result:{hex:'aa'}};}};
  const token='12345678-1234-1234-1234-123456789abc';
  const job=batches.run(native,invoke,identity,{...params,cancellationToken:token,steps:[{kind:'read',address:'0x2000',length:1,view:'rdram'}]},'parent',()=>true,()=>{});
  await started;assert.equal(batches.cancel('foreign',token),false);assert.equal(batches.cancel('parent','87654321-1234-1234-1234-123456789abc'),false);
  assert.equal(batches.cancel('parent',token),true);finish();assert.equal((await job).stop_reason,'cancelled');
  assert.equal(batches.cancel('parent',token),false,'finished token is inert');
});
