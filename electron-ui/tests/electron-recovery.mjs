import { app, BrowserWindow } from 'electron'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { execFile } from 'node:child_process'
import { promisify } from 'node:util'
import assert from 'node:assert/strict'
const run = promisify(execFile)
const temporary = fs.mkdtempSync('/tmp/ares-recovery-ui-')
const profile = path.join(temporary, 'profile')
const source = path.join(temporary, 'wedged')
fs.mkdirSync(path.join(profile, 'plugins', 'storage'), { recursive: true })
fs.mkdirSync(source)
fs.writeFileSync(path.join(source, 'manifest.json'), JSON.stringify({ id: 'test.wedged', name: 'Wedged plugin', version: '1.0.0', apiVersion: 1, entry: 'plugin.js', capabilities: [], contributes: {} }))
fs.writeFileSync(path.join(source, 'plugin.js'), 'while(true){};export function activate(){}')
const configFile = path.join(profile, 'plugins', 'enabled.json')
const config = JSON.stringify({ directories: [source], enabled: ['test.wedged', 'ares.examples.memory', 'test.no-manifest'] })
fs.writeFileSync(configFile, config)
fs.writeFileSync(path.join(profile, 'plugins', 'storage', 'keep.json'), '"preserved"')
app.setPath('userData', profile)
Object.assign(process.env, { ARES_PROFILE_DIR: profile, ARES_SAVE_DIR: path.join(profile, 'saves'), ARES_ROM: '', ARES_AUTOTEST: '', ARES_ENABLE_SERVICES: '1', ARES_SERVICE_DIR: path.join(temporary, 'ipc'), ARES_SILENT: '1' })
assert.ok(process.argv.includes('--disable-plugins'), 'exercise the actual startup flag')
const { sessions, plugins, serviceHost } = await import('../dist/main.js')
const until = async check => {
  for (let i = 0; i < 300; i++) {
    try { const result = await check(); if (result) return result } catch {}
    await new Promise(resolve => setTimeout(resolve, 20))
  }
  throw new Error('recovery check timed out')
}
const deadline = setTimeout(() => app.exit(1), 25000)
app.on('will-quit', () => { clearTimeout(deadline); fs.rmSync(temporary, { recursive: true, force: true }) })
app.whenReady().then(async () => {
  try {
    const first = await until(() => sessions.request('session.status'))
    const window = BrowserWindow.getAllWindows()[0]
    const js = code => window.webContents.executeJavaScript(code)
    await until(() => js("document.querySelector('#plugins [data-recovery]')?.hidden === false"))
    assert.equal(await js("document.querySelector('#plugins').dataset.scopes"), '0')
    assert.equal(fs.readFileSync(configFile, 'utf8'), config, 'safe startup never rewrites selections')
    assert.equal(fs.existsSync(path.join(profile, 'plugins', 'artifacts')), false, 'no plugin code was built')
    assert.ok(plugins.catalog().plugins.every(p => !p.lease && !p.enabled))
    const cli = async (...args) => JSON.parse((await run(process.execPath, [fileURLToPath(new URL('../dist/plugins/cli.js', import.meta.url)), ...args, '--connection', serviceHost.metadataFile, '--instance', serviceHost.instance], { env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '' }, timeout: 8000 })).stdout)
    await assert.rejects(cli('enable', 'test.wedged'), /plugins_disabled_until_restart/)
    await cli('disable', 'test.wedged')
    await cli('disable', 'test.no-manifest')
    assert.deepEqual(JSON.parse(fs.readFileSync(configFile)).enabled, ['ares.examples.memory'])
    await cli('reset-startup')
    assert.deepEqual(JSON.parse(fs.readFileSync(configFile)), { directories: [], enabled: [] })
    assert.equal(fs.readFileSync(path.join(profile, 'plugins', 'storage', 'keep.json'), 'utf8'), '"preserved"')
    await sessions.request('session.pause')
    window.webContents.reload()
    await until(() => js("document.querySelector('#plugins [data-recovery]')?.hidden === false"))
    assert.equal(await js("document.querySelector('#plugins').dataset.scopes"), '0')
    const after = await sessions.request('session.status')
    assert.equal(after.session, first.session)
    assert.equal(after.epoch, first.epoch)
    assert.equal(after.result.paused, true)
    console.log('PLUGIN_RECOVERY_PASS: persisted infinite-loop plugin bypassed, UI responsive, CLI disable/reset, data retained, reload remains safe')
    app.quit()
  } catch (error) { console.error(error); app.exit(1) }
})
