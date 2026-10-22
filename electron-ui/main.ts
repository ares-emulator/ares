import { firmwareDescriptors, importedValues, nativeSettingsBml, parseNativeSettingsInfo } from './settings/native.js'
import { SettingsStore } from './settings/store.js'
import { presentationSettings } from './settings/builtins.js'
import { profileRoot, profileSaves } from './profile.js'
import { PluginRegistry } from './plugins/registry.js'
import {
  app,
  BrowserWindow,
  ipcMain,
  dialog,
  MessageChannelMain,
  protocol,
  type MessagePortMain,
} from 'electron'
import { spawn, execFile, type ChildProcessByStdio } from 'node:child_process'
import type { Readable, Writable } from 'node:stream'
import path from 'node:path'
import fs from 'node:fs'
import { fileURLToPath } from 'node:url'
import { performance } from 'node:perf_hooks'
import { FrameParser, type NativeFrame } from './protocol.js'
import { randomUUID } from 'node:crypto'
import { NativeSessionClient } from './services/native-client.js'
import { LocalServiceHost } from './services/local-endpoint.js'
import { SessionRouter } from './services/session-router.js'
import type { NativeCommand, StartOptions } from './bridge.js'
import type {
  SessionIdentity,
  ServiceMethod,
  ServiceParams,
} from './services/contracts.js'
// Compiled entry lives in dist/ in both checkout and copied distribution.
const appRoot = fileURLToPath(new URL('../', import.meta.url))
const saveDirectory = (() => {
  try {
    // Respect explicit Electron profiles used by embedders/tests and --user-data-dir.
    const inferredProfile = path.join(app.getPath('appData'), app.getName())
    const suppliedProfile = app.getPath('userData')
    const selectedProfile = process.env.ARES_PROFILE_DIR ||
      (app.commandLine.hasSwitch('user-data-dir') || suppliedProfile !== inferredProfile
        ? suppliedProfile
        : profileRoot(app.getPath('appData'), process.env.ARES_PROFILE))
    fs.mkdirSync(selectedProfile, { recursive: true })
    const profileDirectory = fs.realpathSync(selectedProfile)
    app.setName('Ares Workbench')
    app.setPath('userData', profileDirectory)
    app.setPath('sessionData', path.join(profileDirectory, 'browser'))
    // Electron owns the OS-backed lock and releases it even after a crash. Never
    // forward a second launch's arguments or mutations to the existing instance.
    if (!app.requestSingleInstanceLock()) {
      console.error(`profile_in_use: ${profileDirectory}; choose ARES_PROFILE or ARES_PROFILE_DIR`)
      app.exit(1)
    }
    fs.mkdirSync(app.getPath('sessionData'), { recursive: true })
    app.setAppLogsPath(path.join(profileDirectory, 'logs'))
    return profileSaves(profileDirectory, process.env.ARES_SAVE_DIR)
  } catch (error) {
    console.error(error)
    app.exit(1)
    throw error
  }
})()

interface NativeSession {
  child: ChildProcessByStdio<Writable, Readable, Readable>
  system: string
  generation: number
  outstanding: boolean
  lastSequence: number
  diagnostic: string
  failure?: string
  lastFrame?: NativeFrame
  closed: Promise<void>
}
protocol.registerSchemesAsPrivileged([
  {
    scheme: 'ares-plugin',
    privileges: {
      standard: true,
      secure: true,
      supportFetchAPI: true,
      corsEnabled: true,
    },
  },
])
export const sessions = new SessionRouter((reply) => {
  plugins.observe(reply)
  if (window && !window.isDestroyed() && !window.webContents.isDestroyed())
    window.webContents.send('session-state', reply)
})
export const plugins = new PluginRegistry(
  path.join(appRoot, 'bundled-plugins'),
  path.join(app.getPath('userData'), 'plugins'),
  sessions,
  () => sessions.identity,
  (catalog) => {
    if (window && !window.isDestroyed())
      window.webContents.send('plugin-catalog', catalog)
  },
)
const pluginCatalog = (offset = 0, limit = 4) => {
  if (
    !Number.isInteger(offset) ||
    offset < 0 ||
    offset > 128 ||
    !Number.isInteger(limit) ||
    limit < 1 ||
    limit > 4
  )
    throw new Error('invalid_plugin_page')
  const all = plugins.catalog()
  return {
    ...all,
    plugins: all.plugins
      .slice(offset, offset + limit)
      .map((p) => ({ ...p, logs: [] })),
    total: all.plugins.length,
    nextOffset: offset + limit < all.plugins.length ? offset + limit : null,
  }
}
const pluginActions = new Map<
  string,
  {
    token: string
    resolve: () => void
    reject: (error: Error) => void
    timer: ReturnType<typeof setTimeout>
  }
