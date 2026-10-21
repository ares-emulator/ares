'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const path = require('node:path');
const { execFileSync } = require('node:child_process');
const fs = require('node:fs');
const {queueText,elfImage,queueConfig} = require('./rspq-fixture.cjs');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const { readViewerCapture } = require('../dist/mcp/viewer-tools.js');
const identity = { session: 'viewer-fixture', epoch: 9 };
const fixture = process.env.ARES_CPU_FIXTURE || path.resolve(__dirname, '../../../build-electron-make/bin/ares-electron-cpu-fixture');
test('viewer DTOs validate real JIT/interpreter capture and bounded generation pages', () => {
  const output = execFileSync(fixture, { encoding: 'utf8', timeout: 10000 });
  for (const [tag, method] of [['VIEWER_RSP', 'n64.capture.rsp'], ['VIEWER_RDP', 'n64.capture.rdp'], ['VIEWER_CONTROL', 'n64.capture.control']]) {
    const result = JSON.parse(output.split('\n').find(l => l.startsWith(tag + ' ')).slice(tag.length + 1));
    const reply = { protocol: 1, id: 'fixture', ...identity, ok: true, result };
    decodeReply(JSON.stringify(reply), method);
    if (tag === 'VIEWER_RSP') result.entries[0].cycle = 0x100000000;
    else if (tag === 'VIEWER_RDP') result.entries[0].word0 = 0;
    else result.capacity = 8192;
    assert.throws(() => decodeReply(JSON.stringify(reply), method), /invalid_reply/);
  }
  for (const p of [{ generation: null, offset: 0, limit: 65 }, { generation: -1, offset: 0, limit: 1 }]) assert.throws(() => encodeRequest('x', identity, 'n64.capture.rsp', p), /invalid_request/);
  assert.throws(() => encodeRequest('x', identity, 'n64.capture.control', { rsp: 1, rdp: null }), /invalid_request/);
});
test('viewer MCP aggregation bounds inline rows and refuses mixed committed generations', async () => {
  let swapped = false;
  const client = { request: async (_method, _identity, p) => {
    const count = Math.min(p.limit, 130 - p.offset), generation = swapped && p.offset ? 3 : 2;
    return { protocol: 1, id: 'fake', ...identity, ok: true, result: { epoch: 9, generation, kind: 'rdp', mode: 'snapshot', frame_kind: 'vi_origin_swaps', presented_frame: 1, configured: true, rspq_supported: false, committed: 130, retained: 130, dropped: 0, truncated: false, capacity: 4096, offset: p.offset,
      entries: Array.from({ length: count }, (_, i) => ({ seq: p.offset + i, frame: 0, opcode: '0x0', name: 'No_Operation', word0: '0x1234', word1: '0x0', wordCount: 1 })), next_offset: p.offset + count < 130 ? p.offset + count : null } };
  } };
  const all = await readViewerCapture(client, identity, 'rdp', 4096);
  assert.equal(all.count, 130); assert.equal(all.entries[129].seq, 129); assert.equal(all.inline_limit_effective, 1024);
  assert.equal((await readViewerCapture(client, identity, 'rdp', 0)).count, 0);
  const partial = await readViewerCapture(client, identity, 'rdp', 64); assert.equal(partial.count, 64); assert.equal(partial.truncated, true);
  swapped = true; await assert.rejects(readViewerCapture(client, identity, 'rdp', 130), { code: 'stale_capture' });
});

test('RSPQ original ELF/config fixture validates live JIT/interpreter semantics and immutable metadata', () => {
  const dir=fs.mkdtempSync('/tmp/ares-rspq-fixture-');
  try {
    const rom=path.join(dir,'fixture.z64');fs.writeFileSync(path.join(dir,'fixture.elf'),elfImage());
    fs.writeFileSync(path.join(dir,'queue.bin'),queueText());fs.writeFileSync(path.join(dir,'rspq-libdragon.json'),JSON.stringify(queueConfig()));
    const output=execFileSync(fixture,[rom],{encoding:'utf8',timeout:10000});
    const result=JSON.parse(output.split('\n').find(l=>l.startsWith('VIEWER_RSPQ ')).slice(12));
    decodeReply(JSON.stringify({protocol:1,id:'fixture',session:'rspq-fixture',epoch:11,ok:true,result}),'n64.capture.rsp');
    assert.equal(result.entries.find(r=>r.command==='Fixture Signed').args,'n=-2 addr=0x12345678');
    assert.equal(result.entries.at(-1).words_truncated,true);
    result.entries[0].words=Array(17).fill('0x0');
    assert.throws(()=>decodeReply(JSON.stringify({protocol:1,id:'fixture',session:'rspq-fixture',epoch:11,ok:true,result}),'n64.capture.rsp'),/invalid_reply/);
  } finally {fs.rmSync(dir,{recursive:true,force:true});}
});
