// Generates an original, tiny ROM: scrolling tiles, a tone, and joypad palette changes.
// The standard cartridge-header logo is required by the unmodified DMG boot ROM.
const fs = require('node:fs');
const path = require('node:path');
function makeFixture() {
  const rom = Buffer.alloc(32768);
  rom.set([0xc3, 0x50, 0x01], 0x100);
  rom.set(Buffer.from('CEED6666CC0D000B03730083000C000D0008111F8889000EDCCC6EE6DDDDD999BBBB67636E0EECCCDDDC999FBBB9333E', 'hex'), 0x104);
  rom.write('ARES SPIKE', 0x134, 'ascii');
  let check = 0; for (let i = 0x134; i <= 0x14c; i++) check = (check - rom[i] - 1) & 255;
  rom[0x14d] = check;
  const code = [], labels = new Map(), jumps = [];
  const emit = (...v) => code.push(...v);
  const label = name => labels.set(name, code.length);
  const jump = (opcode, name) => { emit(opcode, 0); jumps.push([code.length - 1, name]); };
  const io = (register, value) => emit(0x3e, value, 0xe0, register);
  emit(0xf3, 0x31, 0xfe, 0xff); // DI; LD SP,$fffe
  label('vblank'); emit(0xf0, 0x44, 0xfe, 144); jump(0x38, 'vblank');
  io(0x40, 0); // disable LCD while constructing tile/map
  emit(0x21, 0x00, 0x80, 0x06, 16);
  label('tile'); emit(0x3e, 0xaa, 0x22, 0x05); jump(0x20, 'tile');
  emit(0x21, 0x00, 0x98, 0x01, 0x00, 0x04);
  label('map'); emit(0xaf, 0x22, 0x0b, 0x78, 0xb1); jump(0x20, 'map');
  io(0x47, 0xe4); io(0x42, 0); io(0x43, 0); io(0x40, 0x91);
  io(0x26, 0x80); io(0x24, 0x33); io(0x25, 0x11);
  io(0x11, 0x80); io(0x12, 0x80); io(0x13, 0); io(0x14, 0x87);
  label('frame'); emit(0xf0, 0x44, 0xfe, 144); jump(0x38, 'frame');
  emit(0xf0, 0x43, 0x3c, 0xe0, 0x43); // scroll once per vblank
  io(0x00, 0x10); emit(0xf0, 0x00, 0xe6, 0x0f, 0xfe, 0x0f);
  jump(0x28, 'normal'); io(0x47, 0x1b); jump(0x18, 'wait');
  label('normal'); io(0x47, 0xe4);
  label('wait'); emit(0xf0, 0x44, 0xfe, 144); jump(0x30, 'wait'); jump(0x18, 'frame');
  for (const [offset, name] of jumps) {
    const delta = labels.get(name) - offset - 1;
    if (delta < -128 || delta > 127) throw new Error('Fixture branch out of range');
    code[offset] = delta & 255;
  }
  rom.set(code, 0x150); return rom;
}
if (require.main === module) {
  const target = process.argv[2] || path.join(__dirname, 'artifacts/spike.gb');
  fs.mkdirSync(path.dirname(target), { recursive: true }); fs.writeFileSync(target, makeFixture());
  console.log(target);
}
module.exports = { makeFixture };

// Original NROM demo; fills the backdrop and cycles its color every vblank.
function makeNesFixture() {
  const rom = Buffer.alloc(16 + 16384 + 8192);
  rom.set([0x4e, 0x45, 0x53, 0x1a, 1, 1], 0);
  const code = [], labels = new Map(), jumps = [];
  const emit = (...bytes) => code.push(...bytes);
  const label = name => labels.set(name, code.length);
  const branch = (opcode, name) => { emit(opcode, 0); jumps.push([code.length - 1, name]); };
  const store = (address, value) => emit(0xa9, value, 0x8d, address & 255, address >> 8);
  emit(0x78, 0xd8, 0xa2, 0xff, 0x9a); // SEI; CLD; stack
  store(0x2000, 0); store(0x2001, 0); store(0x4010, 0);
  label('boot1'); emit(0x2c, 2, 0x20); branch(0x10, 'boot1');
  label('boot2'); emit(0x2c, 2, 0x20); branch(0x10, 'boot2');
  emit(0xa2, 0);
  label('frame'); emit(0x2c, 2, 0x20); branch(0x10, 'frame');
  store(0x2006, 0x3f); store(0x2006, 0);
  emit(0xe8, 0x8a, 0x29, 0x3f, 0x8d, 7, 0x20); // palette color
  store(0x2006, 0x3f); store(0x2006, 0); // palette address while display disabled
  branch(0x50, 'frame'); // V cleared by BIT above (vblank status bit6 is zero)
  for (const [offset, name] of jumps) code[offset] = (labels.get(name) - offset - 1) & 255;
  rom.set(code, 16);
  for (const offset of [0x3ffa, 0x3ffc, 0x3ffe]) rom.writeUInt16LE(0x8000, 16 + offset);
  return rom;
}
module.exports.makeNesFixture = makeNesFixture;
