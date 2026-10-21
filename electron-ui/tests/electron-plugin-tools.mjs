import { app, BrowserWindow } from 'electron'
import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { execFile } from 'node:child_process'
import { promisify } from 'node:util'
import assert from 'node:assert/strict'
import { StdioPeer } from './stdio-peer.cjs'
import { LocalServiceClient } from '../dist/services/local-endpoint.js'
const run = promisify(execFile),
  temporary = fs.mkdtempSync('/tmp/ares-tools-'),
  rom = path.join(temporary, 'fixture.z64')
const bytes = Buffer.alloc(0x1000)
bytes.writeUInt32BE(0x80371240)
bytes.write('PLUGIN TOOLS', 0x20)
bytes[0x3e] = 0x45
fs.writeFileSync(rom, bytes)
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
let peer, client
const timer = setTimeout(() => app.exit(1), 90000)
const until = async (fn, label) => {
  for (let i = 0; i < 300; i++) {
    try {
      const v = await fn()
      if (v) return v
    } catch {}
    await new Promise((r) => setTimeout(r, 25))
  }
  throw new Error('Timed out ' + label)
}
app.whenReady().then(async () => {
  try {
    await until(() => sessions.request('session.status'), 'startup')
    await sessions.request('session.pause')
    const window = BrowserWindow.getAllWindows()[0],
      js = (code) => window.webContents.executeJavaScript(code)
    client = new LocalServiceClient(
      serviceHost.metadataFile,
      serviceHost.instance,
    )
    peer = new StdioPeer(
      process.execPath,
      [
        fileURLToPath(new URL('../dist/mcp/server.js', import.meta.url)),
        '--connection',
        serviceHost.metadataFile,
        '--instance',
        serviceHost.instance,
      ],
      { env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '' } },
    )
    await peer.initialize()
    // Original CPU instruction fixture, executed from DMEM into a watched RDRAM word.
    await peer.tool('write_memory', {
      region: 'RSP DMEM',
      addr: 0x80,
      hex_bytes: '3c0880003508200024090007ad0900001000ffff00000000',
    })
    const changed = await peer.tool('cpu_set_registers', { pc: '0xa4000080' })
    assert.equal(changed.error, undefined, JSON.stringify(changed))
    const watch = await peer.tool('watch_add', {
      addr: '0x80002000',
      kind: 'write',
      length: 4,
    })
    assert.equal(typeof watch.id, 'number')
    const stepped = await peer.tool('step_instructions', { n: 1000 })
    assert.equal(stepped.stop_reason, 'watchpoint')
    const nativeStop = (await sessions.request('n64.debug.lastStop')).result
    const mcpStop = await peer.tool('last_stop')
    assert.equal(mcpStop.hit.id, nativeStop.hit.id)
    assert.equal(mcpStop.hit.value, nativeStop.hit.value)
    const cli = fileURLToPath(
      new URL('../dist/plugins/cli.js', import.meta.url),
    )
    const command = async (...args) => {
      const { stdout } = await run(
        process.execPath,
        [
          cli,
          ...args,
          '--connection',
          serviceHost.metadataFile,
          '--instance',
          serviceHost.instance,
        ],
        {
          env: { ...process.env, ELECTRON_RUN_AS_NODE: '1', NODE_OPTIONS: '' },
          timeout: 20000,
        },
      )
      return JSON.parse(stdout)
    }
    await until(
      () =>
        plugins
          .catalog()
          .plugins.find((p) => p.manifest?.id === 'ares.tools.debugger')
          ?.phase === 'active',
      'debug plugin',
    )
    await command('open', 'ares.tools.debugger', 'stop')
    await until(
      () =>
        js(
          "document.querySelector('[data-debug-stop]')?.textContent.includes('watchpoint')",
        ),
      'shared frozen stop',
    )
    const panelStop = await js(
      "JSON.parse(document.querySelector('[data-debug-stop]').textContent)",
    )
    assert.deepEqual(panelStop, nativeStop)
    assert.equal(panelStop.registers.pc, nativeStop.registers.pc)
    const initial = await sessions.request('session.status'),
      source = path.join(temporary, 'agent-tool')
    await command('scaffold', source, '--id', 'local.agent-tool')
    const built = await command('build', source)
    assert.match(built.artifact, /^[a-f0-9]{64}$/)
    await command('add', source)
    await command('enable', 'local.agent-tool')
    await until(
      () =>
        plugins
          .catalog()
          .plugins.find((p) => p.manifest?.id === 'local.agent-tool')?.phase ===
        'active',
      'agent activation',
    )
    await command('open', 'local.agent-tool', 'hello')
    const before = plugins
      .catalog()
      .plugins.find((p) => p.manifest?.id === 'local.agent-tool').lease.token
    const entry = path.join(source, 'src/plugin.tsx')
    fs.writeFileSync(
      entry,
      fs
        .readFileSync(entry, 'utf8')
        .replace('Hello from', 'Agent-edited tool on'),
    )
    await command('reload', 'local.agent-tool')
    const fresh = plugins
      .catalog()
      .plugins.find((p) => p.manifest?.id === 'local.agent-tool')
    assert.notEqual(fresh.lease.token, before)
    await until(
      () =>
        js(
          "document.querySelector('[data-plugin-panel=\"local.agent-tool:hello\"]')?.textContent.includes('Agent-edited tool on Nintendo 64')",
        ),
      'hot tool',
    )
    assert.equal(
      (await sessions.request('session.status')).session,
      initial.session,
    )
    assert.equal(
      (await sessions.request('session.status')).epoch,
      initial.epoch,
    )
    assert.deepEqual(
      (await sessions.request('n64.debug.lastStop')).result,
      nativeStop,
    )
    // Existing native owner exclusion still applies through the plugin's expanded memory API.
    await peer.tool('watch_remove', { id: watch.id })
    const long = peer.tool('run_frames', { n: 1000000, max_seconds: 10 })
    const operation = await until(
      async () =>
        (await sessions.request('session.status')).result.operation_id,
      'external run',
    )
    const memory = plugins
      .catalog()
      .plugins.find((p) => p.manifest?.id === 'ares.examples.memory')
    const busy = await plugins.request(
      memory.lease.token,
      'n64.memory.writeMany',
      {
        writes: [{ address: '0x80002000', view: 'cpu', hex: '00000001' }],
        verifyAfterFrames: 0,
        timeoutMs: 1000,
      },
    )
    assert.equal(busy.error, 'busy')
    assert.equal((await peer.tool('run_frames', { n: 1 })).error, 'busy')
    await js("document.getElementById('pause').click()")
    assert.equal((await long).stop_reason, 'paused')
    await assert.rejects(
      client.manage({
        operation: 'command',
        id: 'local.agent-tool',
        token: before,
        name: 'open',
      }),
      /stale_plugin_scope/,
    )
    console.log(
      'PLUGIN_TOOLS_PASS: CLI scaffold/build/add/enable/open/reload, shared immutable MCP watch hit, same loaded game, busy exclusion and human pause',
    )
    clearTimeout(timer)
    client.close()
    await peer.close()
    await plugins.close()
    await stopNative()
    app.exit(0)
  } catch (error) {
    console.error(error)
    clearTimeout(timer)
    client?.close()
    if (peer) await peer.close()
    await plugins.close()
    await stopNative()
    app.exit(1)
  }
})
app.on('will-quit', () =>
  fs.rmSync(temporary, { recursive: true, force: true }),
)
