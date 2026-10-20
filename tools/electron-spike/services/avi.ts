import type { RecordingStatus } from './recording-contracts.js';
/** AVI 1.0 DIB headers/index matching the reference video-recording.hpp layout. */
export function aviParts(s: RecordingStatus) {
  const bytes: number[] = [];
  const word = (v: number) => bytes.push(v & 255, v >>> 8 & 255);
  const dword = (v: number) => { word(v); word(v >>> 16); };
  const fourcc = (v: string) => bytes.push(...Buffer.from(v, 'ascii'));
  const patch = (at: number, v: number) => { for (let n = 0; n < 4; n++) bytes[at + n] = v >>> (n * 8) & 255; };
  const list = (name: string) => { fourcc('LIST'); const at = bytes.length; dword(0); fourcc(name); return at; };
  const end = (at: number) => patch(at, bytes.length - at - 4);
  const frameBytes = s.captured_bytes / s.frames;
  fourcc('RIFF'); dword(0); fourcc('AVI ');
  const hdrl = list('hdrl');
  fourcc('avih'); dword(56); dword(Math.floor(1000000 / s.fps)); dword(frameBytes * s.fps); dword(0); dword(0x10);
  dword(s.frames); dword(0); dword(1); dword(frameBytes); dword(s.width); dword(s.height); for (let n = 0; n < 4; n++) dword(0);
  const strl = list('strl');
  fourcc('strh'); dword(56); fourcc('vids'); fourcc('DIB '); dword(0); word(0); word(0);
  dword(0); dword(1); dword(s.fps); dword(0); dword(s.frames); dword(frameBytes); dword(0xffffffff); dword(0);
  word(0); word(0); word(s.width); word(s.height);
  fourcc('strf'); dword(40); dword(40); dword(s.width); dword(s.height); word(1); word(24);
  dword(0); dword(frameBytes); dword(0); dword(0); dword(0); dword(0);
  end(strl); end(hdrl);
  const movi = list('movi'), chunkBytes = frameBytes + 8;
  patch(movi, 4 + s.frames * chunkBytes);
  const size = bytes.length + s.frames * chunkBytes + 8 + s.frames * 16;
  patch(4, size - 8);
  const header = Buffer.from(bytes), chunk = Buffer.alloc(8), index = Buffer.alloc(8 + s.frames * 16);
  chunk.write('00db'); chunk.writeUInt32LE(frameBytes, 4); index.write('idx1'); index.writeUInt32LE(s.frames * 16, 4);
  for (let n = 0; n < s.frames; n++) {
    const at = 8 + n * 16; index.write('00db', at); index.writeUInt32LE(0x10, at + 4);
    index.writeUInt32LE(4 + n * chunkBytes, at + 8); index.writeUInt32LE(frameBytes, at + 12);
  }
  return { header, chunk, index, frameBytes, size };
}
