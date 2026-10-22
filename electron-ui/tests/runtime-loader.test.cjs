const { test } = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const os = require('node:os')
const path = require('node:path')
const { spawn, spawnSync } = require('node:child_process')
const { once } = require('node:events')
const host = process.env.ARES_HOST || path.resolve(__dirname, '../../build-electron-make/bin/ares-electron-host')

const { fixtures } = require('./fixtures/gba.cjs')
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

test('registry firmware settings and profile overlay enable GBA without rewriting imported BML', { timeout: 10000 }, async () => {
  const { SettingsStore } = await import('../dist/settings/store.js');
  const { firmwareDescriptors, importedValues, parseNativeSettingsInfo, nativeSettingsBml } = await import('../dist/settings/native.js');
  const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-firmware-settings-'));
  try {
    const { settings, rom } = fixtures(temporary);
    const original = fs.readFileSync(settings, 'utf8').replace(path.join(temporary, 'boot.rom'), path.join(temporary, 'missing.rom'));
    fs.writeFileSync(settings, original);
    const result = spawnSync(host, ['--settings-info', '--settings', settings], { encoding: 'utf8', timeout: 3000 });
    assert.equal(result.status, 0, result.stderr);
    const info = parseNativeSettingsInfo(result.stdout);
    const descriptors = firmwareDescriptors(info);
    assert.ok(descriptors.length > 20, 'all compiled systems contribute firmware requirements');
    const id = descriptors.find(d => d.nativeKey === 'GameBoyAdvance/Firmware/BIOS.World').id;
    const file = path.join(temporary, 'profile', 'preferences.json');
    const store = new SettingsStore(file, descriptors);
    store.inherit(importedValues(info, descriptors));
    await store.set(id, path.join(temporary, 'boot.rom'));
    const restarted = new SettingsStore(file, descriptors);
    restarted.inherit(importedValues(info, descriptors));
    const overlay = path.join(temporary, 'profile', 'launch.bml');
    fs.writeFileSync(overlay, nativeSettingsBml(restarted.catalog()));
    const loaded = spawnSync(host, ['--rom', rom, '--system', 'Game Boy Advance', '--settings', settings,
      '--settings-overlay', overlay, '--save-dir', path.join(temporary, 'profile', 'saves'), '--silent'], { input: 'quit\n', encoding: 'utf8', timeout: 4000 });
    assert.equal(loaded.status, 0, loaded.stderr);
    assert.match(loaded.stderr, /ready: Game Boy Advance/);
    assert.equal(fs.readFileSync(settings, 'utf8'), original);
    assert.equal(fs.existsSync(path.join(temporary, 'foreign-home')), false);
    await restarted.reset(id);
    fs.writeFileSync(overlay, nativeSettingsBml(restarted.catalog()));
    const missing = spawnSync(host, ['--rom', rom, '--system', 'Game Boy Advance', '--settings', settings,
      '--settings-overlay', overlay, '--save-dir', path.join(temporary, 'profile', 'saves'), '--silent'], { input: 'quit\n', encoding: 'utf8', timeout: 3000 });
    assert.notEqual(missing.status, 0);
    assert.match(missing.stderr, /firmware is missing or invalid/);
  } finally { fs.rmSync(temporary, { recursive: true, force: true }); }
});
