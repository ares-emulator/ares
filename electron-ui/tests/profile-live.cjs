const assert = require('node:assert/strict')
const fs = require('node:fs')
const os = require('node:os')
const path = require('node:path')
const { spawn } = require('node:child_process')
const { once } = require('node:events')
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-profile-lock-'))
const children = []
function launch(name) {
  const child = spawn(require('electron'), [path.join(__dirname, 'electron-profile.mjs')], {
    env: { ...process.env, ARES_PROFILE_DIR: path.join(temporary, name), ARES_SAVE_DIR: '', ARES_ROM: '', ARES_BACKGROUND_TEST: '1', ARES_DEFAULT_PLUGINS: '0', ARES_SILENT: '1' },
    stdio: ['ignore', 'pipe', 'pipe'],
  })
  child.output = ''
  for (const stream of [child.stdout, child.stderr]) stream.on('data', data => { child.output += data })
  child.closed = once(child, 'close')
  children.push(child)
  return child
}
async function ready(child) {
  for (let i = 0; i < 500; i++) {
    if (child.output.includes('PROFILE_READY')) return
    if (child.exitCode !== null) throw new Error(child.output)
    await new Promise(resolve => setTimeout(resolve, 20))
  }
  throw new Error(`profile startup timeout: ${child.output}`)
}
async function run() {
  const first = launch('shared')
  await ready(first)
  const duplicate = launch('shared')
  const [code] = await duplicate.closed
  assert.equal(code, 1, duplicate.output)
  assert.match(duplicate.output, /profile_in_use/)
  fs.symlinkSync(path.join(temporary, 'shared'), path.join(temporary, 'alias'))
  const alias = launch('alias')
  assert.equal((await alias.closed)[0], 1, alias.output)
  assert.match(alias.output, /profile_in_use/)
  const independent = launch('independent')
  await ready(independent)
  assert.equal(first.exitCode, null)
  first.kill('SIGKILL')
  await first.closed
  const recovered = launch('shared')
  await ready(recovered)
  recovered.kill('SIGTERM')
  independent.kill('SIGTERM')
  await Promise.all([recovered.closed, independent.closed])
  console.log('PROFILE_OWNERSHIP_PASS: duplicate/alias refused, independent concurrent profile, crash recovery')
}
const deadline = setTimeout(() => { for (const child of children) child.kill('SIGKILL') }, 30000)
run().catch(error => { console.error(error); process.exitCode = 1 }).finally(async () => {
  clearTimeout(deadline)
  for (const child of children) if (child.exitCode === null && child.signalCode === null) child.kill('SIGKILL')
  await Promise.all(children.map(child => child.closed))
  fs.rmSync(temporary, { recursive: true, force: true })
})
