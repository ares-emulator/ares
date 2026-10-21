export const RECORDING_METHODS = ['video.record.start', 'video.record.status', 'video.record.begin', 'video.record.read', 'video.record.release'] as const;
export interface RecordingStatus {
  active: boolean; available: boolean; frames: number; fps: number; width: number; height: number; epoch: number; start_frame: number; end_frame: number;
  frame_kind: 'vi' | 'synthetic-vi' | 'unavailable'; stop_reason: string; format: 'avi_raw_bgr24'; audio: false; source: 'presented'; max_frames: number; max_bytes: number; captured_bytes: number;
}
export interface RecordingServiceMap {
  'video.record.start': { params: { maxFrames: number; maxBytes: number; fps: number; timeoutMs: number; discard: boolean }; result: RecordingStatus };
  'video.record.status': { params: undefined; result: RecordingStatus };
  'video.record.begin': { params: undefined; result: RecordingStatus & { snapshot_id: number } };
  'video.record.read': { params: { snapshotId: number; offset: number; length: number }; result: { snapshot_id: number; offset: number; length: number; hex: string } };
  'video.record.release': { params: { snapshotId: number; consume: boolean }; result: { released: true } };
  'artifact.videoStop': { params: { path: string; overwrite: boolean }; result: RecordingStatus & { path: string; size: number; bytes: number; sha256: string; duration_seconds: number; cleanup_error?: string } };
}
const int = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function encodeRecording(method: string, p: any): string | undefined {
  if (method === 'artifact.videoStop') return keys(p, 'overwrite,path') && typeof p.path === 'string' && p.path.length > 0 && p.path.length <= 4096 && !p.path.includes('\0') && typeof p.overwrite === 'boolean' ? '' : undefined;
  if (method === 'video.record.status' || method === 'video.record.begin') return p === undefined ? '' : undefined;
  if (method === 'video.record.start') return keys(p, 'discard,fps,maxBytes,maxFrames,timeoutMs') && int(p.maxFrames, 36000) && p.maxFrames > 0 && int(p.maxBytes, 268435456) && p.maxBytes >= 1048576 && int(p.fps, 120) && p.fps > 0 && int(p.timeoutMs, 3600000) && p.timeoutMs > 0 && typeof p.discard === 'boolean' ? ` ${p.maxFrames} ${p.maxBytes} ${p.fps} ${p.timeoutMs} ${p.discard ? 1 : 0}` : undefined;
  if (method === 'video.record.release') return keys(p, 'consume,snapshotId') && int(p.snapshotId) && p.snapshotId > 0 && typeof p.consume === 'boolean' ? ` ${p.snapshotId} ${p.consume ? 1 : 0}` : undefined;
  if (method === 'video.record.read') return keys(p, 'length,offset,snapshotId') && int(p.snapshotId) && p.snapshotId > 0 && int(p.offset, 268435456) && int(p.length, 16384) && p.length > 0 ? ` ${p.snapshotId} ${p.offset} ${p.length}` : undefined;
}
export function validRecording(method: string, s: any): boolean {
  if (!s) return false;
  if (method === 'video.record.release') return s.released === true;
  if (method === 'video.record.read') return int(s.snapshot_id) && s.snapshot_id > 0 && int(s.offset, 268435456) && int(s.length, 16384) && s.length > 0 && typeof s.hex === 'string' && s.hex.length === s.length * 2 && /^[a-f0-9]+$/.test(s.hex);
  if (!(typeof s.active === 'boolean' && typeof s.available === 'boolean' && int(s.frames, 36000) && int(s.fps, 120) && s.fps > 0 && int(s.width, 4096) && int(s.height, 4096) && int(s.epoch) && int(s.start_frame) && int(s.end_frame) && s.end_frame >= s.start_frame && ['vi', 'synthetic-vi', 'unavailable'].includes(s.frame_kind) && typeof s.stop_reason === 'string' && s.format === 'avi_raw_bgr24' && s.audio === false && s.source === 'presented' && int(s.max_frames, 36000) && s.max_frames > 0 && s.frames <= s.max_frames && int(s.max_bytes, 268435456) && s.max_bytes >= 1048576 && int(s.captured_bytes, s.max_bytes) && s.captured_bytes === s.frames * Math.ceil(s.width * 3 / 4) * 4 * s.height)) return false;
  if (method === 'video.record.begin') return int(s.snapshot_id) && s.snapshot_id > 0 && s.frames > 0 && s.width > 1 && s.height > 1 && s.active === false;
  if (method === 'artifact.videoStop') return typeof s.path === 'string' && s.path.length > 0 && s.path.length <= 4096 && int(s.bytes, 270000000) && s.size === s.bytes && /^[a-f0-9]{64}$/.test(s.sha256) && s.duration_seconds === s.frames / s.fps && (s.cleanup_error === undefined || typeof s.cleanup_error === 'string');
  return true;
}
