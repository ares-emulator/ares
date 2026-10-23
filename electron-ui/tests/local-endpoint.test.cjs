'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const net = require('node:net');
const { once } = require('node:events');
const { LocalServiceHost, LocalServiceClient, readConnection } = require('../dist/services/local-endpoint.js');
const { ServiceError } = require('../dist/services/contracts.js');
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
async function fixture(check) {
  const directory = fs.mkdtempSync(path.join(process.platform === 'darwin' ? '/tmp' : os.tmpdir(), 'ares-local-'));
  let writes = 0, releasePause;
  const backend = { request: async method => {
    if (method === 'session.pause') { ++writes; await new Promise(resolve => { releasePause = resolve; }); }
    return { protocol: 1, id: 'native1', session: 'fixture', epoch: 1, ok: true, result: { paused: false, system: 'Fixture', synthetic: true, capabilities: ['session.status', 'session.pause'] } };
  } };
  const host = new LocalServiceHost(backend); const file = await host.listen(directory);
  const client = new LocalServiceClient(file, host.instance, 500);
  try { await check({ host, file, client, directory, writes: () => writes, releasePause: () => releasePause?.() }); }
  finally { releasePause?.(); client.close(); await host.close(); fs.rmSync(directory, { recursive: true, force: true }); }
}
test('explicit instance selection, private metadata, disconnect/reconnect and timeout never replay a mutation', async () => {
  await fixture(async ({ host, file, client, writes, releasePause }) => {
    assert.equal((await client.request('session.status')).session, 'fixture');
    await assert.rejects(client.request('session.pause'), /request_timeout/);
    assert.equal(writes(), 1);
    releasePause();
    assert.equal((await client.request('session.status')).session, 'fixture');
    await sleep(50); assert.equal(writes(), 1);
    const wrong = new LocalServiceClient(file, 'different-instance');
    await assert.rejects(wrong.request('session.status'), /wrong_instance/); wrong.close();
    fs.chmodSync(file, 0o644);
    await assert.rejects(readConnection(file, host.instance), /insecure_service_permissions/);
    fs.chmodSync(file, 0o600);
    await host.close();
    await assert.rejects(client.request('session.status'), /app_not_running|app_disconnected/);
  });
});
test('fragmented handshake and stale connection requests fail before touching the backend', async () => {
  await fixture(async ({ host, file, writes }) => {
    const info = JSON.parse(fs.readFileSync(file));
    const socket = net.createConnection(info.endpoint); socket.on('error', () => {});
    await once(socket, 'connect');
    let buffer = ''; const replies = [];
    socket.on('data', chunk => { buffer += chunk; let index; while ((index = buffer.indexOf('\n')) >= 0) { replies.push(JSON.parse(buffer.slice(0, index))); buffer = buffer.slice(index + 1); } });
    const hello = JSON.stringify({ protocol: 1, type: 'hello', id: 'hello', instance: host.instance, token: info.token }) + '\n';
    socket.write(hello.slice(0, 9)); socket.write(hello.slice(9));
    for (let i = 0; i < 100 && !replies.length; i++) await sleep(5);
    assert.equal(replies[0]?.ok, true);
    socket.write(JSON.stringify({ protocol: 1, type: 'request', id: 'mutation', connection: 'old-connection', method: 'session.pause', identity: { session: 'fixture', epoch: 1 } }) + '\n');
    for (let i = 0; i < 100 && replies.length < 2; i++) await sleep(5);
    assert.equal(replies[1]?.error, 'stale_connection'); assert.equal(writes(), 0);
    socket.write(JSON.stringify({ protocol: 1, type: 'request', id: 'seen', connection: replies[0].connection, method: 'session.status' }) + '\n');
    for (let i = 0; i < 100 && replies.length < 3; i++) await sleep(5);
    socket.write(JSON.stringify({ protocol: 1, type: 'request', id: 'seen', connection: replies[0].connection, method: 'session.pause', identity: { session: 'fixture', epoch: 1 } }) + '\n');
    for (let i = 0; i < 100 && replies.length < 4; i++) await sleep(5);
    assert.equal(replies[3]?.error, 'duplicate_request'); assert.equal(writes(), 0); socket.destroy();
  });
});
test('the socket handshake rejects a different app instance and an invalid authentication token', async () => {
  await fixture(async ({ host, file, writes }) => {
    const info = JSON.parse(fs.readFileSync(file));
    for (const [instance, token, expected] of [['different-instance', info.token, 'wrong_instance'], [host.instance, '0'.repeat(64), 'unauthorized']]) {
      const socket = net.createConnection(info.endpoint); socket.on('error', () => {});
      await once(socket, 'connect');
      const response = once(socket, 'data');
      socket.write(JSON.stringify({ protocol: 1, type: 'hello', id: 'hello', instance, token }) + '\n');
      assert.equal(JSON.parse((await response)[0]).error, expected); socket.destroy();
    }
    assert.equal(writes(), 0);
  });
});
test('operation polling can outlive the arbitrary-ID quota without reconnecting or replaying', { timeout: 15000 }, async () => {
  await fixture(async ({ host, file }) => {
    const client = new LocalServiceClient(file, host.instance);
    try {
      for (let i = 0; i < 1050; i++) assert.equal((await client.request('session.status')).ok, true);
      assert.equal(host.sockets.size, 1, 'same connection, and therefore same owner generation');
    } finally { client.close(); }
  });
});
test('named selection uses a kernel claim lock and fresh credentials across launches without replay', async () => {
  const directory=fs.mkdtempSync('/tmp/ares-named-');fs.chmodSync(directory,0o700);
  const binary=path.resolve(__dirname,'../../build-electron-make/bin/ares-electron-host');
  let epoch=1,writes=0,complete;
  const backend={request:async method=>{if(method==='session.pause'){writes++;await new Promise(r=>complete=r)};return {protocol:1,id:'native',session:'launch'+epoch,epoch:1,ok:true,result:{paused:true,system:'Fixture',synthetic:true,capabilities:['session.status']}}}};
  const a=new LocalServiceHost(backend),b=new LocalServiceHost(backend),duplicate=new LocalServiceHost(backend),client=LocalServiceClient.named(directory,'kirby',1000);
  try {
    await a.listen(directory,'kirby',binary);const old={...a.info};
    assert.equal((await client.request('session.status')).session,'launch1');
    await assert.rejects(duplicate.listen(directory,'kirby',binary),/instance_name_in_use/);
    const mutation=client.request('session.pause');await sleep(20);const rejected=assert.rejects(mutation,/app_disconnected/);await a.close();await rejected;complete();
    assert.equal(client.identity,undefined);await assert.rejects(client.request('session.status'),/instance_not_running/);
    epoch=2;await sleep(30);await b.listen(directory,'kirby',binary);
    assert.notEqual(old.instance,b.info.instance);assert.notEqual(old.token,b.info.token);
    assert.equal((await client.request('session.status')).session,'launch2');assert.equal(writes,1);
    await assert.rejects(readConnection(b.metadataFile,old.instance),/wrong_instance/);
    await b.close();
    // A killed process leaves a valid private record; dead credentials cannot
    // retain the name once the kernel has released its claim lock.
    fs.writeFileSync(path.join(directory,'named-kirby.json'),JSON.stringify(old),{mode:0o600});
    const recovered=new LocalServiceHost(backend);try{await sleep(30);await recovered.listen(directory,'kirby',binary);assert.equal((await client.request('session.status')).session,'launch2')}finally{await recovered.close()}
  } finally {complete?.();client.close();await a.close();await b.close();await duplicate.close();fs.rmSync(directory,{recursive:true,force:true})}
});
test('named stale recovery fails closed when authenticated liveness is inconclusive',async()=>{
 const directory=fs.mkdtempSync('/tmp/ares-live-');fs.chmodSync(directory,0o700);
 const binary=path.resolve(__dirname,'../../build-electron-make/bin/ares-electron-host');
 const backend={request:async()=>({protocol:1,id:'x',session:'live',epoch:1,ok:true,result:{paused:true,system:'Fixture',synthetic:true,capabilities:['session.status']}})};
 const old=new LocalServiceHost(backend),next=new LocalServiceHost(backend);
 try{
  await old.listen(directory);const claim=path.join(directory,'named-kirby.json');const info={...old.info,token:'0'.repeat(64)};fs.writeFileSync(claim,JSON.stringify(info),{mode:0o600});
  await assert.rejects(next.listen(directory,'kirby',binary),/instance_liveness_uncertain/);
  assert.deepEqual(JSON.parse(fs.readFileSync(claim)),info,'inconclusive probe does not remove or replace the existing record');
 }finally{await next.close();await old.close();fs.rmSync(directory,{recursive:true,force:true})}
});
