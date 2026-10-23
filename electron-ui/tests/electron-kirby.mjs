import { app, BrowserWindow } from 'electron'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { createHash } from 'node:crypto'
import assert from 'node:assert/strict'
import { LocalServiceClient } from '../dist/services/local-endpoint.js'
const original = process.env.ARES_PLUGIN_TEST_ROM
if (!original)
  throw new Error('Set ARES_PLUGIN_TEST_ROM to the verified Kirby ROM')
const temporary = fs.mkdtempSync('/tmp/ares-kirby-ui-'),
  hash = (b) => createHash('sha256').update(b).digest('hex'),
  sourceHash = hash(fs.readFileSync(original))
const rom = path.join(temporary, 'renamed.z64')
fs.copyFileSync(original, rom)
fs.writeFileSync(path.join(temporary, 'settings.bml'), '')
app.setPath('userData', path.join(temporary, 'profile'))
Object.assign(process.env, {
  ARES_ROM: rom,
  ARES_SYSTEM: 'Nintendo 64',
  ARES_SILENT: '1',
  ARES_SETTINGS: path.join(temporary, 'settings.bml'),
  ARES_SAVE_DIR: path.join(temporary, 'profile', 'saves'),
  ARES_ENABLE_SERVICES: '1',
  ARES_SERVICE_DIR: path.join(temporary, 'ipc'),
})
delete process.env.ARES_AUTOTEST
delete process.env.ARES_DEFAULT_PLUGINS
const { sessions, plugins, serviceHost, stopNative } =
  await import('../dist/main.js')
