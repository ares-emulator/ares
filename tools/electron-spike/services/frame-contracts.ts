export const FRAME_METHODS = ['video.begin', 'video.read', 'video.release'] as const;
export type FrameSource = 'presented' | 'framebuffer';
export interface FrameSnapshot { snapshot_id: number; width: number; height: number; bytes: number; epoch: number; frame: number; current_frame: number; frame_kind: 'vi' | 'synthetic-vi' | 'unavailable'; sequence: number; source: FrameSource; origin: string | null; color_depth: number; nonblack_pixels: number | null; format: 'rgba8' }
export interface FrameServiceMap {
  'video.begin': { params: { source: FrameSource }; result: FrameSnapshot };
  'video.read': { params: { snapshotId: number; offset: number; length: number }; result: { snapshot_id: number; offset: number; length: number; hex: string } };
  'video.release': { params: { snapshotId: number }; result: { released: true } };
}
const integer = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function encodeFrame(method: string, p: any): string | undefined {
  if (method === 'video.begin') return keys(p, 'source') && ['presented', 'framebuffer'].includes(p.source) ? ` ${p.source}` : undefined;
  if (method === 'video.release') return keys(p, 'snapshotId') && integer(p.snapshotId) && p.snapshotId ? ` ${p.snapshotId}` : undefined;
  if (keys(p, 'length,offset,snapshotId') && integer(p.snapshotId) && p.snapshotId && integer(p.offset, 16777216) && integer(p.length, 16384) && p.length) return ` ${p.snapshotId} ${p.offset} ${p.length}`;
}
export function validFrame(method: string, s: any): boolean {
  if (!s) return false;
  if (method === 'video.release') return s.released === true;
  if (!integer(s.snapshot_id) || !s.snapshot_id) return false;
  if (method === 'video.read') return integer(s.offset, 16777216) && integer(s.length, 16384) && s.length > 0 && typeof s.hex === 'string' && s.hex.length === s.length * 2 && /^[a-f0-9]+$/.test(s.hex);
  return integer(s.width, 4194304) && s.width > 0 && integer(s.height, 4194304) && s.height > 0 && s.width * s.height <= 4194304 && s.bytes === s.width * s.height * 4 && integer(s.epoch) && s.epoch > 0 && integer(s.frame) && integer(s.current_frame) && ['vi', 'synthetic-vi', 'unavailable'].includes(s.frame_kind) && integer(s.sequence) && ['presented', 'framebuffer'].includes(s.source) && s.format === 'rgba8' && (s.source === 'framebuffer' ? typeof s.origin === 'string' && /^0x[a-f0-9]{1,8}$/.test(s.origin) && [2, 3].includes(s.color_depth) && integer(s.nonblack_pixels, s.width * s.height) : s.origin === null && s.color_depth === 0 && s.nonblack_pixels === null);
}
