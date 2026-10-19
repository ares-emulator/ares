const assert = require('node:assert/strict');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const { FrameParser } = require('./protocol.cjs');
const { NativeSessionClient } = require('./dist/services/native-client.js');
const { randomUUID } = require('node:crypto');
const { makeFixture, makeNesFixture } = require('./fixture.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../build-electron-make/bin/ares-electron-host');
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const nesFixture = path.join(__dirname, 'artifacts/spike.nes');
const fixture = path.join(__dirname, 'artifacts/spike.gb');
fs.mkdirSync(path.dirname(fixture), { recursive: true }); fs.writeFileSync(fixture, makeFixture()); fs.writeFileSync(nesFixture, makeNesFixture());
async function run(args, label) {
  const identity = { session: randomUUID(), epoch: 1 };
  const child = spawn(host, [...args, '--save-dir', path.join(__dirname, 'artifacts/smoke-saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close');
  let errorLog = '', waiter, received = 0;
  child.stderr.on('data', b => errorLog += b);
  child.stdin.on('error', () => {});
  const client = new NativeSessionClient(child.stdin, child.stdio[3], identity, error => { errorLog += error.message; });
  const parser = new FrameParser(frame => {
    received++;
    if (!waiter) throw new Error(`${label}: unsolicited frame`);
    const resolve = waiter; waiter = null; resolve(frame);
  });
  child.stdout.on('data', b => parser.push(b));
  function next() {
    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => { waiter = null; reject(new Error(`${label}: no frame: ${errorLog}`)); }, 8000);
      waiter = frame => { clearTimeout(timeout); resolve(frame); };
      child.stdin.write('ack\n');
    });
  }
  try {
    const first = await next();
    if (args.includes('--synthetic')) {
      const tick = first.sequence - 1;
      assert.deepEqual([...first.pixels.subarray(0, 4)], [tick % 256, tick % 256, tick % 256, 255]);
      const offset = first.width * 4;
      assert.deepEqual([...first.pixels.subarray(offset, offset + 4)], [tick % 256, (tick + 1) % 256, tick % 256, 255]);
    }
    await sleep(300);
    assert.equal(received, 1, 'No frames without credit');
    const resumed = await next();
    assert.ok(resumed.sequence > first.sequence + 5, 'Native producer continues while consumer stalls');
    child.stdin.write('input 42 32\n');
    let echo;
    for (let i = 0; i < 90; i++) { echo = await next(); if (echo.inputSequence === 42) break; }
    if (!args.includes(nesFixture)) assert.equal(echo.inputSequence, 42, 'Input reaches the native core');
    child.stdin.write('input 43 0\n');
    assert.equal((await client.request('session.pause')).result.paused, true);
    await sleep(100);
    const count = received;
    await sleep(150); assert.equal(received, count);
    assert.equal((await client.request('session.resume')).result.paused, false);
    const start = await next();
    let end = start, changes = 0, previous = start.pixels;
    const finish = Date.now() + (label.includes('GBA') ? 12000 : args.includes('--rom') ? 4000 : 1500);
    while (Date.now() < finish) {
      end = await next(); if (!end.pixels.equals(previous)) changes++; previous = end.pixels;
    }
    assert.ok(changes > 10, 'Video changes over time');
    const fps = (end.sequence - start.sequence) * 1000 / (end.nativeMs - start.nativeMs);
    assert.ok(fps > 50 && fps < 70, `Unexpected native speed ${fps}`);
    client.close();
    child.stdin.end('quit\n');
    const [code, signal] = await closed;
    assert.equal(code, 0, `${signal}: ${errorLog}`);
    return { label, width: end.width, height: end.height, fps, received, changes, status: errorLog.trim() };
  } finally { client.close(); if (child.exitCode === null) child.kill('SIGKILL'); }
}
(async () => {
  const systems = execFileSync(host, ['--systems'], { encoding: 'utf8' }).trim().split('\n');
  for (const name of ['Game Boy', 'Famicom']) assert.ok(systems.includes(name), `${name} is registered`);
  for (const [file, expected] of [[fixture, 'Game Boy'], [nesFixture, 'Famicom']]) {
    assert.equal(execFileSync(host, ['--identify', file], { encoding: 'utf8' }).trim(), expected);
  }
  console.log(`Registered ${systems.length} systems; GB/NES detection passed`);
  const results = [];
  for (const [args, label] of [
    [['--synthetic'], '640x480 transport'], [['--synthetic', '--1080p'], '1080p transport'],
    [['--rom', fixture, ...(process.env.ARES_TEST_AUDIO === '1' ? [] : ['--silent'])], 'Native Game Boy fixture'],
    [['--rom', nesFixture, '--silent'], 'Native Famicom fixture'],
    ...(process.env.ARES_GBA_ROM ? [[['--rom', process.env.ARES_GBA_ROM, '--silent'], 'Native GBA post-BIOS regression']] : []),
  ]) { const result = await run(args, label); results.push(result); console.log(result); }
  fs.writeFileSync(path.join(__dirname, 'artifacts/native-smoke.json'), JSON.stringify(results, null, 2));
})().catch(error => { console.error(error); process.exitCode = 1; });
