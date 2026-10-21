'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost, LocalServiceClient } = require('../dist/services/local-endpoint.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const sleep = ms => new Promise(r => setTimeout(r, ms));
async function finish(client, operation) {
  for (let i = 0; i < 200; i++) {
    const reply = await client.request('execution.status', undefined, { operationId: operation.operation_id });
    assert.equal(reply.ok, true, JSON.stringify(reply));
    if (reply.result.state === 'done') return reply.result;
    await sleep(10);
  }
  throw new Error('operation did not finish');
}
test('native coordinator fixture covers partial frames and immutable final results', () => {
  const dir = fs.mkdtempSync('/tmp/ares-execution-unit-');
  try {
    const binary = path.join(dir, 'fixture');
    execFileSync('c++', ['-std=c++17', path.join(__dirname, 'execution-fixture.cpp'), '-o', binary]);
    assert.match(execFileSync(binary, { encoding: 'utf8' }), /EXECUTION_FIXTURE_PASS/);
  } finally { fs.rmSync(dir, { recursive: true, force: true }); }
});
test('owned exact-frame runs through native, local endpoint and real MCP stdio', { timeout: 30000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-execution-live-');
  const identity = { session: 'execution-fixture', epoch: 1 };
  const child = spawn(host, ['--synthetic', '--silent', '--paused', '--save-dir', dir, '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log += b); child.stdout.resume();
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  const router = new SessionRouter(); router.attach(native);
  const endpoint = new LocalServiceHost(router);
  const clients = [], peers = [];
  try {
    await endpoint.listen(path.join(dir, 'ipc'));
    const connect = async () => { const c = new LocalServiceClient(endpoint.metadataFile, endpoint.instance); clients.push(c); await c.request('session.status'); return c; };
    const a = await connect(), b = await connect();
    const zero = (await a.request('execution.runFrames', undefined, { count: 0, timeoutMs: 1000 })).result;
    assert.equal(zero.state, 'done'); assert.equal(zero.frame, 0); assert.equal(zero.slices, 0);
    const exact = await finish(a, (await a.request('execution.runFrames', undefined, { count: 3, timeoutMs: 1000 })).result);
    assert.equal(exact.frame, 3); assert.equal(exact.elapsed_frames, 3); assert.equal(exact.slices, 6);
    assert.equal(exact.stop_reason, 'frames_done'); assert.equal(exact.at_frame_boundary, true);
    assert.deepEqual((await b.request('execution.status', undefined, { operationId: exact.operation_id })).result, exact);
    const absolute = await finish(a, (await a.request('execution.runUntilFrame', undefined, { frame: 5, timeoutMs: 1000 })).result);
    assert.equal(absolute.frame, 5);
    const unpaced = await finish(a, (await a.request('execution.runFrames', undefined, { count: 60, timeoutMs: 800 })).result);
    assert.equal(unpaced.stop_reason, 'frames_done'); assert.equal(unpaced.elapsed_frames, 60);
    assert.ok(unpaced.elapsed_seconds < 0.8, 'controlled execution must bypass ordinary 60 Hz pacing');
    assert.equal((await a.request('execution.runUntilFrame', undefined, { frame: 4, timeoutMs: 1000 })).error, 'invalid_target');
    let op = (await a.request('execution.runFrames', undefined, { count: 1000000, timeoutMs: 10000 })).result;
    assert.equal((await b.request('execution.runFrames', undefined, { count: 1, timeoutMs: 1000 })).error, 'busy');
    assert.equal((await b.request('execution.cancel', undefined, { operationId: op.operation_id })).error, 'busy');
    assert.equal((await b.request('session.resume')).error, 'busy');
    const regions = (await b.request('memory.regions')).result.regions;
    assert.equal((await b.request('memory.write', undefined, { region: regions[0].id, offset: 0, hex: '00' })).error, 'busy');
    const cancelStart = Date.now();
    const cancelled = (await a.request('execution.cancel', undefined, { operationId: op.operation_id })).result;
    assert.equal(cancelled.stop_reason, 'cancelled'); assert.ok(Date.now() - cancelStart < 1000);
    assert.deepEqual((await a.request('execution.cancel', undefined, { operationId: op.operation_id })).result, cancelled);
    op = (await a.request('execution.runFrames', undefined, { count: 1000000, timeoutMs: 10000 })).result;
    await router.request('session.pause');
    assert.equal((await finish(a, op)).stop_reason, 'paused');
    op = (await a.request('execution.runFrames', undefined, { count: 1000000, timeoutMs: 30 })).result;
    const timed = await finish(a, op); assert.equal(timed.stop_reason, 'timeout'); assert.ok(timed.elapsed_seconds < 1);
    op = (await a.request('execution.runFrames', undefined, { count: 1000000, timeoutMs: 10000 })).result;
    a.disconnect(); const disconnected = await finish(b, op);
    assert.equal(disconnected.stop_reason, 'disconnected');
    assert.ok(disconnected.elapsed_seconds < 1);
    // Reconnecting cannot release an operation belonging to another generation.
    await a.request('session.status');
    op = (await b.request('execution.runFrames', undefined, { count: 1000000, timeoutMs: 10000 })).result;
    a.disconnect(); await sleep(30);
    assert.equal((await b.request('execution.status', undefined, { operationId: op.operation_id })).result.state, 'running');
    const oldIdentity = { ...b.identity };
    await router.request('session.reset');
    assert.equal((await b.request('execution.status', oldIdentity, { operationId: op.operation_id })).error, 'stale_epoch');
    assert.equal((await b.request('session.status')).result.frame, 0);
    assert.equal((await b.request('execution.status', undefined, { operationId: op.operation_id })).result.stop_reason, 'reset');
    const peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); peers.push(peer);
    await peer.initialize();
    const mcpExact = await peer.tool('run_frames', { n: 2 });
    assert.equal(mcpExact.frame, 2); assert.equal(mcpExact.stop_reason, 'frames_done');
    assert.equal((await peer.tool('step')).frame, 3);
    assert.equal((await peer.tool('run_until_frame', { k: 4 })).frame, 4);
    assert.equal((await peer.tool('operation_status', { operation_id: mcpExact.operation_id })).frame, 2);
    const pending = peer.tool('run_frames', { n: 1000000, max_seconds: 10 });
    let running;
    for (let i = 0; i < 100 && !running; i++) { running = (await router.request('session.status')).result.operation_id; if (!running) await sleep(10); }
    assert.ok(running);
    const mcpCancel = await peer.tool('cancel_operation', { operation_id: running });
    assert.equal(mcpCancel.stop_reason, 'cancelled');
    assert.deepEqual(await pending, mcpCancel);
    assert.equal((await peer.tool('run_frames', { n: 0, fpr: true })).error, 'unavailable');
    const cancelledRequest=peer.id+1;
    const cancelledTool=peer.request('tools/call',{name:'run_frames',arguments:{n:1000000,max_seconds:10}},2000).catch(()=>undefined);
    let signalId;
    for(let i=0;i<100&&!signalId;++i){signalId=(await router.request('session.status')).result.operation_id;if(!signalId)await sleep(10);}
    assert.ok(signalId);
    peer.child.stdin.write(JSON.stringify({jsonrpc:'2.0',method:'notifications/cancelled',params:{requestId:cancelledRequest,reason:'migration regression'}})+'\n');
    const signalled=await finish(b,{operation_id:signalId});
    assert.equal(signalled.stop_reason,'cancelled','MCP request abort cancels its native operation');
    await cancelledTool;
    const abandoned = peer.tool('run_frames', { n: 1000000, max_seconds: 10 }).catch(() => undefined);
    let abandonedId;
    for (let i = 0; i < 100 && !abandonedId; i++) { abandonedId = (await router.request('session.status')).result.operation_id; if (!abandonedId) await sleep(10); }
    assert.ok(abandonedId);
    assert.equal((await peer.close())[0], 0, peer.stderr);
    await abandoned;
    assert.equal((await finish(b, { operation_id: abandonedId })).stop_reason, 'disconnected');
    child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    for (const p of peers) await p.close();
    for (const c of clients) c.close();
    await endpoint.close(); router.detach();
    if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(dir, { recursive: true, force: true });
  }
});
test('real N64 inactive VI does not create frames and yields for deadline/cancel', { timeout: 20000 }, async () => {
  const dir = fs.mkdtempSync('/tmp/ares-n64-execution-');
  const rom = path.join(dir, 'fixture.z64'), settings = path.join(dir, 'settings.bml');
  const image = Buffer.alloc(0x1000); image.writeUInt32BE(0x80371240); image.write('EXECUTION FIXTURE', 0x20); image[0x3e] = 0x45;
  fs.writeFileSync(rom, image); fs.writeFileSync(settings, '');
  const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--settings', settings, '--silent', '--paused', '--save-dir', dir, '--control-session', 'n64-execution'], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log += b); child.stdout.resume();
  const client = new NativeSessionClient(child.stdin, child.stdio[3], { session: 'n64-execution', epoch: 1 }, () => {});
  try {
    const initial = await client.request('session.status');
    assert.equal(initial.result.frame_kind, 'vi'); assert.equal(initial.result.frame, 0);
    assert.ok(['recompiler', 'interpreter'].includes(initial.result.executor));
    const start = Date.now();
    const op = (await client.request('execution.runFrames', undefined, { count: 1, timeoutMs: 50 })).result;
    assert.ok(op, log);
    const timed = await finish(client, op);
    assert.equal(timed.stop_reason, 'timeout'); assert.equal(timed.frame, 0);
    assert.ok(timed.slices > 0); assert.equal(timed.at_frame_boundary, false);
    assert.ok(Date.now() - start < 1500, log);
    const running = (await client.request('execution.runFrames', undefined, { count: 1, timeoutMs: 10000 })).result;
    const before = Date.now();
    assert.equal((await client.request('execution.cancel', undefined, { operationId: running.operation_id })).result.stop_reason, 'cancelled');
    assert.ok(Date.now() - before < 1000);
    assert.equal((await client.request('session.status')).result.paused, true);
    child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    client.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(dir, { recursive: true, force: true });
  }
});
