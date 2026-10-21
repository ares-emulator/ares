'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawn, execFileSync } = require('node:child_process');
const { once } = require('node:events');
const { NativeSessionClient } = require('../dist/services/native-client.js');
const { SessionRouter } = require('../dist/services/session-router.js');
const { LocalServiceHost } = require('../dist/services/local-endpoint.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const { StdioPeer } = require('./stdio-peer.cjs');
const {queueText,elfImage,queueConfig} = require('./rspq-fixture.cjs');
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-host');
const fixture = process.env.ARES_CPU_FIXTURE || path.join(path.dirname(host), 'ares-electron-cpu-fixture');
const identity = { session: 'machine-live', epoch: 1 }, empty = { gpr: [], pc: null, hi: null, lo: null, status: null, llbit: null };
const unwrap = r => { assert.equal(r.ok, true, JSON.stringify(r)); return r.result; };
const sleep = ms => new Promise(r => setTimeout(r, ms));
for (const mode of ['F3DEX2','Custom DL','RSPQ']) test(`real N64 ${mode} viewer: live dispatch, Vulkan, origin commit and ownership`, { timeout: 30000 }, async () => {
  const queue=mode==='RSPQ',custom=mode==='Custom DL';
  const dir = fs.mkdtempSync('/tmp/ares-viewer-live-'), image = Buffer.alloc(0x1000);
  image.writeUInt32BE(0x80371240); image.write('MACHINE FIXTURE', 0x20); image[0x3e] = 0x45;
  image.writeUInt32BE(0x24020007, 0x80); image.writeUInt32BE(0x1000ffff, 0x84); image.writeUInt32BE(0, 0x88);
  const rom = path.join(dir, 'fixture.z64'), settings = path.join(dir, 'settings.bml'); fs.writeFileSync(rom, image); fs.writeFileSync(settings, '');
  if(queue) { fs.writeFileSync(path.join(dir,'fixture.elf'),elfImage());fs.writeFileSync(path.join(dir,'rspq-libdragon.json'),JSON.stringify(queueConfig())); }
  if(custom) fs.writeFileSync(path.join(dir,'f3dex2.json'),JSON.stringify({mode:'displaylist',banner:'Original DL',dispatch:{signature:['0x00196603','0x000c5840','0x24080060'],loopOffset:'-0x0c',dispatchOffset:'0x14',word0Reg:25,word1Reg:24,opcodeShift:24},commands:{'0x05':{name:'Decoded Triangle',fmt:'v={vertex} addr={address:x}',args:[{name:'vertex',word:0,bits:[23,16]},{name:'address',word:1}]}}}));
  const child = spawn(host, ['--rom', rom, '--system', 'Nintendo 64', '--settings', settings, '--silent', '--paused', '--save-dir', path.join(dir, 'saves'), '--control-session', identity.session], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] });
  const closed = once(child, 'close'); let log = ''; child.stderr.on('data', b => log = (log + b).slice(-20000)); child.stdout.resume();
  const native = new NativeSessionClient(child.stdin, child.stdio[3], identity, () => {}), router = new SessionRouter(); router.attach(native);
  const endpoint = new LocalServiceHost(router); let peer;
  try {
    unwrap(await native.request('session.status')); await endpoint.listen(path.join(dir, 'ipc'));
    peer = new StdioPeer(process.execPath, [path.resolve(__dirname, '../dist/mcp/server.js'), '--connection', endpoint.metadataFile, '--instance', endpoint.instance]); await peer.initialize();
    const control = await peer.tool('capture_control'); assert.equal(control.rsp, false); assert.equal(control.rdp, false);
    assert.equal((await peer.tool('capture_control', { rsp: true, rdp: true })).rspq_supported, true);
    assert.equal((await native.request('n64.capture.control', undefined, { rsp: false, rdp: false }, 'foreign')).error, 'busy');
    const imem = Buffer.alloc(0x6c);
    for (const [at, word] of [[0x14, 0x8c190000], [0x18, 0x8c180004], [0x20, 0x00196603], [0x24, 0x000c5840], [0x28, 0x24080060], [0x34, 0x01000008], [0x60, 0x24100001], [0x64, 0xac100004], [0x68, 0x0000000d]]) imem.writeUInt32BE(word, at);
    await peer.tool('write_memory', { region: 'RSP IMEM', addr: 0, hex_bytes: (queue ? queueText() : imem).toString('hex') });
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: queue ? 0xa0 : 0, hex_bytes: queue ? '1100fffe12345678' : '0501020312345678' });
    if(queue) {
      await peer.tool('write_memory',{region:'RSP DMEM',addr:0xc,hex_bytes:'00003000'});
      await peer.tool('write_memory',{region:'RSP DMEM',addr:0x49,hex_bytes:'0404'});
    }
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: 0x300, hex_bytes: '00112233445566770077665544332211' });
    // CPU launches the original F3DEX2-shaped dispatch fixture and submits two
    // DMEM RDP no-ops to Vulkan, then changes VI origin to commit viewer buffers.
    const code = [0x3c08a408, queue ? 0x24090000 : 0x24090014, 0xad090000, 0x3c08a404, 0x24090001, 0xad090010,
      0x3c08a410, 0x24090002, 0xad09000c, 0x24090300, 0xad090000, 0x24090310, 0xad090004,
      0x3c08a440, 0x24092000, 0xad090004, 0x1000ffff, 0];
    const bytes = Buffer.alloc(code.length * 4); code.forEach((w, i) => bytes.writeUInt32BE(w, i * 4));
    await peer.tool('write_memory', { region: 'RSP DMEM', addr: queue ? 0x500 : 0x80, hex_bytes: bytes.toString('hex') });
    await peer.tool('cpu_set_registers', { pc: queue ? '0xa4000500' : '0xa4000080', status: '0x34000000' });
    const run = await peer.tool('run_frames', { n: 1, max_seconds: 0.05 }); assert.equal(run.stop_reason, 'timeout');
    const rsp = await peer.tool('rsp_capture'); assert.equal(rsp.error, undefined, JSON.stringify(rsp));
    const cmd = rsp.entries.find(r => !r.overhead); assert.ok(cmd, JSON.stringify(rsp));
    assert.equal(cmd.command, queue ? 'Fixture Signed' : custom ? 'Decoded Triangle' : 'Tri1'); assert.deepEqual(cmd.words, [queue ? '0x1100fffe' : '0x5010203', '0x12345678']);
    if(custom) {assert.equal(cmd.args,'v=1 addr=0x12345678');assert.equal(cmd.overlay,'Original DL');}
    if(queue) {assert.equal(cmd.args,'n=-2 addr=0x12345678');assert.equal(cmd.overlay,'fixture');} assert.ok(cmd.cycle > 0);
    assert.equal((await peer.tool('read_memory', { region: 'RSP DMEM', addr: queue ? 0xa4 : 4, length: 4 })).hex, '00000001', 'handler overwrote DMEM only after command capture');
    const rdp = await peer.tool('rdp_capture', { next_frame: false });
    assert.equal(rdp.count, 2, JSON.stringify(rdp)); assert.equal(rdp.entries[0].word0, '0x11223344556677'); assert.equal(rdp.frame_kind, 'vi_origin_swaps');
    assert.equal((await peer.tool('rsp_capture', { limit: 0 })).count, 0);
    // Running new commands without another origin change cannot mutate the
    // immutable committed snapshot seen by MCP.
    const prior = JSON.stringify(rdp.entries);
    await peer.tool('capture_control', { rsp: false, rdp: false });
    assert.equal(JSON.stringify((await peer.tool('rdp_capture', { next_frame: false })).entries), prior);
    await peer.tool('capture_control', { rsp: true, rdp: true });
    await peer.close(); peer = undefined;
    for (let i = 0; i < 100; ++i) { const c = unwrap(await native.request('n64.capture.control', undefined, { rsp: null, rdp: null })); if (!c.rsp && !c.rdp) break; await sleep(10); }
    assert.equal(unwrap(await native.request('n64.capture.control', undefined, { rsp: null, rdp: null })).rsp, false, 'disconnect releases capture owner');
    await endpoint.close(); router.detach(); native.close(); child.stdin.end('quit\n'); assert.equal((await closed)[0], 0, log);
  } finally {
    if (peer) await peer.close(); await endpoint.close(); router.detach(); native.close(); if (child.exitCode === null) { child.kill('SIGKILL'); await closed; } fs.rmSync(dir, { recursive: true, force: true });
  }
});
