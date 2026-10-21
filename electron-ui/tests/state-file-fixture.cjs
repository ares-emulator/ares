// Original artificial offline-inspection fixture, NOT a loadable emulator state.
const fs = require('node:fs');
const { createHash } = require('node:crypto');
function writeStateFixture(file) {
  const bytes = Buffer.alloc(549); bytes.writeUInt32LE(0x31545342); bytes[4] = 1; bytes.write('v153', 5);
  for (let i = 0; i < 16; i++) bytes[533 + (i ^ 3)] = 0x40 + i;
  fs.writeFileSync(file, bytes);
  fs.writeFileSync(file + '.json', JSON.stringify({ metadata_version: 1, serializer_version: 'v153', state_size: bytes.length, state_sha256: createHash('sha256').update(bytes).digest('hex'), epoch: 3, frame: 7, rdram_layout: { version: 'v153', offset: 533, size: 16, storage: 'lsb32' } }));
}
module.exports = { writeStateFixture };
