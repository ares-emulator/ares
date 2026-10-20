'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const { createHash } = require('node:crypto');
const { publishArtifact } = require('../dist/services/artifact-writer.js');
const { TraceArtifacts } = require('../dist/services/trace-artifacts.js');
const { encodeRequest, decodeReply, ServiceError } = require('../dist/services/contracts.js');
const identity = { session: 'artifact-fixture', epoch: 1 };
const read = async (file) => JSON.parse(await fs.readFile(file, 'utf8'));
function page(offset, total = 130) {
  const count = Math.min(64, total - offset);
  return { operation_id: 7, kind: 'pc', epoch: 1, start_frame: 0, end_frame: 1, stop_reason: 'frames_done', record_count: total, storage_record_limit: 262144, offset, complete: true, truncated: false, unique_pcs: total, counted_instructions: total, frames_traced: 1, dropped_records: 0, overflow_observations: 0, traced_instructions: total, instructions_observed: total, calls_observed: 0, excluded_callsite: 0, excluded_target: 0, records: Array.from({ length: count }, (_, i) => ({ pc: `0x${(0xffffffff80001000n + BigInt((offset + i) * 4)).toString(16)}`, frame: 0, count: 1 })), next_offset: offset + count < total ? offset + count : null };
}
const fake = (request) => ({ identity: { ...identity }, request: async (_method, _identity, p) => ({ protocol: 1, id: 'fixture', ...identity, ok: true, result: await request(p) }) });
test('artifact publication: concurrent no-overwrite, overwrite, failed/oversized/cancelled producer and symlinks', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-artifacts-');
  const destination = path.join(dir, 'nested', 'Unicode é file.json');
  try {
    const write = text => publishArtifact(destination, false, async out => { await out(text); }, () => {});
    const raced = await Promise.allSettled([write('first'), write('second')]);
    assert.equal(raced.filter(r => r.status === 'fulfilled').length, 1);
    assert.equal(raced.find(r => r.status === 'rejected').reason.code, 'file_exists');
    const prior = await fs.readFile(destination, 'utf8');
    await assert.rejects(publishArtifact(destination, true, async out => { await out('partial'); throw new Error('producer failed'); }, () => {}), { code: 'write_failed' });
    assert.equal(await fs.readFile(destination, 'utf8'), prior);
    await assert.rejects(publishArtifact(destination, true, out => out('too big'), () => {}, 2), { code: 'artifact_too_large' });
    let revoked = false;
    await assert.rejects(publishArtifact(destination, true, async out => { await out('partial'); revoked = true; }, () => { if (revoked) throw new ServiceError('artifact_cancelled'); }), { code: 'artifact_cancelled' });
    assert.equal(await fs.readFile(destination, 'utf8'), prior);
    const replaced = await publishArtifact(destination, true, async out => { await out('full'); await out(' replacement'); }, () => {});
    assert.equal(await fs.readFile(destination, 'utf8'), 'full replacement');
    assert.equal(replaced.sha256, createHash('sha256').update('full replacement').digest('hex'));
    assert.equal(replaced.bytes, 16);
    const dangling = path.join(dir, 'dangling'); await fs.symlink('missing', dangling);
    await assert.rejects(publishArtifact(dangling, false, out => out('data'), () => {}), { code: 'file_exists' });
    assert.equal((await fs.lstat(dangling)).isSymbolicLink(), true);
    assert.deepEqual((await fs.readdir(path.dirname(destination))).sort(), [path.basename(destination)]);
    await assert.rejects(publishArtifact(path.join(destination, 'bad-parent'), false, out => out('data'), () => {}));
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
test('trace export streams all bounded pages with identities/hash and revokes stale or disconnected work', async () => {
  const dir = await fs.mkdtemp('/tmp/ares-trace-export-'); const destination = path.join(dir, 'trace.json');
  try {
    const service = new TraceArtifacts(); const client = fake(p => page(p.offset));
    const result = await service.export(client, identity, { operationId: 7, path: destination, overwrite: false }, 'owner', () => true);
    const artifact = await read(destination);
    assert.equal(artifact.version, 1); assert.equal(artifact.kind, 'trace_pc'); assert.equal(artifact.entries.length, 130);
    assert.equal(artifact.entries[129].pc, '0xffffffff80001204'); assert.equal(artifact.summary.session, identity.session);
    assert.equal(result.entries, 130); assert.equal(result.sha256, createHash('sha256').update(await fs.readFile(destination)).digest('hex'));
    assert.equal(decodeReply(JSON.stringify({ protocol: 1, id: 'x', ...identity, ok: true, result }), 'artifact.traceExport').result.path, destination);
    assert.throws(() => encodeRequest('x', identity, 'artifact.traceExport', { operationId: 7, path: 'bad\0path', overwrite: false }), /invalid_request/);
    let release; const waiting = new Promise(resolve => release = resolve); let entered;
    const started = new Promise(resolve => entered = resolve);
    const slow = fake(async p => { if (p.offset) { entered(); await waiting; } return page(p.offset); });
    const exportJob = service.export(slow, identity, { operationId: 7, path: destination, overwrite: true }, 'owner', () => true);
    await started;
    await assert.rejects(service.export(client, identity, { operationId: 7, path: destination, overwrite: true }, 'owner', () => true), { code: 'busy' });
    service.release('owner'); release();
    await assert.rejects(exportJob, { code: 'artifact_cancelled' });
    assert.equal((await read(destination)).entries.length, 130, 'failed export preserves previous artifact');
    const stale = fake(p => { if (p.offset) stale.identity.epoch = 2; return page(p.offset); });
    await assert.rejects(service.export(stale, identity, { operationId: 7, path: destination, overwrite: true }, 'fresh-owner', () => true), { code: 'stale_epoch' });
    assert.deepEqual(await fs.readdir(dir), ['trace.json']);
  } finally { await fs.rm(dir, { recursive: true, force: true }); }
});
