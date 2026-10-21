const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
const { spawn, execFileSync } = require('node:child_process')
const { once } = require('node:events')
const { StdioPeer } = require('./stdio-peer.cjs')
const root = path.resolve(__dirname, '..')
const temporary = fs.mkdtempSync('/tmp/ares-launch-')
const report = path.join(temporary, 'renderer.json')
const child = spawn(process.execPath, [path.join(root, 'scripts/start.cjs')], {
  cwd: root,
  env: { ...process.env, ARES_PROFILE_DIR: path.join(temporary, 'profile'), ARES_SAVE_DIR: '', ARES_ROM: '', ARES_BACKGROUND_TEST: '1', ARES_DEFAULT_PLUGINS: '0', ARES_SILENT: '1', ARES_ENABLE_SERVICES: '1', ARES_SERVICE_DIR: path.join(temporary, 'ipc'), ARES_AUTOTEST: '1', ARES_TEST_SECONDS: '12', ARES_REPORT: report },
  stdio: ['ignore', 'pipe', 'pipe'],
})
let diagnostics = ''
for (const stream of [child.stdout, child.stderr]) stream.on('data', data => { diagnostics += data })
const closed = once(child, 'close')
const deadline = setTimeout(() => child.kill('SIGTERM'), 30000)
let peer
async function run() {
  let connection
  for (let i = 0; i < 500; i++) {
    if (fs.existsSync(path.join(temporary, 'ipc')))
      connection = fs.readdirSync(path.join(temporary, 'ipc')).find(name => name.endsWith('.json'))
    if (connection) break
    if (child.exitCode !== null) throw new Error(diagnostics)
    await new Promise(resolve => setTimeout(resolve, 20))
  }
  assert.ok(connection, diagnostics)
  const snapshot = diagnostics.match(/Development runtime: (.+)/)[1].trim()
  const entry = path.join(snapshot, 'dist/main.js')
  const before = fs.statSync(entry)
  const host = path.join(snapshot, 'native/bin/ares-electron-host')
  const sourceHost = process.env.ARES_HOST || path.join(root, '../build-electron-make/bin/ares-electron-host')
  assert.notEqual(fs.statSync(host).ino, fs.statSync(sourceHost).ino)
  const file = path.join(temporary, 'ipc', connection)
  const { instance } = JSON.parse(fs.readFileSync(file))
  peer = new StdioPeer(process.execPath, [path.join(snapshot, 'dist/mcp/server.js'), '--connection', file, '--instance', instance])
  await peer.initialize()
  let initial
  for (let i = 0; i < 100; i++) {
    initial = await peer.tool('emu_status')
    if (!initial.isError && initial.session) break
    await new Promise(resolve => setTimeout(resolve, 20))
  }
  assert.ok(initial.session, JSON.stringify(initial))
  execFileSync('npm', ['run', 'build'], { cwd: root, stdio: 'pipe' })
  assert.equal(fs.statSync(entry).ino, before.ino)
  assert.equal(fs.statSync(entry).mtimeMs, before.mtimeMs)
  const after = await peer.tool('emu_status')
  assert.equal(after.session, initial.session)
  assert.equal(after.epoch, initial.epoch)
  await peer.close()
  peer = null
  const [code] = await closed
  assert.equal(code, 0, diagnostics)
  assert.equal(fs.existsSync(snapshot), false, 'snapshot cleaned after shutdown')
  assert.ok(JSON.parse(fs.readFileSync(report)).frames > 0)
  console.log('ISOLATED_LAUNCH_PASS: copied host and UI survive checkout rebuild, same MCP session/epoch, shutdown cleanup')
}
run().catch(error => { console.error(error); console.error(diagnostics); process.exitCode = 1 }).finally(async () => {
  clearTimeout(deadline)
  if (peer) await peer.close()
  if (child.exitCode === null && child.signalCode === null) child.kill('SIGTERM')
  await closed
  fs.rmSync(temporary, { recursive: true, force: true })
})
