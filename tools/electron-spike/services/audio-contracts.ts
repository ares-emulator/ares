export const AUDIO_METHODS = ['audio.capture', 'audio.status', 'audio.begin', 'audio.read', 'audio.release'] as const;
export interface AudioServiceMap {
  'audio.capture': { params: { frames: number }; result: { capturing: true; cap: number; rate: 48000; seconds: number } };
  'audio.status': { params: undefined; result: { available: boolean; active: boolean; count: number; cap: number; rate: 48000; done: boolean } };
  'audio.begin': { params: undefined; result: { snapshot_id: number; frames: number; bytes: number; rate: 48000 } };
  'audio.read': { params: { snapshotId: number; offset: number; length: number }; result: { snapshot_id: number; offset: number; length: number; hex: string } };
  'audio.release': { params: { snapshotId: number; consume: boolean }; result: { released: true } };
  'artifact.audioSave': { params: { path: string }; result: { path: string; frames: number; rate: 48000; seconds: number; bytes: number; sha256: string; epoch: number; cleanup_error?: string } };
}
const int = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function encodeAudio(method: string, p: any): string | undefined {
  if (method === 'artifact.audioSave') return keys(p, 'path') && typeof p.path === 'string' && p.path.length > 0 && p.path.length <= 4096 && !p.path.includes('\0') ? '' : undefined;
  if (method === 'audio.status' || method === 'audio.begin') return p === undefined ? '' : undefined;
  if (method === 'audio.capture') return keys(p, 'frames') && int(p.frames, 192000) ? ` ${p.frames}` : undefined;
  if (method === 'audio.release') return keys(p, 'consume,snapshotId') && int(p.snapshotId) && p.snapshotId > 0 && typeof p.consume === 'boolean' ? ` ${p.snapshotId} ${p.consume ? 1 : 0}` : undefined;
  if (method === 'audio.read') return keys(p, 'length,offset,snapshotId') && int(p.snapshotId) && p.snapshotId > 0 && int(p.offset, 768000) && int(p.length, 16384) && p.length > 0 ? ` ${p.snapshotId} ${p.offset} ${p.length}` : undefined;
}
export function validAudio(method: string, s: any): boolean {
  if (!s) return false;
  if (method === 'audio.release') return s.released === true;
  if (method === 'audio.read') return int(s.snapshot_id) && s.snapshot_id > 0 && int(s.offset, 768000) && int(s.length, 16384) && s.length > 0 && typeof s.hex === 'string' && s.hex.length === s.length * 2 && /^[a-f0-9]+$/.test(s.hex);
  if (s.rate !== 48000) return false;
  if (method === 'audio.capture') return s.capturing === true && int(s.cap, 192000) && Math.abs(s.seconds - s.cap / 48000) <= 0.000001;
  if (method === 'audio.status') return typeof s.available === 'boolean' && typeof s.active === 'boolean' && int(s.count, 192000) && int(s.cap, 192000) && s.count <= s.cap && s.done === (!s.active || s.count >= s.cap);
  if (!int(s.frames, 192000)) return false;
  if (method === 'audio.begin') return int(s.snapshot_id) && s.snapshot_id > 0 && s.bytes === s.frames * 4;
  return typeof s.path === 'string' && s.path.length > 0 && s.path.length <= 4096 && s.seconds === s.frames / 48000 && s.bytes === 44 + s.frames * 4 && int(s.epoch) && s.epoch > 0 && typeof s.sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.sha256) && (s.cleanup_error === undefined || typeof s.cleanup_error === 'string');
}
