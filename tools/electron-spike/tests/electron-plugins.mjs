import { buildSync } from 'esbuild'
import { fileURLToPath } from 'node:url'
import { app, BrowserWindow } from 'electron'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import assert from 'node:assert/strict'
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-plugin-ui-'))
app.setPath('userData', path.join(temporary, 'profile'))
process.env.ARES_SAVE_DIR = path.join(temporary, 'saves')
delete process.env.ARES_ROM
delete process.env.ARES_AUTOTEST
const template = path.join(temporary, 'tsx')
fs.mkdirSync(template)
const templateRoot = fileURLToPath(
  new URL('../templates/plugin-tsx/', import.meta.url),
)
fs.cpSync(templateRoot, template, { recursive: true })
fs.copyFileSync(
  path.join(templateRoot, 'manifest.json'),
  path.join(template, 'manifest.json'),
)
buildSync({
  entryPoints: [path.join(templateRoot, 'src/plugin.tsx')],
  bundle: true,
  format: 'esm',
  target: 'es2022',
  jsxFactory: 'h',
  outfile: path.join(template, 'dist/plugin.js'),
})
const failing = path.join(temporary, 'failing')
fs.mkdirSync(failing)
const sample = {
  id: 'test.failing',
  name: 'Failing',
  version: '1.0.0',
  apiVersion: 1,
  entry: 'plugin.js',
  capabilities: [],
  contributes: {},
}
fs.writeFileSync(path.join(failing, 'manifest.json'), JSON.stringify(sample))
fs.writeFileSync(
  path.join(failing, 'plugin.js'),
  "export function activate(){throw new Error('actionable activation failure')}",
)
const delayed = path.join(temporary, 'delayed')
fs.mkdirSync(delayed)
fs.writeFileSync(
  path.join(delayed, 'manifest.json'),
  JSON.stringify({ ...sample, id: 'test.delayed', name: 'Delayed' }),
)
fs.writeFileSync(
  path.join(delayed, 'plugin.js'),
  'export async function activate(){window.pluginActivationStarted=true;await new Promise(r=>setTimeout(r,500));return ()=>{window.pluginLateCleanup=true;};}',
)
process.env.ARES_PLUGIN_DIRS = [template, failing, delayed].join(path.delimiter)
process.env.ARES_DEFAULT_PLUGINS = '0'
const { sessions, plugins, stopNative } = await import('../dist/main.js')
const timeout = setTimeout(() => {
  console.error('plugin UI timeout')
  app.exit(1)
}, 30000)
const until = async (fn) => {
  for (let i = 0; i < 200; i++) {
    try {
      const value = await fn()
      if (value) return value
    } catch {}
    await new Promise((r) => setTimeout(r, 25))
  }
  throw new Error('plugin check timed out')
}
app.whenReady().then(async () => {
  try {
    await until(() => BrowserWindow.getAllWindows()[0])
    const window = BrowserWindow.getAllWindows()[0]
    window.webContents.on('console-message', (e) =>
      console.log('RENDERER', e.message),
    )
    const js = (code) => window.webContents.executeJavaScript(code)
    await until(() => sessions.request('session.status'))
    await until(() => js("!!document.querySelector('#plugins')"))
    await sessions.request('session.pause')
    await plugins.enable('ares.examples.memory', true)
    await plugins.enable('ares.examples.kirby', true)
    await until(() =>
      js(
        '!!document.querySelector(\'[data-plugin-command="ares.examples.memory:open"]\')',
      ),
    )
    assert.equal(
      plugins
        .catalog()
        .plugins.find((p) => p.manifest.id === 'ares.examples.kirby').state,
      'waiting',
    )
    assert.equal(
      await js(
        'document.querySelector(\'[data-plugin-panel="ares.examples.memory:memory"]\')?.hidden',
      ),
      false,
    )
    await js(
      'document.querySelector(\'[data-plugin-command="ares.examples.memory:open"]\').click()',
    )
    await until(() =>
      js(
        "document.querySelector('[data-plugin-panel] select').options.length>0",
      ),
    )
    await js("document.getElementById('memory-read').click()")
    await until(() =>
      js(
        "document.querySelector('[data-plugin-panel] pre').textContent.includes('Offset')",
      ),
    )
    const token = plugins
      .catalog()
      .plugins.find((p) => p.manifest.id === 'ares.examples.memory').lease.token
    await plugins.enable('ares.examples.memory', false)
    await until(() =>
      js(
        '!document.querySelector(\'[data-plugin-panel="ares.examples.memory:memory"]\')',
      ),
    )
    await assert.rejects(
      plugins.request(token, 'memory.regions'),
      /stale_plugin_scope/,
    )
    await plugins.enable('ares.examples.memory', true)
    await until(() =>
      js(
        '!!document.querySelector(\'[data-plugin-command="ares.examples.memory:open"]\')',
      ),
    )
    const old = plugins
      .catalog()
      .plugins.find((p) => p.manifest.id === 'ares.examples.memory').lease.token
    await sessions.request('session.reset')
    await assert.rejects(
      plugins.request(old, 'memory.regions'),
      /stale_plugin_scope/,
    )
    await until(() =>
      js(
        '!!document.querySelector(\'[data-plugin-command="local.example:open"]\')',
      ),
    )
    await js(
      'document.querySelector(\'[data-plugin-command="local.example:open"]\').click()',
    )
    await until(() =>
      js(
        "document.querySelector('[data-plugin-panel=\"local.example:hello\"]')?.textContent.includes('Hello from Synthetic')",
      ),
    )
    await until(
      () =>
        plugins.catalog().plugins.find((p) => p.manifest?.id === 'test.failing')
          ?.state === 'error',
    )
    assert.match(
      plugins.catalog().plugins.find((p) => p.manifest?.id === 'test.failing')
        .error,
      /actionable activation failure/,
    )
    await until(() => js('window.pluginActivationStarted'))
    await plugins.enable('test.delayed', false)
    await until(() => js('window.pluginLateCleanup'))
    for (const position of ['left', 'right', 'bottom']) {
      await js(
        `(()=>{const position=document.querySelector('#plugins select');position.value=${JSON.stringify(position)};position.dispatchEvent(new Event('change'));})()`,
      )
      assert.equal(
        await js("document.getElementById('plugin-dock').dataset.position"),
        position,
      )
    }
    const screenshot = await window.webContents.capturePage()
    const artifact = fileURLToPath(
      new URL('../artifacts/plugin-dock.png', import.meta.url),
    )
    fs.mkdirSync(path.dirname(artifact), { recursive: true })
    fs.writeFileSync(artifact, screenshot.toPNG())
    console.log('PLUGIN_UI_PASS')
    clearTimeout(timeout)
    await stopNative()
    app.exit(0)
  } catch (e) {
    console.error(e)
    clearTimeout(timeout)
    await stopNative()
    app.exit(1)
  }
})
app.on('will-quit', () =>
  fs.rmSync(temporary, { recursive: true, force: true }),
)
