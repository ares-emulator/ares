const { test } = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const os = require('node:os')
const path = require('node:path')
const { spawn, spawnSync } = require('node:child_process')
const { once } = require('node:events')
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../build-electron-make/bin/ares-electron-host')

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
function findSave(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const file = path.join(directory, entry.name)
    if (entry.isDirectory()) { const found = findSave(file); if (found) return found }
    else if (file.endsWith('.ram')) return file
  }
}
test('extracted GBA loader reports missing firmware, loads explicit firmware, samples input and persists isolated SRAM', { timeout: 15000 }, async () => {
  const { NativeSessionClient } = await import('../dist/services/native-client.js')
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-runtime-gba-'))
  try {
    const { settings, rom } = fixtures(temporary)
    const saves = path.join(temporary, 'saves')
    const missing = spawnSync(host, ['--rom', rom, '--system', 'Game Boy Advance', '--settings', path.join(temporary, 'absent.bml'), '--save-dir', saves, '--silent'], { input: 'quit\n', timeout: 5000, encoding: 'utf8' })
    assert.notEqual(missing.status, 0)
    assert.match(missing.stderr, /firmware is missing or invalid[\s\S]*Game Boy Advance.*BIOS.*World/)
    async function run(pressed) {
      const child = spawn(host, ['--rom', rom, '--system', 'Game Boy Advance', '--settings', settings, '--save-dir', saves, '--silent', '--paused', '--control-session', 'gba-loader'], { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] })
      const closed = once(child, 'close')
      child.stdout.resume()
      let log = ''
      child.stderr.on('data', data => { log += data })
      const client = new NativeSessionClient(child.stdin, child.stdio[3], { session: 'gba-loader', epoch: 1 })
      try {
        assert.equal((await client.request('session.status')).result.system, 'Game Boy Advance')
        child.stdin.write(`input 1 ${pressed ? 32 : 0} 0 0 0 0\n`)
        await client.request('session.resume')
        await new Promise(resolve => setTimeout(resolve, 250))
        await client.request('session.pause')
        client.close()
        child.stdin.end('quit\n')
        assert.equal((await closed)[0], 0, log)
      } finally {
        client.close()
        if (child.exitCode === null && child.signalCode === null) { child.kill('SIGKILL'); await closed }
      }
      const file = findSave(saves)
      assert.ok(file, 'save is written under the isolated save root')
      return fs.readFileSync(file)
    }
    const first = await run(true)
    assert.equal(first[0], 0x5a)
    assert.equal(first[1] & 1, 0, 'virtual A is pressed')
    const second = await run(false)
    assert.equal(second[0], 0x5a)
    assert.equal(second[1] & 1, 1, 'virtual A is released')
    assert.equal(second[2], (first[2] + 1) & 255, 'SRAM from the earlier run was loaded')
    assert.equal(fs.existsSync(path.join(temporary, 'foreign-home')), false, 'imported home path cannot redirect writable system data')
    assert.equal(fs.existsSync(path.join(temporary, 'fixture.ram')), false, 'original media directory is untouched')
  } finally { fs.rmSync(temporary, { recursive: true, force: true }) }
})

test('native resource root selects its own database and never falls back to checkout or user databases', { timeout: 10000 }, () => {
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-runtime-resources-'))
  try {
    const rom = path.join(temporary, 'absent-fixture.zip')
    fs.writeFileSync(rom, Buffer.alloc(1))
    const resources = path.join(temporary, 'resources')
    fs.mkdirSync(resources)
    const args = ['--rom', rom, '--system', 'Arcade', '--settings', path.join(temporary, 'empty.bml'), '--save-dir', path.join(temporary, 'saves'), '--resource-dir', resources, '--silent']
    const missing = spawnSync(host, args, { input: 'quit\n', timeout: 3000, encoding: 'utf8' })
    assert.notEqual(missing.status, 0)
    assert.match(missing.stderr, /database file for the system was not found[\s\S]*Arcade.bml and VsSystem.bml/)
    fs.mkdirSync(path.join(resources, 'Database'))
    fs.writeFileSync(path.join(resources, 'Database', 'Arcade.bml'), 'game\n  name: unrelated-original-fixture\n')
    const supplied = spawnSync(host, args, { input: 'quit\n', timeout: 3000, encoding: 'utf8' })
    assert.notEqual(supplied.status, 0)
    assert.match(supplied.stderr, /required manifest for this ROM was not found/)
    assert.doesNotMatch(supplied.stderr, /database file for the system was not found/)
  } finally { fs.rmSync(temporary, { recursive: true, force: true }) }
})
