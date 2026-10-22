const fs = require('node:fs')
const path = require('node:path')

// Original ARM instructions, not game or vendor firmware bytes. The tiny BIOS
// jumps to the cartridge. The cartridge increments a persisted boot byte and
// samples KEYINPUT into SRAM, exercising the actual GBA loader and input path.
function fixtures(directory) {
  const bios = Buffer.alloc(16384)
  bios.writeUInt32LE(0xe51ff004, 0) // ldr pc,[pc,#-4]
  bios.writeUInt32LE(0x08000000, 4)
  const rom = Buffer.alloc(512)
  const instructions = [0xe3a0040e, 0xe5d01002, 0xe2811001, 0xe5c01002,
    0xe3a0105a, 0xe5c01000, 0xe59f3010, 0xe1d320b0, 0xe5c02001, 0xeafffffc]
  instructions.forEach((value, index) => rom.writeUInt32LE(value, index * 4))
  rom.writeUInt32LE(0x04000130, 0x30)
  rom.write('ARES FIXTURE', 0xa0)
  rom.write('SRAM_V113', 0x100)
  fs.writeFileSync(path.join(directory, 'boot.rom'), bios)
  fs.writeFileSync(path.join(directory, 'fixture.gba'), rom)
  const settings = path.join(directory, 'settings.bml')
  fs.writeFileSync(settings, `Paths\n Home: ${path.join(directory, 'foreign-home')}\nGameBoyAdvance\n Firmware\n  BIOS.World: ${path.join(directory, 'boot.rom')}\n`)
  return { settings, rom: path.join(directory, 'fixture.gba') }
}

module.exports = { fixtures }