>()
async function pluginAction(
  token: string,
  kind: 'open' | 'command',
  name: string,
) {
  plugins.authorize(token)
  if (!window || window.isDestroyed()) throw new Error('renderer_unavailable')
  const id = randomUUID()
  await new Promise<void>((resolve, reject) => {
    const timer = setTimeout(() => {
      pluginActions.delete(id)
      reject(new Error('plugin_action_timeout; request was not replayed'))
    }, 3000)
    pluginActions.set(id, { token, resolve, reject, timer })
    window.webContents.send('plugin-action', { id, token, kind, name })
  })
  return { completed: true }
}
export const serviceHost = new LocalServiceHost(sessions, async (params) => {
  if (
    !params ||
    typeof params !== 'object' ||
    Object.keys(params).some(
      (key) =>
        ![
          'operation',
          'id',
          'source',
          'enabled',
          'token',
          'name',
          'offset',
          'limit',
        ].includes(key),
    )
  )
    throw new Error('invalid_plugin_request')
  switch (params.operation) {
    case 'list':
      return pluginCatalog(params.offset ?? 0, params.limit ?? 4)
    case 'add':
      if (typeof params.source !== 'string' || !path.isAbsolute(params.source))
        throw new Error('source_requires_absolute_directory')
      await plugins.add(params.source)
      return pluginCatalog()
    case 'enable':
      await plugins.enable(params.id, params.enabled)
      return pluginCatalog()
    case 'resetStartup':
      await plugins.resetStartup()
      return pluginCatalog()
    case 'reload':
      await plugins.reload(params.id)
      return pluginCatalog()
    case 'open':
    case 'command':
      if (typeof params.token !== 'string' || typeof params.name !== 'string')
        throw new Error('plugin_token_and_name_required')
      if (plugins.authorize(params.token).manifest?.id !== params.id)
        throw new Error('plugin_id_mismatch')
      return pluginAction(params.token, params.operation, params.name)
    default:
      throw new Error('unknown_plugin_operation')
  }
})
export { stop as stopNative }
const installedHost = path.join(
  process.resourcesPath,
  'native/bin/ares-electron-host',
)
const installedLayout = app.isPackaged || fs.existsSync(installedHost)
const hostPath =
  process.env.ARES_HOST ||
  (installedLayout
    ? installedHost
    : path.resolve(appRoot, '../build-electron-make/bin/ares-electron-host'))
const resourceDirectory = process.env.ARES_RESOURCE_DIR || path.dirname(hostPath)
let generation = 0
const probe = (args: string[]): Promise<string> =>
  new Promise((resolve, reject) => {
    execFile(hostPath, ['--resource-dir', resourceDirectory, ...args], { timeout: 30000 }, (error, stdout, stderr) => {
      if (error) reject(new Error(stderr.trim() || error.message))
      else resolve(stdout.trim())
    })
  })
