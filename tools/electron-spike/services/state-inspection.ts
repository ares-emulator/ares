import fs from 'node:fs/promises';
import { constants } from 'node:fs';
import path from 'node:path';
import { createHash } from 'node:crypto';
import { performance } from 'node:perf_hooks';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from './contracts.js';
import type { StateFormat, StateInfo, StateList, StatePeekParams } from './state-contracts.js';
const MAX = 64 * 1024 * 1024, HEADER = 533;
const integer = (v: unknown): v is number => Number.isSafeInteger(v) && (v as number) >= 0;
const hash = (v: unknown): v is string => typeof v === 'string' && /^[a-f0-9]{64}$/.test(v);
const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
// Read only regular bounded files, including through symlinks. Nonblocking open
// makes a named pipe an explicit rejection rather than an unbounded main job.
async function read(file: string, max: number, guard: () => void, headerOnly = false) {
  guard();
  const handle = await fs.open(file, constants.O_RDONLY | constants.O_NONBLOCK);
  try {
    guard(); const before = await handle.stat();
    if (!before.isFile()) throw new ServiceError('not_regular_file');
    if (before.size > max) throw new ServiceError('state_too_large');
    const bytes = Buffer.alloc(headerOnly ? Math.min(HEADER, before.size) : before.size);
    for (let at = 0; at < bytes.length;) {
      guard(); const { bytesRead } = await handle.read(bytes, at, Math.min(1048576, bytes.length - at), at);
      if (!bytesRead) throw new ServiceError('state_changed'); at += bytesRead;
    }
    guard(); const after = await handle.stat();
    if (before.size !== after.size || before.mtimeMs !== after.mtimeMs || before.ctimeMs !== after.ctimeMs) throw new ServiceError('state_changed');
    return { bytes, size: before.size, modified: Math.floor(before.mtimeMs / 1000) };
  } finally { await handle.close(); }
}
function header(file: string, data: Awaited<ReturnType<typeof read>>, format: StateFormat): StateInfo {
  const b = data.bytes; if (b.length < HEADER) throw new ServiceError('state_truncated');
  const versionBytes = b.subarray(5, 21), nul = versionBytes.indexOf(0);
  const version = versionBytes.subarray(0, nul < 0 ? 16 : nul).toString('utf8');
  const valid = b.readUInt32LE(0) === format.signature && b[4] <= 1;
  const supported = valid && version === format.version;
  return { path: file, size: data.size, version, signature_valid: valid, version_supported: supported,
    compatibility: !valid ? 'bad_signature' : !supported ? 'unsupported_version' : 'header_supported_configuration_unverified',
    capture_epoch: null, capture_frame: null, header_only: true, metadata_status: 'unchecked', modified_time_unix: data.modified };
}
async function metadata(file: string, bytes: Buffer, version: string, guard: () => void): Promise<{ status: StateInfo['metadata_status']; value?: any }> {
  let raw;
  try { raw = await read(file + '.json', 16384, guard); }
  catch (error) { if (error instanceof ServiceError && ['artifact_cancelled', 'stale_epoch', 'state_timeout'].includes(error.code)) throw error; return { status: (error as NodeJS.ErrnoException).code === 'ENOENT' ? 'missing_or_stale' : 'invalid' }; }
  try {
    const value = JSON.parse(raw.bytes.toString('utf8'));
    if (!value || value.metadata_version !== 1 || value.state_size !== bytes.length || !hash(value.state_sha256) || value.state_sha256 !== createHash('sha256').update(bytes).digest('hex')) return { status: 'missing_or_stale' };
    if (value.serializer_version !== version || [value.epoch, value.frame].some(v => v !== undefined && v !== null && !integer(v)) || value.rom_sha256 !== undefined && !hash(value.rom_sha256)) return { status: 'invalid' };
    return { status: 'validated', value };
  } catch { return { status: 'invalid' }; }
}
export class StateInspection {
  private jobs = new Map<string, AbortController>();
  release(owner: string) { this.jobs.get(owner)?.abort(); }
  close() { for (const job of this.jobs.values()) job.abort(); }
  async inspect(client: SessionClient & { identity: SessionIdentity }, identity: SessionIdentity, method: 'state.info' | 'state.list' | 'state.peek', params: any, owner: string, current: () => boolean): Promise<any> {
    if (this.jobs.has(owner) || this.jobs.size >= 2) throw new ServiceError('busy');
    const job = new AbortController(), deadline = performance.now() + 25000; this.jobs.set(owner, job);
    const guard = () => {
      if (!current() || job.signal.aborted) throw new ServiceError('artifact_cancelled');
      if (client.identity.session !== identity.session || client.identity.epoch !== identity.epoch) throw new ServiceError('stale_epoch');
      if (performance.now() >= deadline) throw new ServiceError('state_timeout');
    };
    try {
      guard(); const format = unwrap(await client.request('n64.state.format', identity, undefined, owner));
      const resolve = (file: string) => { const result = path.resolve(file); if (result.length > 4096) throw new ServiceError('invalid_path'); return result; };
      const info = async (file: string, budget?: { bytes: number }) => {
        const data = await read(file, MAX, guard, true), result = header(file, data, format);
        if (budget && data.size <= budget.bytes) {
          const full = await read(file, Math.min(MAX, budget.bytes), guard);
          if (full.size !== data.size || !full.bytes.subarray(0, HEADER).equals(data.bytes)) throw new ServiceError('state_changed');
          budget.bytes -= full.size;
          const checked = header(file, full, format);
          const meta = await metadata(file, full.bytes, checked.version, guard);
          Object.assign(result, checked, { header_only: false, metadata_status: meta.status });
          if (meta.value) Object.assign(result, { capture_epoch: meta.value.epoch ?? null, capture_frame: meta.value.frame ?? null, ...(meta.value.rom_sha256 === undefined ? {} : { rom_sha256: meta.value.rom_sha256 }) });
        }
        return result;
      };
      if (method === 'state.info') return await info(resolve(params.path));
      if (method === 'state.list') {
        const files: { path: string; slot?: number }[] = []; let truncated = false;
        if (params.directory === null) format.slots.forEach((file, i) => files.push({ path: file, slot: i + 1 }));
        else {
          const directory = await fs.opendir(resolve(params.directory));
          let scanned = 0;
          for await (const entry of directory) {
            guard(); if (++scanned > 4096) { truncated = true; break; }
            if (entry.isFile() && /\.bs[0-9]?$/.test(entry.name)) files.push({ path: path.join(resolve(params.directory), entry.name) });
          }
        }
        files.sort((a, b) => a.path < b.path ? -1 : a.path > b.path ? 1 : 0);
        const entries: StateList['entries'] = [], budget = { bytes: MAX }; let responseBytes = 0, index = params.offset;
        for (; index < files.length && entries.length < params.limit; index++) {
          guard(); const file = files[index]; let entry: any;
          try { entry = { ...await info(file.path, budget), exists: true }; }
          catch (error) {
            if (error instanceof ServiceError && ['artifact_cancelled', 'stale_epoch', 'state_timeout'].includes(error.code)) throw error;
            entry = (error as NodeJS.ErrnoException).code === 'ENOENT' ? { path: file.path, exists: false } : { path: file.path, error: error instanceof ServiceError ? error.code : 'state_read_failed' };
          }
          if (file.slot !== undefined) entry.slot = file.slot;
          const size = Buffer.byteLength(JSON.stringify(entry)); if (responseBytes + size > 48000) break;
          responseBytes += size; entries.push(entry);
        }
        return { entries, offset: params.offset, total: files.length, truncated, next_offset: index < files.length ? index : null };
      }
      const p = params as StatePeekParams, file = resolve(p.path), data = await read(file, MAX, guard), state = header(file, data, format);
      if (!state.version_supported || state.version !== 'v153') throw new ServiceError('unsupported_version');
      const meta = await metadata(file, data.bytes, state.version, guard);
      let layout = meta.value?.rdram_layout, source = 'hash_validated_sidecar';
      if (!layout) {
        if (!p.assumeCurrentConfiguration) throw new ServiceError('layout_unavailable');
        layout = unwrap(await client.request('n64.state.layout', identity, undefined, owner)); source = 'current_configuration_assumed';
      }
      if (!layout || layout.version !== 'v153' || layout.storage !== 'lsb32') throw new ServiceError('unsupported_layout');
      const { offset, size } = layout;
      if (!integer(offset) || offset < HEADER || !integer(size) || !size || size > 8388608 || size % 4 || offset > data.bytes.length || size > data.bytes.length - offset) throw new ServiceError('invalid_layout');
      let address = BigInt(p.address); if (address <= 0xffffffffn && address >= 0x80000000n) address |= 0xffffffff00000000n;
      let physical = address;
      if (address >= 0xffffffff80000000n && address < 0xffffffffc0000000n) physical &= 0x1fffffffn;
      if (physical >= BigInt(size) || BigInt(p.length) > BigInt(size) - physical) throw new ServiceError('out_of_bounds');
      const bytes = Buffer.alloc(p.length); for (let i = 0; i < bytes.length; i++) bytes[i] = data.bytes[offset + ((Number(physical) + i) ^ 3)];
      guard(); return { path: file, addr: `0x${address.toString(16)}`, paddr: `0x${physical.toString(16)}`, length: p.length, view: 'rdram', layout_source: source,
        hex: bytes.toString('hex'), capture_epoch: meta.value?.epoch ?? null, capture_frame: meta.value?.frame ?? null, active_machine_unchanged: true };
    } catch (error) { if (error instanceof ServiceError) throw error; throw new ServiceError('state_read_failed'); }
    finally { if (this.jobs.get(owner) === job) this.jobs.delete(owner); }
  }
}
