'use strict'
// The same fixture-only lane runs locally and in CI. No private media is read.
const fs = require('node:fs')
const path = require('node:path')
const os = require('node:os')
const { spawn, spawnSync } = require('node:child_process')
const root = path.resolve(__dirname, '..'), repository = path.dirname(root)
const output = path.join(root, 'artifacts', 'acceptance')
fs.mkdirSync(output, { recursive: true })
const runDirectory = fs.mkdtempSync(path.join(output, 'run-'))
const summary = { result: 'running', started: new Date().toISOString(), platform: process.platform, arch: process.arch, node: process.version, runDirectory, gates: [], excluded: ['private-ROM identity/Kirby checks', 'cross-binary reference checks', 'physical input and audible output', 'serializer rollback-failure fault injection', 'soak/latency and other platforms'] }
const summaryFile = path.join(output, 'latest.json')
const record = () => {
  fs.writeFileSync(path.join(runDirectory, 'summary.json'), JSON.stringify(summary, null, 2) + '\n')
  fs.writeFileSync(summaryFile, JSON.stringify(summary, null, 2) + '\n')
}
const env = { ...process.env }
// Do not inherit a caller's live profile, connection, media or instrumentation.
for (const key of Object.keys(env)) if (key.startsWith('ARES_')) delete env[key]
env.ARES_BACKGROUND_TEST = '1'
env.ARES_SILENT = '1'
env.NODE_OPTIONS = ''
env.NODE_PATH = ''
delete env.ELECTRON_RUN_AS_NODE
let active, interrupted = false
function stopGroup(child, signal) {
  if (!child?.pid) return
  try { process.kill(-child.pid, signal) } catch (error) { if (error.code !== 'ESRCH') throw error }
}
for (const signal of ['SIGINT', 'SIGTERM']) process.on(signal, () => {
  interrupted = true
  stopGroup(active, signal)
  process.exitCode = 1
})
async function gate(name, command, args, cwd = root, timeoutMs = 180000) {
  if (interrupted) throw new Error('Acceptance interrupted')
  console.log(`ACCEPTANCE: ${name}`)
  const log = path.join(runDirectory, `${name}.log`), fd = fs.openSync(log, 'w')
  const result = { name, status: 'running', log, started: new Date().toISOString() }
  summary.gates.push(result); record()
  try {
    await new Promise((resolve, reject) => {
      const child = active = spawn(command, args, { cwd, env, detached: true, stdio: ['ignore', fd, fd] })
      const timer = setTimeout(() => stopGroup(child, 'SIGTERM'), timeoutMs)
      const force = setTimeout(() => stopGroup(child, 'SIGKILL'), timeoutMs + 5000)
      child.once('error', reject)
      child.once('close', (code, signal) => {
        clearTimeout(timer); clearTimeout(force); active = undefined
        if (code === 0) resolve()
        else reject(new Error(`${name}: exit ${code ?? signal}; see ${log}`))
      })
    })
    result.status = 'passed'
  } catch (error) {
    result.status = 'failed'
    throw error
  } finally {
    fs.closeSync(fd); result.finished = new Date().toISOString(); record()
  }
  return log
}
function executable(command) {
  const found = spawnSync('/usr/bin/which', [command], { encoding: 'utf8' })
  if (found.status !== 0) throw new Error(`Missing ${command}; see README prerequisites`)
  return found.stdout.trim()
}
async function main() {
  if (process.platform !== 'darwin' || process.arch !== 'arm64') throw new Error('Acceptance is currently supported on macOS arm64')
  const major = Number(process.versions.node.split('.')[0])
  if (major < 24 || major >= 27) throw new Error('Use Node 24–26; CI pins the version in .node-version')
  const cmake = executable('cmake')
  const version = spawnSync(cmake, ['--version'], { encoding: 'utf8' }).stdout.match(/version (\d+)\.(\d+)/)
  if (!version || Number(version[1]) < 3 || (Number(version[1]) === 3 && Number(version[2]) < 28)) throw new Error('CMake 3.28 or newer is required')
  env.ARES_FFPROBE = process.env.ARES_FFPROBE ? path.resolve(process.env.ARES_FFPROBE) : executable('ffprobe')
  fs.accessSync(env.ARES_FFPROBE, fs.constants.X_OK)
  fs.accessSync(path.join(path.dirname(env.ARES_FFPROBE), 'ffmpeg'), fs.constants.X_OK)
  const electron = require('electron')
  fs.accessSync(electron, fs.constants.X_OK)
  const build = path.resolve(repository, process.env.ARES_ACCEPT_BUILD_DIR || 'build-electron-acceptance')
  const jobs = Number(process.env.ARES_ACCEPT_JOBS || Math.min(8, os.availableParallelism()))
  if (!Number.isInteger(jobs) || jobs < 1 || jobs > 16) throw new Error('ARES_ACCEPT_JOBS must be 1–16')
  env.ARES_HOST = path.join(build, 'bin', 'ares-electron-host')
  env.ARES_CPU_FIXTURE = path.join(build, 'bin', 'ares-electron-cpu-fixture')
  summary.nativeBuild = build
  summary.revision = {
    commit: spawnSync('git', ['rev-parse', 'HEAD'], { cwd: repository, encoding: 'utf8' }).stdout.trim(),
    dirty: !!spawnSync('git', ['status', '--porcelain'], { cwd: repository, encoding: 'utf8' }).stdout.trim(),
  }
  summary.tools = { cmake: version[0], electron: require('electron/package.json').version, ffprobe: env.ARES_FFPROBE }
  await gate('configure', cmake, ['-S', repository, '-B', build, '-G', 'Unix Makefiles', '-UARES_CORES', '-DARES_BUILD_ELECTRON_UI=ON', '-DARES_BUILD_OPTIONAL_TARGETS=OFF', '-DARES_ENABLE_LIBRASHADER=OFF', '-DENABLE_CCACHE=OFF', '-DENABLE_IPO=OFF', '-DCMAKE_BUILD_TYPE=RelWithDebInfo'], repository, 300000)
  await gate('native-build', cmake, ['--build', build, '--target', 'ares-electron-host', 'ares-electron-cpu-fixture', '-j', String(jobs)], repository, 1800000)
  await gate('typescript', 'npm', ['run', 'build'])
  const tests = fs.readdirSync(path.join(root, 'tests')).filter(name => name.endsWith('.test.cjs') && name !== 'plugins-game-live.test.cjs').sort().map(name => `tests/${name}`)
  const unitLog = await gate('fixture-regressions', process.execPath, ['--test', '--test-concurrency=1', '--test-reporter=tap', 'protocol.test.cjs', ...tests])
  const tap = fs.readFileSync(unitLog, 'utf8')
  if (!/^# fail 0$/m.test(tap) || !/^# skipped 0$/m.test(tap)) throw new Error('Fixture lane must report zero failed and skipped tests')
  summary.fixtureTests = Number(tap.match(/^# tests (\d+)$/m)?.[1])
  if (!Number.isInteger(summary.fixtureTests) || summary.fixtureTests < 1) throw new Error('Missing fixture test count')
  await gate('native-smoke', process.execPath, ['smoke.cjs'])
  for (const [name, file, args] of [
    ['electron-session', 'electron-session.mjs', []],
    ['electron-mcp', 'electron-mcp.mjs', []],
    ['electron-plugins', 'electron-plugins.mjs', []],
    ['electron-reload', 'electron-reload.mjs', []],
    ['electron-plugin-tools', 'electron-plugin-tools.mjs', []],
    ['electron-settings', 'electron-settings.mjs', []],
    ['electron-recovery', 'electron-recovery.mjs', ['--disable-plugins']],
  ]) await gate(name, electron, [`tests/${file}`, ...args])
  await gate('profile-ownership', process.execPath, ['tests/profile-live.cjs'])
  await gate('launch-isolation', process.execPath, ['tests/launch-live.cjs'])
  const bundle = path.join(runDirectory, 'Ares.app')
  try {
    await gate('package', process.execPath, ['scripts/package-macos.cjs', bundle])
    const packageStarted = Date.now()
    await gate('copied-package', process.execPath, ['tests/package-live.cjs', bundle])
    if (fs.statSync(path.join(root, 'artifacts/package-mcp.json')).mtimeMs < packageStarted) throw new Error('Copied-package evidence was not refreshed')
    summary.package = JSON.parse(fs.readFileSync(path.join(root, 'artifacts/package-mcp.json'), 'utf8'))
    if (summary.package.result !== 'PACKAGE_MCP_PASS') throw new Error('Missing copied-package acceptance evidence')
  } finally { fs.rmSync(bundle, { recursive: true, force: true }) }
  summary.result = 'passed'
  console.log(`ACCEPTANCE_PASS: ${summary.gates.length} gates; ${summary.fixtureTests} fixture tests; ${summaryFile}`)
}
record()
main().catch(error => { summary.result = 'failed'; summary.error = String(error); console.error(error); process.exitCode = 1 }).finally(() => { summary.finished = new Date().toISOString(); record() })