let window: BrowserWindow
export const preferences = new SettingsStore(
  path.join(app.getPath('userData'), 'preferences.json'), [
    ...presentationSettings,
    { id: 'paths.importBml', title: 'Imported ares settings', section: 'Paths', type: 'string', file: { extensions: ['bml'] },
      default: path.join(app.getPath('userData'), 'settings.bml'), apply: 'next-rom' },
  ],
  snapshot => {
    if (window && !window.isDestroyed()) window.webContents.send('preferences-changed', snapshot)
  },
  undefined,
  process.env.ARES_SETTINGS ? { 'paths.importBml': process.env.ARES_SETTINGS } : {},
)
let nativeSettingsRegistered = false
async function refreshNativeSettings() {
  const importedFile = String(preferences.catalog().entries.find(entry => entry.descriptor.id === 'paths.importBml')!.value)
  const info = parseNativeSettingsInfo(await probe(['--settings-info', '--settings', importedFile]))
  // A newer file choice can arrive while the metadata probe is running.
  if (preferences.catalog().entries.find(entry => entry.descriptor.id === 'paths.importBml')!.value !== importedFile) return
  if (!nativeSettingsRegistered) {
    preferences.register(firmwareDescriptors(info))
    nativeSettingsRegistered = true
  }
  preferences.inherit(importedValues(info, preferences.catalog().entries.map(entry => entry.descriptor)))
}
async function setPreference(id: string, value: unknown, reset = false) {
  if (reset) await preferences.reset(id); else await preferences.set(id, value)
  if (id === 'paths.importBml') await refreshNativeSettings()
  return preferences.catalog()
}
async function choosePreferenceFile(id: string) {
  const descriptor = preferences.catalog().entries.find(entry => entry.descriptor.id === id)?.descriptor
  if (!descriptor?.file) throw new Error('not_a_file_setting')
  const result = await dialog.showOpenDialog(window, {
    title: descriptor.title, properties: ['openFile'],
    filters: descriptor.file.extensions.length ? [{ name: descriptor.title, extensions: descriptor.file.extensions }] : [],
  })
  return result.canceled ? preferences.catalog() : setPreference(id, result.filePaths[0])
}
let active: NativeSession | null = null,
  port: MessagePortMain | null = null
let quitting = false,
  operation: Promise<void> = Promise.resolve()
