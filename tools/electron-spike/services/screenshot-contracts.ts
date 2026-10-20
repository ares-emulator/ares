import { validFrame, type FrameSource, type FrameSnapshot } from './frame-contracts.js';
export const SCREENSHOT_METHODS = ['artifact.screenshot', 'artifact.imageRead', 'artifact.imageRelease'] as const;
export interface ScreenshotParams { path: string | null; source: FrameSource; region: number[] | null; scale: number; inline: boolean; overwrite: boolean }
export interface ScreenshotResult extends Omit<FrameSnapshot, 'snapshot_id' | 'bytes' | 'width' | 'height' | 'format'> { source_width: number; source_height: number; width: number; height: number; region: number[]; scale: number; bytes: number; sha256: string; path?: string; image_id?: string; format: 'png' }
export interface ScreenshotServiceMap {
  'artifact.screenshot': { params: ScreenshotParams; result: ScreenshotResult };
  'artifact.imageRead': { params: { imageId: string; offset: number; length: number }; result: { image_id: string; offset: number; length: number; base64: string } };
  'artifact.imageRelease': { params: { imageId: string }; result: { released: true } };
}
const integer = (v: unknown, max = Number.MAX_SAFE_INTEGER): v is number => Number.isSafeInteger(v) && (v as number) >= 0 && (v as number) <= max;
const token = (v: unknown): v is string => typeof v === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(v);
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function validScreenshotRequest(method: string, p: any): boolean {
  if (method === 'artifact.imageRelease') return keys(p, 'imageId') && token(p.imageId);
  if (method === 'artifact.imageRead') return keys(p, 'imageId,length,offset') && token(p.imageId) && integer(p.offset, 8388608) && integer(p.length, 16384) && p.length > 0;
  return keys(p, 'inline,overwrite,path,region,scale,source') && (p.path === null || typeof p.path === 'string' && p.path.length > 0 && p.path.length <= 4096 && !p.path.includes('\0')) && ['presented', 'framebuffer'].includes(p.source) && (p.region === null || Array.isArray(p.region) && p.region.length === 4 && p.region.every((v: unknown) => integer(v)) && p.region[2] > 0 && p.region[3] > 0) && typeof p.scale === 'number' && Number.isFinite(p.scale) && p.scale >= 0.125 && p.scale <= 8 && typeof p.inline === 'boolean' && typeof p.overwrite === 'boolean';
}
export function validScreenshotResult(method: string, s: any): boolean {
  if (!s) return false;
  if (method === 'artifact.imageRelease') return s.released === true;
  if (method === 'artifact.imageRead') return token(s.image_id) && integer(s.offset, 8388608) && integer(s.length, 16384) && s.length > 0 && typeof s.base64 === 'string' && s.base64.length === Math.ceil(s.length / 3) * 4 && /^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$/.test(s.base64);
  return validFrame('video.begin', { ...s, snapshot_id: 1, width: s.source_width, height: s.source_height, bytes: s.source_width * s.source_height * 4, format: 'rgba8' }) && s.format === 'png' && ['presented', 'framebuffer'].includes(s.source) && integer(s.epoch) && s.epoch > 0 && integer(s.frame) && integer(s.sequence) && integer(s.source_width) && s.source_width > 0 && integer(s.source_height) && s.source_height > 0 && integer(s.width) && s.width > 0 && integer(s.height) && s.height > 0 && s.width * s.height <= 4194304 && Array.isArray(s.region) && s.region.length === 4 && s.region.every((v: unknown) => integer(v)) && s.region[2] > 0 && s.region[3] > 0 && s.region[0] < s.source_width && s.region[1] < s.source_height && s.region[2] <= s.source_width - s.region[0] && s.region[3] <= s.source_height - s.region[1] && s.width === Math.max(1, Math.round(s.region[2] * s.scale)) && s.height === Math.max(1, Math.round(s.region[3] * s.scale)) && Number.isFinite(s.scale) && s.scale >= 0.125 && s.scale <= 8 && integer(s.bytes, 8388608) && s.bytes > 0 && typeof s.sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.sha256) && (s.path === undefined || typeof s.path === 'string' && s.path.length <= 4096) && (s.image_id === undefined || token(s.image_id));
}
