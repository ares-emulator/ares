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
  let writes = 0;
  const backend = { request: async method => {
    if (method === 'session.pause') { ++writes; await sleep(100); }
    return { protocol: 1, id: 'native1', session: 'fixture', epoch: 1, ok: true, result: { paused: false, system: 'Fixture', synthetic: true, capabilities: ['session.status', 'session.pause'] } };
  } };
  const host = new LocalServiceHost(backend); const file = await host.listen(directory);
  const client = new LocalServiceClient(file, host.instance, 30);
  try { await check({ host, file, client, directory, writes: () => writes }); }
  finally { client.close(); await host.close(); fs.rmSync(directory, { recursive: true, force: true }); }
}
test('explicit instance selection, private metadata, disconnect/reconnect and timeout never replay a mutation', async () => {
  await fixture(async ({ host, file, client, writes }) => {
    assert.equal((await client.request('session.status')).session, 'fixture');
    await assert.rejects(client.request('session.pause'), /request_timeout/);
    assert.equal(writes(), 1);
    assert.equal((await client.request('session.status')).session, 'fixture');
    await sleep(130); assert.equal(writes(), 1);
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
