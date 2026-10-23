'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { RdpArtifacts } = require('../dist/services/rdp-artifacts.js');
const { encodeRequest, decodeReply } = require('../dist/services/contracts.js');
const identity = { session: 'rdp-artifact', epoch: 1 };
function page(kind, offset) {
  const total = kind === 'words' ? 130 : 1, size = kind === 'words' ? 64 : 1;
  const records = Array.from({ length: Math.min(size, total - offset) }, (_, n) => kind === 'words'
    ? { value: '0x123456789abcdef0', address: '0x1000', batch: 0 }
    : kind === 'batches' ? { id: 0, source: 'RDRAM', dpc_start: '0x1000', current: '0x1000', end: '0x1410', words: 130, first_word: 0, captured_words: 130, cpu_pc: '0xffffffff80001000', rsp_pc: '0x0', rsp_running: false, task: 0 }
    : { id: 0, cpu_pc: '0xffffffff80001000', header: 'ab'.repeat(64), dmem_hash_xxh64: '0x1', imem_hash_xxh64: '0x2', dmem: 'ab'.repeat(4096) });
  const point={pc:'0xffffffff80001000',frame:4,epoch:1,vi:{VI_ORIGIN:'0x2000'},dpc:{DPC_CURRENT:'0x1000'}};
  return { rom:{sha256:'0'.repeat(64),size:4096},renderer:'software',start:point,end:point,rdram_size:8*1024*1024, operation_id: 7, epoch: 1, frame: 4, kind, stop_reason: 'timeout', complete: false, truncated: false, dropped_words: 0, dropped_batches: 0, dropped_tasks: 0, batches_outside_interval: 0,
    word_count: 130, batch_count: 1, task_count: 1, storage_word_limit: 65536, storage_batch_limit: 512, storage_task_limit: 64, dmem_snapshots: true,
    offset, total, records, next_offset: offset + records.length < total ? offset + records.length : null };
}
const fake = fn => ({ identity: { ...identity }, request: async (_method, _identity, p) => ({ protocol: 1, id: 'fixture', ...identity, ok: true, result: await fn(p) }) });
test('RDP capture contracts validate page ranges, task bytes and bounded starts', () => {
  for (const kind of ['words', 'batches', 'tasks']) decodeReply(JSON.stringify({ protocol: 1, id: 'x', ...identity, ok: true, result: page(kind, 0) }), 'n64.rdp.capturePage');
  for (const mutate of [p => p.records[0].batch = 1, p => p.word_count = 65537, p => p.next_offset = 0, p => p.epoch = 2]) {
    const result = page('words', 0); mutate(result);
    assert.throws(() => decodeReply(JSON.stringify({ protocol: 1, id: 'x', ...identity, ok: true, result }), 'n64.rdp.capturePage'), /invalid_reply/);
  }
  assert.throws(() => encodeRequest('x', identity, 'n64.rdp.captureStart', { maxWords: 65537, dmem: false, timeoutMs: 1 }), /invalid_request/);
});
test('RDP export streams every word, batch and task and cancels without replacing an artifact', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-rdp-export-'), destination = path.join(dir, 'capture.json');
  const service = new RdpArtifacts(), client = fake(p => page(p.kind, p.offset));
  try {
    const artifact = await service.export(client, identity, { operationId: 7, path: destination, overwrite: false }, 'owner', () => true);
    const bytes = await fs.readFile(destination), output = JSON.parse(bytes);
    assert.equal(output.rom.sha256,'0'.repeat(64));assert.equal(output.start.frame,4);assert.equal(output.end.frame,4);assert.equal(output.batches[0].dpc_current,'0x1000');
    assert.equal(output.words.length, 130); assert.equal(output.batches[0].captured_words, 130); assert.equal(output.tasks[0].dmem.length, 8192); assert.equal(output.summary.complete, false);
    decodeReply(JSON.stringify({ protocol: 1, id: 'x', ...identity, ok: true, result: artifact }), 'artifact.rdpExport');
    let release, entered; const wait = new Promise(r => release = r), started = new Promise(r => entered = r);
    const slow = fake(async p => { if (p.offset) { entered(); await wait; } return page(p.kind, p.offset); });
    const pending = service.export(slow, identity, { operationId: 7, path: destination, overwrite: true }, 'owner', () => true);
    await started; service.release('owner'); release(); await assert.rejects(pending, { code: 'artifact_cancelled' });
    assert.deepEqual(await fs.readFile(destination), bytes);
    const stale = fake(p => { const result = page(p.kind, p.offset); if (p.offset) result.operation_id++; return result; });
    await assert.rejects(service.export(stale, identity, { operationId: 7, path: destination, overwrite: true }, 'owner', () => true), { code: 'stale_capture' });
    assert.deepEqual(await fs.readdir(dir), ['capture.json']);
  } finally { service.close(); await fs.rm(dir, { recursive: true, force: true }); }
});