const until = async (fn, label) => {
  for (let i = 0; i < 300; i++) {
    try {
      const v = await fn()
      if (v) return v
    } catch {}
    await new Promise((r) => setTimeout(r, 25))
  }
  throw new Error('Timed out: ' + label)
}
const timer = setTimeout(() => app.exit(1), 90000)
let client
app.whenReady().then(async () => {
  try {
    const status = await until(() => sessions.request('session.status'), 'host')
    await sessions.request('session.pause')
    const window = BrowserWindow.getAllWindows()[0],
      js = (s) => window.webContents.executeJavaScript(s)
    client = new LocalServiceClient(
      serviceHost.metadataFile,
      serviceHost.instance,
    )
    const boot = await sessions.request('execution.runFrames', undefined, {
      count: 120,
      timeoutMs: 15000,
    })
    assert.equal(boot.ok, true, JSON.stringify(boot))
    await until(async () => {
      const op = await sessions.request('execution.status', undefined, {
        operationId: boot.result.operation_id,
      })
      return op.result.state === 'done'
    }, 'boot/RDRAM initialization')
    await until(
      () =>
        plugins
          .catalog()
          .plugins.find((p) => p.manifest?.id === 'ares.examples.kirby')
          ?.phase === 'active',
      'Kirby activation',
    )
    let identity = { session: status.session, epoch: status.epoch }
    // Optional original saved state is copied and loaded into this disposable host only.
    if (process.env.ARES_PLUGIN_TEST_STATE) {
      const previousToken = plugins.catalog().plugins.find(
        (p) => p.manifest?.id === 'ares.examples.kirby',
      )?.lease?.token
      const copy = path.join(temporary, 'gameplay.bs')
      fs.copyFileSync(process.env.ARES_PLUGIN_TEST_STATE, copy)
      const loaded = await sessions.request('state.load', undefined, {
        path: copy,
        resume: false,
        keepOverrides: false,
      })
      assert.equal(loaded.ok, true, JSON.stringify(loaded))
      identity = { session: loaded.session, epoch: loaded.epoch }
      await until(
        () => {
          const plugin = plugins.catalog().plugins.find(
            (p) => p.manifest?.id === 'ares.examples.kirby',
          )
          return plugin?.phase === 'active' && plugin.lease.token !== previousToken
        },
        'post-load plugin',
      )
    }
    const info = () =>
      plugins
        .catalog()
        .plugins.find((p) => p.manifest?.id === 'ares.examples.kirby')
    await client.manage({
      operation: 'open',
      id: info().manifest.id,
      token: info().lease.token,
      name: 'powers',
    })
    await until(
      () => js("document.querySelectorAll('[data-power-id]').length===36"),
      '36 power buttons',
    )
    await until(
      () =>
        js(
          "Array.from(document.querySelectorAll('.kirby-powers img')).every(img=>img.complete&&img.naturalWidth>0)",
        ),
      'all icons loaded',
    )
    const { POWERS, ADDRESSES } =
      await import('../bundled-plugins/kirby/powers.js')
    const read = async (address) => {
      const r = await sessions.request('n64.memory.read', identity, {
        address,
        view: 'cpu',
        length: 4,
      })
      assert.equal(r.ok, true, JSON.stringify(r))
      return r.result.hex
    }
    const addresses = Object.values(ADDRESSES),
      before = await Promise.all(addresses.map(read))
    const initial = await sessions.request('session.status')
    for (const power of POWERS) {
      await js(
        `document.querySelector('[data-power-id="${power.id}"]').click()`,
      )
      await until(
        () =>
          js(
            `document.querySelector('.kirby-powers [role="status"]').textContent===${JSON.stringify('Selected ' + power.name + '.')}`,
          ),
        'power ' + power.id,
      )
      assert.equal(
        await read(ADDRESSES.power),
        power.id.toString(16).padStart(8, '0'),
      )
      assert.equal(
        await read(ADDRESSES.hud),
        power.id.toString(16).padStart(8, '0'),
      )
      assert.notEqual(
        await read(ADDRESSES.lastShown),
        power.id.toString(16).padStart(8, '0'),
      )
    }
    const after = await sessions.request('session.status')
    assert.equal(after.result.paused, true)
    assert.equal(after.result.frame, initial.result.frame)
    assert.equal(after.session, initial.session)
    assert.equal(after.epoch, initial.epoch)
    // Reload/open and management commands use the same explicit handlers; they never reapply a power.
    const words = await Promise.all(addresses.map(read)),
      token = info().lease.token
    await client.manage({ operation: 'reload', id: info().manifest.id })
    await until(
      () => info().phase === 'active' && info().lease.token !== token,
      'reload',
    )
    assert.deepEqual(await Promise.all(addresses.map(read)), words)
    await assert.rejects(
      client.manage({
        operation: 'command',
        id: info().manifest.id,
        token,
        name: 'power-14',
      }),
      /stale_plugin_scope/,
    )
    await client.manage({
      operation: 'command',
      id: info().manifest.id,
      token: info().lease.token,
      name: 'power-14',
    })
    assert.equal(await read(ADDRESSES.power), '0000000e')
    await until(
      () =>
        js(
          "document.querySelector('[data-power-id=\"14\"]').getAttribute('aria-pressed')==='true'",
        ),
      'selected button refresh',
    )
    const root = fileURLToPath(new URL('../artifacts/', import.meta.url))
    fs.mkdirSync(root, { recursive: true })
    fs.writeFileSync(
      path.join(root, 'kirby-powers.png'),
      (await window.webContents.capturePage()).toPNG(),
    )
    const restore = await sessions.request('n64.memory.writeMany', identity, {
      writes: addresses.map((address, i) => ({
        address,
        view: 'cpu',
        hex: before[i],
      })),
      verifyAfterFrames: 0,
      timeoutMs: 1000,
    })
    assert.equal(restore.ok, true)
    assert.equal(hash(fs.readFileSync(original)), sourceHash)
    console.log(
      'KIRBY_POWERS_PASS: 36 buttons, icons, CPU-view writes/readback, paused/frame/session retention, no reload replay, stale scopes and command API',
    )
    clearTimeout(timer)
    client.close()
    await plugins.close()
    await stopNative()
    app.exit(0)
  } catch (error) {
    console.error(error)
    const w = BrowserWindow.getAllWindows()[0]
    if (w)
      console.error(
        await w.webContents
          .executeJavaScript(
            "({status:document.querySelector('.kirby-powers [role=status]')?.textContent,errors:Array.from(document.querySelectorAll('#plugins pre')).map(p=>p.textContent)})",
          )
          .catch(() => null),
      )
    clearTimeout(timer)
    client?.close()
    await plugins.close()
    await stopNative()
    app.exit(1)
  }
})
app.on('will-quit', () =>
  fs.rmSync(temporary, { recursive: true, force: true }),
)
