'use strict';
const HEADER = 48;
const MAX_BYTES = 1920 * 1080 * 4;
// Incremental parser: each payload is allocated once, even with fragmented pipes.
class FrameParser {
  constructor(onFrame) {
    this.onFrame = onFrame;
    this.header = Buffer.alloc(HEADER);
    this.used = 0;
    this.frame = null;
  }
  push(chunk) {
    let offset = 0;
    while (offset < chunk.length) {
      const target = this.frame?.pixels ?? this.header;
      const count = Math.min(target.length - this.used, chunk.length - offset);
      chunk.copy(target, this.used, offset, offset + count);
      this.used += count; offset += count;
      if (this.used !== target.length) continue;
      this.used = 0;
      if (this.frame) {
        const complete = this.frame; this.frame = null;
        this.onFrame(complete);
      } else {
        const h = this.header;
        const size = h.readUInt32LE(4), width = h.readUInt32LE(8), height = h.readUInt32LE(12);
        if (h.toString('ascii', 0, 4) !== 'AEF2' || !width || !height ||
            size > MAX_BYTES || size !== width * height * 4 ||
            !Number.isFinite(h.readDoubleLE(40)) || h.readDoubleLE(40) <= 0) throw new Error('Invalid AEF2 frame');
        this.frame = { width, height, sequence: h.readUInt32LE(16), inputSequence: h.readUInt32LE(20),
          nativeMs: h.readDoubleLE(24), queueMs: h.readDoubleLE(32), aspect: h.readDoubleLE(40), pixels: Buffer.allocUnsafe(size) };
      }
    }
  }
}
module.exports = { FrameParser, HEADER };
