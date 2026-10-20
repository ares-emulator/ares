export const NATIVE_STATE_METHODS = ['n64.state.format', 'n64.state.layout', 'n64.state.begin', 'n64.state.read', 'n64.state.release'] as const;
export const STATE_METHODS = ['state.info', 'state.list', 'state.peek', 'state.save'] as const;
export interface StateFormat { version: string; signature: number; header_size: 533; slots: string[] }
export interface StateLayout { version: string; signature: number; header_size: 533; offset: number; size: number; storage: 'lsb32' }
export interface StateInfo {
  path: string; size: number; version: string; signature_valid: boolean; version_supported: boolean;
  compatibility: 'bad_signature' | 'unsupported_version' | 'header_supported_configuration_unverified';
  capture_epoch: number | null; capture_frame: number | null; header_only: boolean;
  metadata_status: 'unchecked' | 'missing_or_stale' | 'validated' | 'invalid'; modified_time_unix: number; rom_sha256?: string;
}
export interface StatePeekParams { path: string; address: string; length: number; view: 'rdram'; assumeCurrentConfiguration: boolean }
export interface StateList { entries: (StateInfo | { path: string; error: string } | { path: string; exists: false })[]; offset: number; total: number; truncated: boolean; next_offset: number | null }
export interface StateSnapshotResult { snapshot_id: number; bytes: number; version: string; epoch: number; frame: number; offset: number; size: number; storage: 'lsb32'; rom_sha256: string; pc: string; stopped: true; mid_frame_cpu_stop: boolean }
export interface StateSaveResult { path: string; size: number; bytes: number; sha256: string; version: string; epoch: number; frame: number; pc: string; stopped: true; mid_frame_cpu_stop: boolean; metadata_path?: string; metadata_error?: string }
export interface StateServiceMap {
  'n64.state.begin': { params: undefined; result: StateSnapshotResult };
  'n64.state.read': { params: { snapshotId: number; offset: number; length: number }; result: { snapshot_id: number; offset: number; length: number; hex: string } };
  'n64.state.release': { params: { snapshotId: number }; result: { released: true } };
  'state.save': { params: { path: string; overwrite: boolean; metadata: boolean }; result: StateSaveResult };
  'n64.state.format': { params: undefined; result: StateFormat };
  'n64.state.layout': { params: undefined; result: StateLayout };
  'state.info': { params: { path: string }; result: StateInfo };
  'state.list': { params: { directory: string | null; offset: number; limit: number }; result: StateList };
  'state.peek': { params: StatePeekParams; result: { path: string; addr: string; paddr: string; length: number; view: 'rdram'; layout_source: 'hash_validated_sidecar' | 'current_configuration_assumed'; hex: string; capture_epoch: number | null; capture_frame: number | null; active_machine_unchanged: true } };
}
const integer = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const path = (v: unknown): v is string => typeof v === 'string' && v.length > 0 && v.length <= 4096 && !v.includes('\0');
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function validStateRequest(method: string, p: any) {
  if (method === 'n64.state.read') return keys(p, 'length,offset,snapshotId') && integer(p.snapshotId) && p.snapshotId > 0 && integer(p.offset, 67108864) && integer(p.length, 16384) && p.length > 0;
  if (method === 'n64.state.release') return keys(p, 'snapshotId') && integer(p.snapshotId) && p.snapshotId > 0;
  if (method.startsWith('n64.')) return p === undefined;
  if (method === 'state.save') return keys(p, 'metadata,overwrite,path') && path(p.path) && typeof p.overwrite === 'boolean' && typeof p.metadata === 'boolean';
  if (method === 'state.info') return keys(p, 'path') && path(p.path);
  if (method === 'state.list') return keys(p, 'directory,limit,offset') && (p.directory === null || path(p.directory)) && integer(p.offset, 4096) && integer(p.limit, 1000) && p.limit > 0;
  return keys(p, 'address,assumeCurrentConfiguration,length,path,view') && path(p.path) && typeof p.address === 'string' && /^0x[0-9a-f]{1,16}$/i.test(p.address) && integer(p.length, 1024) && p.length > 0 && p.view === 'rdram' && typeof p.assumeCurrentConfiguration === 'boolean';
}
export function validStateResult(method: string, s: any): boolean {
  if (!s) return false;
  if (method === 'n64.state.release') return s.released === true;
  if (method === 'n64.state.read') return integer(s.snapshot_id) && s.snapshot_id > 0 && integer(s.offset, 67108864) && integer(s.length, 16384) && s.length > 0 && typeof s.hex === 'string' && s.hex.length === s.length * 2 && /^[a-f0-9]+$/.test(s.hex);
  if (method === 'state.save' || method === 'n64.state.begin') {
    if (!(typeof s.version === 'string' && s.version.length > 0 && s.version.length <= 16 && integer(s.epoch) && s.epoch > 0 && integer(s.frame) && typeof s.pc === 'string' && /^0x[0-9a-f]{1,16}$/.test(s.pc) && s.stopped === true && typeof s.mid_frame_cpu_stop === 'boolean' && integer(s.bytes, 67108864) && s.bytes >= 533)) return false;
    return method === 'state.save' ? path(s.path) && s.size === s.bytes && typeof s.sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.sha256) && (s.metadata_path === undefined || path(s.metadata_path)) && (s.metadata_error === undefined || typeof s.metadata_error === 'string') : integer(s.snapshot_id) && s.snapshot_id > 0 && integer(s.offset) && s.offset >= 533 && integer(s.size, 8388608) && s.size > 0 && s.size % 4 === 0 && s.offset + s.size <= s.bytes && s.storage === 'lsb32' && typeof s.rom_sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.rom_sha256);
  }
  if (method.startsWith('n64.')) return typeof s.version === 'string' && s.version.length > 0 && s.version.length <= 16 && integer(s.signature, 0xffffffff) && s.header_size === 533 && (method === 'n64.state.format' ? Array.isArray(s.slots) && s.slots.length <= 9 && s.slots.every(path) : integer(s.offset, 67108864) && s.offset >= 533 && integer(s.size, 8388608) && s.size > 0 && s.size % 4 === 0 && s.storage === 'lsb32');
  if (method === 'state.info') return path(s.path) && integer(s.size, 67108864) && s.size >= 533 && typeof s.version === 'string' && s.version.length <= 16 && typeof s.signature_valid === 'boolean' && typeof s.version_supported === 'boolean' && ['bad_signature', 'unsupported_version', 'header_supported_configuration_unverified'].includes(s.compatibility) && [s.capture_epoch, s.capture_frame].every(v => v === null || integer(v)) && typeof s.header_only === 'boolean' && ['unchecked', 'missing_or_stale', 'validated', 'invalid'].includes(s.metadata_status) && Number.isSafeInteger(s.modified_time_unix) && (s.rom_sha256 === undefined || typeof s.rom_sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.rom_sha256));
  if (method === 'state.list') return Array.isArray(s.entries) && s.entries.length <= 1000 && s.entries.every((entry: any) => path(entry.path) && (typeof entry.error === 'string' || entry.exists === false || validStateResult('state.info', entry))) && integer(s.offset, 4096) && integer(s.total, 4096) && typeof s.truncated === 'boolean' && (s.next_offset === null || integer(s.next_offset, 4096));
  return path(s.path) && typeof s.addr === 'string' && /^0x[0-9a-f]{1,16}$/.test(s.addr) && typeof s.paddr === 'string' && /^0x[0-9a-f]{1,8}$/.test(s.paddr) && integer(s.length, 1024) && s.length > 0 && s.view === 'rdram' && ['hash_validated_sidecar', 'current_configuration_assumed'].includes(s.layout_source) && typeof s.hex === 'string' && s.hex.length === s.length * 2 && /^[0-9a-f]+$/.test(s.hex) && [s.capture_epoch, s.capture_frame].every(v => v === null || integer(v)) && s.active_machine_unchanged === true;
}
