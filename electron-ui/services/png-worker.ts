import { parentPort, workerData } from 'node:worker_threads';
import { deflateSync } from 'node:zlib';
// Crop nearest-neighbor in source pixels, then encode bounded RGB8 PNG.
const { pixels, sourceWidth, sourceHeight, region, scale } = workerData as { pixels: Uint8Array; sourceWidth: number; sourceHeight: number; region: number[] | null; scale: number };
try {
  const [x, y, w, h] = region ?? [0, 0, sourceWidth, sourceHeight];
  if (!Number.isFinite(scale) || scale < 0.125 || scale > 8 || ![x, y, w, h].every(Number.isSafeInteger) || x < 0 || y < 0 || w <= 0 || h <= 0 || x >= sourceWidth || y >= sourceHeight || w > sourceWidth - x || h > sourceHeight - y || pixels.length !== sourceWidth * sourceHeight * 4) throw new Error('invalid_region');
  const width = Math.max(1, Math.round(w * scale)), height = Math.max(1, Math.round(h * scale));
  if (width * height > 4194304) throw new Error('image_too_large');
  const raw = Buffer.alloc((width * 3 + 1) * height);
  for (let oy = 0; oy < height; oy++) for (let ox = 0; ox < width; ox++) {
    const source = ((y + Math.floor(oy * h / height)) * sourceWidth + x + Math.floor(ox * w / width)) * 4;
    const target = oy * (width * 3 + 1) + 1 + ox * 3;
    raw[target] = pixels[source]; raw[target + 1] = pixels[source + 1]; raw[target + 2] = pixels[source + 2];
  }
  const crcTable = Array.from({ length: 256 }, (_, i) => { let c = i; for (let n = 0; n < 8; n++) c = c & 1 ? 0xedb88320 ^ c >>> 1 : c >>> 1; return c >>> 0; });
  const chunk = (type: string, bytes: Buffer) => {
    const out = Buffer.alloc(bytes.length + 12); out.writeUInt32BE(bytes.length); out.write(type, 4, 'ascii'); bytes.copy(out, 8);
    let crc = 0xffffffff; for (const byte of out.subarray(4, -4)) crc = crcTable[(crc ^ byte) & 255] ^ crc >>> 8;
    out.writeUInt32BE((crc ^ 0xffffffff) >>> 0, out.length - 4); return out;
  };
  const header = Buffer.alloc(13); header.writeUInt32BE(width); header.writeUInt32BE(height, 4); header[8] = 8; header[9] = 2;
  const png = Buffer.concat([Buffer.from('89504e470d0a1a0a', 'hex'), chunk('IHDR', header), chunk('IDAT', deflateSync(raw, { maxOutputLength: 8388608 })), chunk('IEND', Buffer.alloc(0))]);
  if (png.length > 8388608) throw new Error('image_too_large');
  parentPort!.postMessage({ width, height, region: [x, y, w, h], png });
} catch (error) { parentPort!.postMessage({ error: (error as NodeJS.ErrnoException).code === 'ERR_BUFFER_TOO_LARGE' ? 'image_too_large' : error instanceof Error && ['invalid_region', 'image_too_large'].includes(error.message) ? error.message : 'encoding_failed' }); }
