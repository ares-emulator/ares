'use strict';
// Optional cross-binary gate. Launches only isolated fixtures and protects the
// reference app with a write sandbox; never connects to an existing emulator.
const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const { createHash, randomUUID } = require('node:crypto');
const assert = require('node:assert/strict');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const pause = ms => new Promise(r => setTimeout(r, ms));
const reference = process.env.ARES_REFERENCE_HOST;
if (!reference || !path.isAbsolute(reference)) throw new Error('Set ARES_REFERENCE_HOST to the audited reference app executable.');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const unwrap = r => { assert.equal(r.ok, true, JSON.stringify(r)); return r.result; };
const canonical = v => { let n = BigInt(v); if (n >= 0x80000000n && n <= 0xffffffffn) n |= 0xffffffff00000000n; return n; };
async function nativeInstance(dir, rom, check, settingsText = '') {
  fs.mkdirSync(dir, { recursive: true }); const settings = path.join(dir, 'settings.bml'); fs.writeFileSync(settings, settingsText);
  const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--settings', settings, '--save-dir', path.join(dir, 'saves'), '--silent', '--paused', '--control-session', randomUUID()], { stdio: ['pipe','pipe','pipe','pipe'] });
  let log = ''; child.stdout.resume(); child.stderr.on('data', b => log += b); const closed = once(child, 'close');
  const native = new NativeSessionClient(child.stdin, child.stdio[3], { session: '', epoch: 1 }, () => {});
  // The request identity must match the explicit process argument.
  native.identity.session = child.spawnargs[child.spawnargs.length - 1];
  const router = new SessionRouter(); router.attach(native);
  try {
    unwrap(await native.request('session.status')); await check(native, router);
    router.detach(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally { router.detach(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } }
}
async function run() {
  const dir = fs.mkdtempSync('/tmp/ares-state-reference-'); let child, closed, socket, diagnostics = '';
  try {
    const image = Buffer.alloc(0x1000); image.writeUInt32BE(0x80371240); image.write('STATE REFERENCE FIXTURE', 0x20); image[0x3e] = 0x45;
    const rom = path.join(dir, 'fixture.z64'); fs.writeFileSync(rom, image);
    const ours = path.join(dir, 'electron.bs'), theirs = path.join(dir, 'reference.bs'); let baseline, saved;
    await nativeInstance(path.join(dir, 'first'), rom, async (native, router) => {
      const op = unwrap(await native.request('execution.stepInstructions', undefined, { count: 2, timeoutMs: 1000 }));
      for (let n = 0; n < 200; n++) { if (unwrap(await native.request('execution.status', undefined, { operationId: op.operation_id })).state === 'done') break; await pause(5); }
      baseline = unwrap(await native.request('n64.debug.registers'));
      saved = unwrap(await router.request('state.save', undefined, { path: ours, overwrite: false, metadata: true }, 'reference-check'));
    });
    const server = net.createServer(); server.listen(0, '127.0.0.1'); await once(server, 'listening'); const port = server.address().port; await new Promise(r => server.close(r));
    const settings = path.join(dir, 'reference-settings.bml'), temporary = path.join(dir, 'tmp'); fs.mkdirSync(temporary);
    fs.writeFileSync(settings, `Paths\n  Saves: ${dir}/reference-saves/\n  Home: ${dir}/systems/\nBoot\n  Fast: true\nAudio\n  Mute: true\n  Blocking: false\nInput\n  Defocus: Allow\nNintendo64\n  ExpansionPak: true\n  Quality: SD\n  Supersampling: false\nGeneral\n  AutoSaveMemory: false\n`);
    const policy = `(version 1)(allow default)(deny file-write*)(allow file-write* (subpath ${JSON.stringify(dir)}) (subpath ${JSON.stringify('/private' + dir)}))(allow file-write-data (literal "/dev/null"))`;
    child = spawn('/usr/bin/sandbox-exec', ['-p', policy, reference, '--settings-file', settings, '--system', 'Nintendo 64', '--state', ours, '--mcp', '--telemetry-port', String(port), '--kiosk', rom], { env: { ...process.env, TMPDIR: temporary + '/', SDL_AUDIODRIVER: 'dummy' }, stdio: ['ignore','pipe','pipe'] });
    closed = once(child, 'close'); child.stdout.on('data', b => diagnostics = (diagnostics + b).slice(-20000)); child.stderr.on('data', b => diagnostics = (diagnostics + b).slice(-20000));
    let listening = false;
    for (let n = 0; n < 200; n++) {
      if (child.exitCode !== null) throw new Error(`Reference exited: ${diagnostics}`);
      try { const owned = execFileSync('/usr/sbin/lsof', ['-nP', '-a', '-p', String(child.pid), '-iTCP:' + port, '-sTCP:LISTEN'], { encoding: 'utf8', stdio: ['ignore','pipe','ignore'] }); if (owned.includes('127.0.0.1:' + port)) { listening = true; break; } } catch {}
      await pause(50);
    }
    assert.equal(listening, true, diagnostics);
    socket = net.createConnection({ host: '127.0.0.1', port }); await once(socket, 'connect'); socket.setTimeout(15000, () => socket.destroy(new Error('Reference RPC timed out')));
    let buffer = '', id = 0; const waiting = new Map();
    socket.setEncoding('utf8'); socket.on('data', b => { buffer += b; let end; while ((end = buffer.indexOf('\n')) >= 0) { const r = JSON.parse(buffer.slice(0, end)); buffer = buffer.slice(end + 1); const cb = waiting.get(r.id); if (cb) { waiting.delete(r.id); cb.resolve(r); } } });
    socket.on('error', error => { for (const cb of waiting.values()) cb.reject(error); waiting.clear(); });
    socket.on('close', () => { for (const cb of waiting.values()) cb.reject(new Error('Reference disconnected')); waiting.clear(); });
    const query = (cmd, args = {}) => new Promise((resolve, reject) => { const rid = ++id; waiting.set(rid, { resolve, reject }); socket.write(JSON.stringify({ id: rid, cmd, args }) + '\n'); });
    let regs;
    for (let n = 0; n < 100; n++) { const r = await query('cpu.regs', { verbose: true }); if (r.ok) { regs = r.data; break; } await pause(50); }
    assert.ok(regs, diagnostics); assert.equal(canonical(regs.pc), canonical(baseline.pc), JSON.stringify(regs));
    const back = await query('state.save', { path: theirs, overwrite: false, metadata: true }); assert.equal(back.ok, true, JSON.stringify(back));
    assert.ok(fs.existsSync(theirs));
    socket.destroy(); socket = undefined; child.kill('SIGTERM'); await Promise.race([closed, pause(2000).then(() => { if (child?.exitCode === null) child.kill('SIGKILL'); return closed; })]); child = undefined;
    let restored;
    await nativeInstance(path.join(dir, 'second'), rom, async (native, router) => {
      restored = unwrap(await router.request('state.load', undefined, { path: theirs, resume: false, keepOverrides: false }, 'reference-check'));
      assert.equal(canonical(restored.pc), canonical(baseline.pc));
      const current = unwrap(await native.request('n64.debug.registers')); assert.deepEqual(current.gpr, baseline.gpr); assert.equal(current.lo, baseline.lo); assert.equal(current.hi, baseline.hi);
    });
    await nativeInstance(path.join(dir, 'four-megabyte'), rom, async (native, router) => {
      const before = unwrap(await native.request('n64.debug.registers'));
      assert.equal(unwrap(await native.request('n64.state.layout')).size, 4194304);
      const rejected = await router.request('state.load', undefined, { path: theirs, resume: false, keepOverrides: false }, 'reference-check');
      assert.equal(rejected.error, 'state_configuration_mismatch');
      assert.deepEqual(unwrap(await native.request('n64.debug.registers')), before);
      assert.equal(native.identity.epoch, 1);
    }, 'Nintendo64\n  ExpansionPak: false\n');
    const result = { result: 'STATE_REFERENCE_PASS', reference, reference_sha256: createHash('sha256').update(fs.readFileSync(reference)).digest('hex'), version: saved.version, electron_size: saved.size, reference_size: fs.statSync(theirs).size, epoch_after_restore: restored.epoch, directions: ['electron-to-reference', 'reference-to-electron'], incompatible_4MiB_configuration_rejected: true, fixture: 'original ROM-free N64 fixture; same default hardware', reference_writes_confined_to: 'disposable test directory' };
    fs.mkdirSync(path.resolve(__dirname, '../artifacts'), { recursive: true }); fs.writeFileSync(path.resolve(__dirname, '../artifacts/state-reference.json'), JSON.stringify(result, null, 2)); console.log(JSON.stringify(result));
  } catch (error) { if (diagnostics) console.error(diagnostics.slice(-6000)); throw error; }
  finally { socket?.destroy(); if (child && child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true }); }
}
run().catch(error => { console.error(error); process.exitCode = 1; });