const status = (text: string) => {
  if (window && !window.isDestroyed()) window.webContents.send('status', text)
}
function command(text: string) {
  if (active && !active.child.stdin.destroyed && active.child.stdin.writable)
    active.child.stdin.write(text + '\n')
}
async function stop() {
  const old = active
  if (!old) return
  active = null
  plugins.disconnect()
  sessions.detach()
  // EOF also releases the native command reader. Escalate only for a stuck host.
  old.child.stdin.end('quit\n')
  const timer = setTimeout(() => old.child.kill('SIGKILL'), 2000)
  await old.closed
  clearTimeout(timer)
}
async function start(options: StartOptions) {
  let system = 'Synthetic'
  await refreshNativeSettings()
  const startupSettings = preferences.catalog()
  const settingsFile = String(startupSettings.entries.find(entry => entry.descriptor.id === 'paths.importBml')!.value)
  // Probe before stopping the running game, so an unsupported file leaves it intact.
  if (options.rom) {
    const identifyArgs = [
      '--identify',
      options.rom,
      '--save-dir',
      saveDirectory,
    ]
    if (options.system) identifyArgs.push('--system', options.system)
    if (settingsFile) identifyArgs.push('--settings', settingsFile)
    const matches = (await probe(identifyArgs)).split('\n').filter(Boolean)
    system = matches[0]
    if (matches.length > 1) {
      const choice = await dialog.showMessageBox(window, {
        type: 'question',
        title: 'Choose system',
        message: 'This file can be loaded by more than one system.',
        buttons: [...matches, 'Cancel'],
        cancelId: matches.length,
      })
      if (choice.response === matches.length) return
      system = matches[choice.response]
    }
  }
  await stop()
  const args = options.rom ? ['--rom', options.rom] : ['--synthetic']
  if (options.rom) args.push('--system', system)
  if (
    process.env.ARES_ENABLE_SERVICES === '1' ||
    process.argv.includes('--services')
  )
    args.push('--debug-provenance', '--debug-capture')
  for (const media of options.media || []) args.push('--media', media)
  if (settingsFile) args.push('--settings', settingsFile)
  if (options.hd) args.push('--1080p')
  if (options.silent || process.env.ARES_SILENT === '1') args.push('--silent')
  args.push('--save-dir', saveDirectory, '--resource-dir', resourceDirectory)
  const identity = { session: randomUUID(), epoch: 1 }
  args.push('--control-session', identity.session)
  const configurationDirectory = path.join(app.getPath('userData'), 'runtime-settings')
  fs.mkdirSync(configurationDirectory, { recursive: true })
  const configurationFile = path.join(configurationDirectory, `${identity.session}.bml`)
  fs.writeFileSync(configurationFile, nativeSettingsBml(startupSettings), { flag: 'wx', mode: 0o600 })
  args.push('--settings-overlay', configurationFile)
  const removeConfiguration = () => fs.rmSync(configurationFile, { force: true })
  const child = spawn(hostPath, args, {
    stdio: ['pipe', 'pipe', 'pipe', 'pipe'],
  }) as ChildProcessByStdio<Writable, Readable, Readable>
  const session: NativeSession = {
    child,
    system,
    generation: ++generation,
    outstanding: false,
    lastSequence: 0,
    diagnostic: '',
    closed: Promise.resolve(),
  }
  session.closed = new Promise((resolve) =>
    child.once('close', (code, signal) => {
      removeConfiguration()
      if (active === session) {
        active = null
        plugins.disconnect()
        sessions.detach()
        status(
          session.failure ||
            (code !== 0 || signal
              ? `Host stopped (${signal || code}). ${session.diagnostic}`
              : 'Host stopped. Open a ROM or restart a test.'),
        )
      }
      resolve()
    }),
  )
  active = session
  const client = new NativeSessionClient(
    child.stdin,
    child.stdio[3] as Readable,
    identity,
    (error) => {
      if (active === session) {
        session.failure = String(error)
        status(session.failure)
        child.kill()
      }
    },
  )
  sessions.attach(client)
  void client.request('session.status').then(reply => {
    removeConfiguration()
    if (reply.ok && active === session) preferences.markApplied(startupSettings, 'next-rom')
  }).catch(error => status(String(error)))
  child.on('error', (error) =>
    status(`Cannot start native host: ${error.message}`),
  )
  child.stdin.on('error', () => {}) // exit races are reported by close/error
  child.stderr.on('data', (data) => {
    process.stderr.write(data)
    session.diagnostic = data.toString().trim()
    if (active === session) status(session.diagnostic)
  })
  const parser = new FrameParser((frame) => {
    if (active !== session) return
    session.outstanding = true
    session.lastSequence = frame.sequence
    session.lastFrame = frame
    // This is intentionally a copied baseline. MessagePortMain does not transfer ArrayBuffers.
    port?.postMessage({
      ...frame,
      generation: session.generation,
      receivedMs: performance.timeOrigin + performance.now(),
    })
  })
  child.stdout.on('data', (data) => {
    try {
      parser.push(data)
    } catch (error) {
      session.failure = String(error)
      status(session.failure)
      child.kill()
    }
  })
  port?.postMessage({ reset: true, system })
  command('ack')
}
function enqueue(task: () => Promise<void>) {
  operation = operation.then(task).catch((error) => {
    status(error.message)
    console.error(error)
  })
  return operation
}
app
  .whenReady()
  .then(async () => {
    await refreshNativeSettings()
    await plugins.initialize(
      (process.env.ARES_PLUGIN_DIRS || '')
        .split(path.delimiter)
        .filter(Boolean),
      process.env.ARES_DEFAULT_PLUGINS === '0'
        ? []
        : [
            'ares.examples.memory',
            'ares.tools.debugger',
            'ares.examples.kirby',
          ],
      process.argv.includes('--disable-plugins') ||
        process.env.ARES_DISABLE_PLUGINS === '1',
    )
    protocol.handle('ares-plugin', async (request) => {
      try {
        const { bytes, type } = await plugins.resource(request.url)
        return new Response(new Uint8Array(bytes), {
          headers: { 'Content-Type': type, 'Access-Control-Allow-Origin': '*' },
        })
      } catch {
        return new Response('Plugin resource unavailable or scope expired', {
          status: 404,
        })
      }
    })
    if (
      process.env.ARES_ENABLE_SERVICES === '1' ||
      process.argv.includes('--services')
    ) {
      const file = await serviceHost.listen(process.env.ARES_SERVICE_DIR)
      console.error(`ares service instance ${serviceHost.instance}: ${file}`)
    }
    window = new BrowserWindow({
      show: process.env.ARES_BACKGROUND_TEST !== '1',
      width: 1100,
      height: 800,
      minWidth: 650,
      minHeight: 480,
      webPreferences: {
        preload: path.join(appRoot, 'dist/preload.cjs'),
        contextIsolation: true,
        nodeIntegration: false,
        sandbox: true,
        backgroundThrottling: false,
      },
    })
    window.webContents.setWindowOpenHandler(() => ({ action: 'deny' }))
    window.webContents.on('will-navigate', (event) => event.preventDefault())
    window.webContents.on('render-process-gone', () => {
      plugins.detach()
      port?.close()
      port = null
    })
    window.webContents.on('did-start-loading', () => {
      plugins.detach()
      port?.close()
      port = null
    })
    ipcMain.on('ready', (event) => {
      if (event.sender !== window.webContents) return
      port?.close()
      const channel = new MessageChannelMain()
      port = channel.port1
      port.on('message', ({ data }) => {
        if (port !== channel.port1) return
        if (
          active &&
          data?.generation === active.generation &&
          data?.ack === active.lastSequence &&
          active.outstanding
        ) {
          active.outstanding = false
          command('ack')
        }
      })
      port.start()
      window.webContents.postMessage('frames', null, [channel.port2])
      if (active) {
        port.postMessage({ reset: true, system: active.system })
        if (active.outstanding && active.lastFrame)
          port.postMessage({
            ...active.lastFrame,
            generation: active.generation,
            receivedMs: performance.timeOrigin + performance.now(),
          })
      } else
        enqueue(() =>
          start(
            process.env.ARES_ROM
              ? { rom: process.env.ARES_ROM, system: process.env.ARES_SYSTEM }
              : { hd: process.env.ARES_HD === '1' },
          ),
        )
    })
    ipcMain.handle(
      'plugin.actionResult',
      (event, id: string, token: string, error?: string) => {
        if (
          event.sender !== window.webContents ||
          event.senderFrame !== window.webContents.mainFrame
        )
          throw new Error('invalid_sender')
        const action = pluginActions.get(id)
        if (!action || action.token !== token) return
        pluginActions.delete(id)
        clearTimeout(action.timer)
        try {
          plugins.authorize(token)
          if (error) throw new Error(String(error).slice(0, 2048))
          action.resolve()
        } catch (error) {
          action.reject(
            error instanceof Error ? error : new Error(String(error)),
          )
        }
      },
    )
    for (const name of [
      'catalog',
      'enable',
      'add',
      'refresh',
      'request',
      'storage',
      'log',
      'fail',
      'reload',
      'begin',
      'finish',
      'reject',
      'rollback',
    ] as const) {
      ipcMain.handle('plugin.' + name, async (event, ...args) => {
        if (
          event.sender !== window.webContents ||
          event.senderFrame !== window.webContents.mainFrame
        )
          throw new Error('invalid_sender')
        switch (name) {
          case 'catalog':
            return plugins.attach()
          case 'reload':
            return plugins.reload(args[0])
          case 'begin':
            return plugins.begin(args[0])
          case 'finish':
            return plugins.finish(args[0])
          case 'reject':
            return plugins.reject(args[0], args[1])
          case 'rollback':
            return plugins.rollback(args[0], args[1])
          case 'enable':
            return plugins.enable(args[0], args[1])
          case 'refresh':
            return plugins.refresh()
          case 'request':
            return plugins.request(args[0], args[1], args[2])
          case 'storage':
            return plugins.storage(args[0], args[1], args[2], args[3])
          case 'fail':
            return plugins.fail(args[0], args[1])
          case 'log':
            return plugins.log(args[0], args[1], args[2])
          case 'add': {
            const result = await dialog.showOpenDialog(window, {
              title: 'Add trusted local plugin directory',
              properties: ['openDirectory'],
            })
            return result.canceled
              ? plugins.catalog()
              : plugins.add(result.filePaths[0])
          }
        }
      })
    }
    ipcMain.handle(
      'session',
      async (
        event,
        method: ServiceMethod,
        identity?: SessionIdentity,
        params?: ServiceParams<ServiceMethod>,
      ) => {
        if (
          event.sender !== window.webContents ||
          event.senderFrame !== window.webContents.mainFrame
        )
          throw new Error('invalid_sender')
        if (!['session.status', 'memory.regions'].includes(method) && !identity)
          throw new Error('invalid_request')
        return sessions.request(method, identity, params)
      },
    )
    for (const operation of ['catalog', 'set', 'reset', 'chooseFile'] as const) {
      ipcMain.handle('preferences.' + operation, (event, ...args: unknown[]) => {
        if (event.sender !== window.webContents || event.senderFrame !== window.webContents.mainFrame)
          throw new Error('invalid_sender')
        if (operation === 'catalog' && args.length === 0) return preferences.catalog()
        if (operation === 'set' && args.length === 2 && typeof args[0] === 'string')
          return setPreference(args[0], args[1])
        if (operation === 'reset' && args.length === 1 && typeof args[0] === 'string')
          return setPreference(args[0], undefined, true)
        if (operation === 'chooseFile' && args.length === 1 && typeof args[0] === 'string')
          return choosePreferenceFile(args[0])
        throw new Error('invalid_request')
      })
    }
    ipcMain.handle('systems', async (event) => {
      if (event.sender !== window.webContents) return []
      return (await probe(['--systems'])).split('\n').filter(Boolean)
    })
    ipcMain.handle('settings', async (event) => {
      if (event.sender !== window.webContents) return
      const result = await dialog.showOpenDialog(window, {
        title: 'Use ares firmware and core settings',
        properties: ['openFile'],
        filters: [{ name: 'ares settings', extensions: ['bml'] }],
      })
      if (!result.canceled) {
        await setPreference('paths.importBml', result.filePaths[0])
        status('Selected ares settings; they will apply to the next ROM.')
      }
    })
    ipcMain.handle('open', async (event, options) => {
      if (event.sender !== window.webContents) return
      const result = await dialog.showOpenDialog(window, {
        title: 'Open ROM (additional selections supply companion media)',
        properties: ['openFile', 'multiSelections'],
        filters: [{ name: 'All ROMs and media', extensions: ['*'] }],
      })
      if (!result.canceled)
        await enqueue(() =>
          start({
            rom: result.filePaths[0],
            media: result.filePaths.slice(1),
            system: typeof options?.system === 'string' ? options.system : '',
          }),
        )
    })
    ipcMain.on('command', (event, data: NativeCommand) => {
      if (event.sender !== window.webContents || !data) return
      if (data.type === 'synthetic')
        enqueue(() => start({ hd: data.hd === true }))

      if (
        data.type === 'input' &&
        Number.isInteger(data.sequence) &&
        Number.isInteger(data.mask)
      ) {
        const axis = (value: number | undefined) =>
          typeof value === 'number' && Number.isFinite(value)
            ? Math.max(-32767, Math.min(32767, Math.round(value)))
            : 0
        command(
          `input ${data.sequence >>> 0} ${data.mask >>> 0} ${axis(data.x)} ${axis(data.y)} ${axis(data.rx)} ${axis(data.ry)}`,
        )
      }
      if (
        data.type === 'key' &&
        typeof data.name === 'string' &&
        /^[A-Za-z0-9]{1,32}$/.test(data.name)
      ) {
        command(`key ${data.name} ${data.value ? 1 : 0}`)
      }
      if (data.type === 'clear') command('clear')
      if (data.type === 'mouse') {
        const limit = (value: number | undefined) =>
          typeof value === 'number' && Number.isFinite(value)
            ? Math.max(-32767, Math.min(32767, Math.round(value)))
            : 0
        command(`mouse ${limit(data.x)} ${limit(data.y)} ${data.mask & 15}`)
      }
      if (data.type === 'report') {
        // Opt-in automated renderer smoke test; no general filesystem API is exposed.
        if (process.env.ARES_REPORT)
          fs.writeFileSync(
            process.env.ARES_REPORT,
            JSON.stringify(data.report, null, 2),
          )
        if (process.env.ARES_AUTOTEST === '1') app.quit()
      }
    })
    window.loadFile(path.join(appRoot, 'index.html'), {
      query: {
        autotest: process.env.ARES_AUTOTEST || '',
        duration: process.env.ARES_TEST_SECONDS || '12',
      },
    })
  })
  .catch((error) => {
    console.error(error)
    app.quit()
  })
app.on('window-all-closed', () => app.quit())
app.on('before-quit', (event) => {
  if (quitting) return
  event.preventDefault()
  quitting = true
  enqueue(stop).finally(async () => {
    await plugins.close()
    await serviceHost.close()
    app.quit()
  })
})
