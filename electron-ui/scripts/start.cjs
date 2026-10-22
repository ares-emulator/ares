'use strict'
// Each launch owns its compiled JS, bundled plugins and native executable.
// Tests/package builds can clean dist and rebuild C++ while this app is running.
const fs = require('node:fs')
const path = require('node:path')
const { spawn, execFileSync } = require('node:child_process')
const root = path.resolve(__dirname, '..')
const launches = path.join(root, 'artifacts', 'runs')
fs.mkdirSync(launches, { recursive: true })
const snapshot = fs.mkdtempSync(path.join(launches, 'launch-'))
const copy = (from, to) => fs.cpSync(from, to, { recursive: true, mode: fs.constants.COPYFILE_FICLONE })
let child
try {
  for (const entry of fs.readdirSync(root)) {
    if (/\.(?:ts|cts|json|html|css)$/.test(entry) || ['plugins', 'services', 'settings', 'workspace', 'scripts', 'mcp', 'bundled-plugins', 'templates'].includes(entry))
      copy(path.join(root, entry), path.join(snapshot, entry))
  }
  fs.symlinkSync(path.join(root, 'node_modules'), path.join(snapshot, 'node_modules'), 'dir')
  execFileSync('npm', ['run', 'build'], { cwd: snapshot, stdio: 'inherit' })
  const sourceHost = path.resolve(process.env.ARES_HOST || path.join(root, '../build-electron-make/bin/ares-electron-host'))
  const native = path.join(snapshot, 'native')
  fs.mkdirSync(path.join(native, 'bin'), { recursive: true })
  const host = path.join(native, 'bin', 'ares-electron-host')
  copy(sourceHost, host)
  // Match both nall CLI resource lookup and executable-adjacent lookup.
  for (const destination of [path.join(native, 'Database'), path.join(native, 'bin', 'Database')])
    copy(path.join(process.env.ARES_RESOURCE_DIR || path.join(root, '../mia'), 'Database'), destination)
  const env = { ...process.env, ARES_HOST: host, ARES_PROFILE: process.env.ARES_PROFILE || 'development', ARES_RESOURCE_DIR: path.join(native, 'bin') }
  for (const key of ['ARES_ROM', 'ARES_SETTINGS', 'ARES_SAVE_DIR', 'ARES_REPORT', 'ARES_SERVICE_DIR', 'ARES_PROFILE_DIR'])
    if (env[key]) env[key] = path.resolve(env[key])
  if (env.ARES_PLUGIN_DIRS)
    env.ARES_PLUGIN_DIRS = env.ARES_PLUGIN_DIRS.split(path.delimiter).map(directory => path.resolve(directory)).join(path.delimiter)
  console.error(`Development runtime: ${snapshot}`)
  child = spawn(require('electron'), [snapshot, ...process.argv.slice(2)], { cwd: root, env, stdio: 'inherit' })
  for (const signal of ['SIGINT', 'SIGTERM']) process.on(signal, () => child.kill(signal))
  child.on('error', error => { console.error(error); process.exitCode = 1 })
  child.on('close', (code, signal) => {
    fs.rmSync(snapshot, { recursive: true, force: true })
    process.exitCode = code ?? (signal ? 1 : 0)
  })
} catch (error) {
  fs.rmSync(snapshot, { recursive: true, force: true })
  console.error(error)
  process.exitCode = 1
}
