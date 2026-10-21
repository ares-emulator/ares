const { test } = require('node:test');
const assert = require('node:assert/strict');
const { FrameParser } = require('./dist/protocol.js');
function packet(sequence) {
  const b = Buffer.alloc(72); b.write('AEF2'); b.writeUInt32LE(24, 4);
  b.writeUInt32LE(3, 8); b.writeUInt32LE(2, 12); b.writeUInt32LE(sequence, 16);
  b.writeUInt32LE(7, 20); b.writeDoubleLE(123.5, 24); b.writeDoubleLE(0.25, 32);
  b.writeDoubleLE(1.5, 40);
  for (let i = 48; i < b.length; i++) b[i] = i;
  return b;
}
test('parses arbitrary pipe fragmentation and coalesced frames', () => {
  for (const chunkSize of [1, 3, 47, 48, 49, 72, 1000]) {
    const result = [], parser = new FrameParser(f => result.push(f));
    const bytes = Buffer.concat([packet(1), packet(2), packet(3)]);
    for (let i = 0; i < bytes.length; i += chunkSize) parser.push(bytes.subarray(i, i + chunkSize));
    assert.deepEqual(result.map(f => f.sequence), [1, 2, 3]);
    assert.deepEqual(result[0].pixels, packet(1).subarray(48));
    assert.equal(result[0].nativeMs, 123.5); assert.equal(result[0].inputSequence, 7);
  }
});
test('rejects malformed or oversized payloads before allocating', () => {
  for (const mutate of [b => b.write('NOPE'), b => b.writeUInt32LE(0, 8),
    b => b.writeUInt32LE(0xffffffff, 4), b => b.writeUInt32LE(3, 12), b => b.writeDoubleLE(NaN, 40)]) {
    const b = packet(1); mutate(b);
    assert.throws(() => new FrameParser(() => {}).push(b), /Invalid AEF2/);
  }
});
