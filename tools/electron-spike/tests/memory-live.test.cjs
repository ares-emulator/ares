'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { randomUUID } = require('node:crypto');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { MemoryClient, hexToBytes } = require('../dist/services/memory.js');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
async function instance(args, check) {
  const saves = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-memory-saves-'));
  const identity = { session: randomUUID(), epoch: 1 };
  const child = spawn(host, [...args, '--silent', '--paused', '--save-dir', saves, '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close');
  let log = '';
  child.stderr.on('data', b => log += b); child.stdout.resume();
  const client = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {});
  const memory = new MemoryClient(client);
  try {
    assert.equal((await client.request('session.status')).result.paused, true, log);
    await check(memory, client, child);
    client.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    client.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; }
    fs.rmSync(saves, { recursive: true, force: true });
  }
}
async function roundTrip(memory, client, region) {
  const offset = region.size - 16;
  const before = await memory.read(region, offset, 16);
  try {
    await memory.writeUnsigned(region, offset, 0x12345678, 4, 'big');
    assert.equal((await memory.read(region, offset, 4)).hex, '12345678');
    assert.equal(await memory.readUnsigned(region, offset, 4, 'little'), 0x78563412);
    // Entire writes must be checked before any callback, not wrapped or partially applied.
    await assert.rejects(memory.write(region, region.size - 1, Uint8Array.of(0xaa, 0xbb)), /out_of_bounds/);
    await assert.rejects(memory.read(region, region.size, 1), /out_of_bounds/);
    await assert.rejects(memory.read(region, 0xffffffff, 16), /out_of_bounds/);
    assert.equal((await memory.read(region, offset, 16)).hex.slice(8), before.hex.slice(8));
  } finally {
    await memory.write(region, offset, hexToBytes(before.hex));
    assert.equal((await memory.read(region, offset, 16)).hex, before.hex, 'Fixture bytes restored');
  }
  const bulkLength = Math.min(1024, region.size), bulkOffset = region.size - bulkLength;
  const baseline = await memory.read(region, bulkOffset, bulkLength);
  const pattern = Uint8Array.from({ length: bulkLength }, (_, i) => (i * 17 + 3) & 255);
  try {
    await memory.write(region, bulkOffset, pattern);
    assert.deepEqual(hexToBytes((await memory.read(region, bulkOffset, bulkLength)).hex), pattern);
  } finally {
    await memory.write(region, bulkOffset, hexToBytes(baseline.hex));
    assert.equal((await memory.read(region, bulkOffset, bulkLength)).hex, baseline.hex);
  }
  await client.request('session.reset');
  await assert.rejects(memory.write(region, offset, Uint8Array.of(0)), /stale_epoch/);
}
test('synthetic nodes report actual callback metadata; absent/read-only callbacks reject access', { timeout: 15000 }, async () => {
  await instance(['--synthetic'], async (memory, client, child) => {
    const regions = await memory.regions();
    assert.equal(regions.length, 3);
    const [ram, rom, absent] = regions;
    assert.equal(ram.addressing, 'region-offset');
    assert.equal(rom.writable, false); assert.equal(rom.readable, true);
    assert.equal(absent.readable, false); assert.equal(absent.writable, false);
    await assert.rejects(memory.write(rom, 0, Uint8Array.of(1)), /region_not_writable/);
    await assert.rejects(memory.read(absent, 0, 1), /region_not_readable/);
    await roundTrip(memory, client, ram);
    const refreshed = (await memory.regions())[0];
    assert.equal((await memory.read(refreshed, 0, 4)).hex, '00010203');
    await client.request('session.resume');
    await memory.writeUnsigned(refreshed, 128, 0xaabbccdd, 4, 'big');
    assert.equal(await memory.readUnsigned(refreshed, 128, 4, 'big'), 0xaabbccdd);
    await memory.write(refreshed, 128, Uint8Array.of(128, 129, 130, 131));
    // Bypass JS validation to exercise malformed payload rejection in C++ itself.
    let replies = '', observed;
    const listener = chunk => { replies += chunk; if (replies.includes('\n')) observed = JSON.parse(replies.split('\n')[0]); };
    // The shared client owns fd 3; capture the raw failure through a separate
    // request ID that it expects, after disabling the client for this final check.
    client.close(); child.stdio[3].on('data', listener);
    child.stdin.write(`control 1 raw ${refreshed.identity.session} ${refreshed.identity.epoch} memory.write ${refreshed.id} 0 0z\n`);
    for (let i = 0; i < 100 && !observed; i++) await new Promise(resolve => setTimeout(resolve, 10));
    assert.equal(observed?.error, 'invalid_request');
    child.stdio[3].off('data', listener);
  });
});
test('real N64 and GBA debugger regions inspect and restore deterministic writes on isolated paused cores', { timeout: 30000 }, async () => {
  const scratch = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-memory-fixtures-'));
  try {
    // Original dummy images only initialize the normal loaders and debugger nodes.
    // --paused prevents executing either cartridge; this is not boot/game evidence.
    const n64 = Buffer.alloc(0x1000); n64.writeUInt32BE(0x80371240); n64.write('MEMORY FIXTURE', 0x20); n64[0x3e] = 0x45;
    const gba = Buffer.alloc(0x8000); gba.writeUInt32LE(0xeafffffe); gba.write('MEMORY TEST', 0xa0); gba[0xb2] = 0x96;
    const bios = Buffer.alloc(0x4000); bios.writeUInt32LE(0xeafffffe); // Original ARM b . stand-in, no vendor BIOS content.
    const n64Path = path.join(scratch, 'fixture.z64'), gbaPath = path.join(scratch, 'fixture.gba');
    const biosPath = path.join(scratch, 'fixture-bios.bin'), settingsPath = path.join(scratch, 'settings.bml');
    fs.writeFileSync(n64Path, n64); fs.writeFileSync(gbaPath, gba); fs.writeFileSync(biosPath, bios);
    fs.writeFileSync(settingsPath, `GameBoyAdvance\n  Firmware\n    BIOS.World: ${biosPath}\n`);
    let oldRegion;
    for (const [rom, system, target] of [[n64Path, 'Nintendo 64', 'RSP DMEM'], [gbaPath, 'Game Boy Advance', 'CPU EWRAM']]) {
      await instance(['--rom', rom, '--system', system, '--settings', settingsPath], async (memory, client) => {
        const regions = await memory.regions();
        const region = regions.find(r => r.name === target);
        assert.ok(region, `${system}: ${regions.map(r => r.name).join(', ')}`);
        assert.equal(region.writable, true); assert.equal(region.readable, true);
        if (oldRegion) await assert.rejects(memory.read(oldRegion, 0, 1), /stale_session/);
        if (oldRegion) assert.equal((await client.request('memory.read', region.identity, { region: oldRegion.id, offset: 0, length: 1 })).error, 'stale_region');
        const rdram = regions.find(r => r.name === 'RDRAM');
        if (rdram) {
          assert.equal(rdram.writable, false);
          await memory.read(rdram, 0, 4);
          await assert.rejects(memory.write(rdram, 0, Uint8Array.of(1)), /region_not_writable/);
        }
        const cache = regions.find(r => r.name === 'DCache');
        if (cache) {
          assert.equal(cache.readable, false); assert.equal(cache.writable, false);
          await assert.rejects(memory.read(cache, 0, 1), /unsupported_region/);
        }
        await roundTrip(memory, client, region);
        oldRegion = region;
      });
    }
  } finally { fs.rmSync(scratch, { recursive: true, force: true }); }
});
