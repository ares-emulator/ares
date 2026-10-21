import { StdioPeer } from './stdio-peer.cjs'
import { fileURLToPath } from 'node:url'
import { app, BrowserWindow } from 'electron'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import assert from 'node:assert/strict'
const temporary = fs.mkdtempSync('/tmp/ares-reload-'),
  source = path.join(temporary, 'plugin')
fs.mkdirSync(source)
app.setPath('userData', path.join(temporary, 'profile'))
process.env.ARES_SAVE_DIR = path.join(temporary, 'saves')
process.env.ARES_PLUGIN_DIRS = source
process.env.ARES_ENABLE_SERVICES = '1'
process.env.ARES_SERVICE_DIR = path.join(temporary, 'ipc')
delete process.env.ARES_ROM
delete process.env.ARES_AUTOTEST
const manifest = {
  id: 'test.reload',
  name: 'Reload fixture',
  version: '1.0.0',
  apiVersion: 1,
  entry: 'plugin.js',
  capabilities: [
    'session.read',
    'session.control',
    'memory.read',
    'memory.write',
  ],
  contributes: {
    panels: [{ id: 'panel', title: 'Reload fixture' }],
    commands: [
      { id: 'open', title: 'Open reload fixture' },
      { id: 'patch', title: 'Explicit patch' },
      { id: 'late', title: 'Late patch' },
    ],
  },
}
fs.writeFileSync(path.join(source, 'manifest.json'), JSON.stringify(manifest))
function plugin(version, extra = '', prepare = '') {
  return `
 let value=0;export const stateVersion=1;export function saveState(){return {value}};
 ${prepare}
 export async function activate(ctx,state){
  if(window.failRollback)throw new Error('deliberate rollback failure');
  value=state?.value??0;
  ctx.scope.listen(window,'plugin-probe',()=>{window.probes=(window.probes||0)+1});
  ctx.scope.interval(async()=>{window.ticks=(window.ticks||0)+1;await ctx.services.request('session.status')},50);
  ctx.scope.add(()=>{window.cleanups=(window.cleanups||0)+1});ctx.scope.add(()=>window.hangCleanup?new Promise(()=>{}):undefined);
  ctx.ui.registerPanel('panel',({element})=>{const input=document.createElement('input');input.value=String(value);input.dataset.value='';ctx.scope.listen(input,'input',()=>{value=Number(input.value)});const label=document.createElement('span');label.dataset.version='${version}';label.textContent='Version ${version}';element.append(label,input)});
  const patch=async()=>{const regions=await ctx.services.request('memory.regions');const region=regions.result.regions.find(r=>r.writable&&r.readable);const reply=await ctx.services.request('memory.write',{region:region.id,offset:240,hex:'11223344'});if(!reply.ok)throw new Error(reply.error);window.patches=(window.patches||0)+1;};
  ctx.ui.registerCommand('open',()=>ctx.ui.openPanel('panel'));
  ctx.ui.registerCommand('patch',patch);
  ctx.ui.registerCommand('late',async()=>{window.lateStarted=true;await new Promise(r=>setTimeout(r,500));try{await patch()}catch(error){window.lateError=String(error)}});
  ${extra}
 }
 `
}
const entry = path.join(source, 'plugin.js')
fs.writeFileSync(entry, plugin(1))
let peer
const { sessions, plugins, serviceHost, stopNative } =
  await import('../dist/main.js')
const sleep = (ms) => new Promise((r) => setTimeout(r, ms))
const until = async (fn, label) => {
  for (let i = 0; i < 300; i++) {
    try {
      const value = await fn()
      if (value) return value
    } catch {}
    await sleep(25)
  }
  throw new Error('Timed out: ' + label)
}
const timer = setTimeout(() => {
  console.error('reload integration deadline')
  app.exit(1)
}, 90000)
let stage = 'startup'
const enter = (s) => {
  stage = s
  console.log('RELOAD_STAGE:', s)
}
app.whenReady().then(async () => {
  try {
    await until(() => BrowserWindow.getAllWindows()[0], 'window')
    const window = BrowserWindow.getAllWindows()[0],
      js = (code) => window.webContents.executeJavaScript(code)
    const info = () =>
      plugins.catalog().plugins.find((p) => p.manifest?.id === 'test.reload')
    const idle = () =>
      until(
        () =>
          info()?.phase === 'active' &&
          !info()?.candidate &&
          info()?.reload !== 'building' &&
          info()?.reload !== 'activating',
        'active generation',
      )
    const command = (id) =>
      js(
        `document.querySelector('[data-plugin-command="test.reload:${id}"]').click()`,
      )
    const version = (n) =>
      until(
        () => js(`!!document.querySelector('[data-version="${n}"]')`),
        'version ' + n,
      )
    await idle()
    await command('open')
    await version(1)
    const baseline = await sessions.request('session.pause')
    const identity = { session: baseline.session, epoch: baseline.epoch }
    const oldCatalog=plugins.catalog()
    const regions = (await sessions.request('memory.regions')).result.regions,
      region = regions.find((r) => r.writable && r.readable)
    const read = async () =>
      (
        await sessions.request('memory.read', identity, {
          region: region.id,
          offset: 240,
          length: 4,
        })
      ).result.hex
    const original = await read()
    await js(
      "const input=document.querySelector('[data-value]');input.value='37';input.dispatchEvent(new Event('input'));window.originalPanel=document.querySelector('[data-plugin-panel]')",
    )
    const counts = () =>
      js(
        "(()=>{const d=document.getElementById('plugins').dataset;return {scopes:+d.scopes,listeners:+d.listeners,timers:+d.timers}})()",
      )
    const before = await counts()
    assert.deepEqual(before, { scopes: 1, listeners: 2, timers: 1 })
    enter('invalid edit preserves live UI')
    const token1 = info().lease.token
    fs.writeFileSync(entry, 'export function activate( { broken')
    await until(() => info().reload === 'error', 'build diagnostic')
    assert.equal(info().lease.token, token1)
    assert.equal(
      await js(
        "window.originalPanel===document.querySelector('[data-plugin-panel]')",
      ),
      true,
    )
    enter('watched recovery and UI state')
    fs.writeFileSync(entry, plugin(2))
    await version(2)
    await idle()
    assert.equal(await js("document.querySelector('[data-value]').value"), '37')
    assert.deepEqual(await counts(), before)
    assert.equal(
      (await sessions.request('session.status')).epoch,
      identity.epoch,
    )
    assert.equal(await read(), original)
    enter('late catalog snapshots cannot revive the old scope')
    await js("window.currentPanel=document.querySelector('[data-plugin-panel]')")
    window.webContents.send('plugin-catalog',oldCatalog);await sleep(100)
    assert.equal(await js("window.currentPanel===document.querySelector('[data-plugin-panel]')"),true);assert.deepEqual(await counts(),before)
    enter('failed candidate preparation')
    await js(
      "window.originalPanel=document.querySelector('[data-plugin-panel]')",
    )
    fs.writeFileSync(
      entry,
      plugin(
        3,
        '',
        "export function prepare(){throw new Error('prepare rejected')}",
      ),
    )
    await until(
      () => info().error?.includes('prepare rejected'),
      'prepare failure',
    )
    assert.equal(
      await js(
        "window.originalPanel===document.querySelector('[data-plugin-panel]')",
      ),
      true,
    )
    enter('failed activation restores immutable artifact')
    fs.writeFileSync(
      entry,
      plugin(4, "throw new Error('activation rejected');"),
    )
    await until(
      () =>
        info().error?.includes('previous artifact restored') &&
        info().phase === 'active',
      'activation rollback',
    )
    await version(2)
    assert.equal(await js("document.querySelector('[data-value]').value"), '37')
    assert.deepEqual(await counts(), before)
    assert.equal(await read(), original)
    enter('activation cannot replay a patch')
    fs.writeFileSync(entry, plugin(5, 'await patch();'))
    await until(
      () =>
        info().error?.includes('plugin_not_ready') && info().phase === 'active',
      'blocked activation write',
    )
    await version(2)
    assert.equal(await read(), original)
    enter('explicit recovery and stale async write')
    fs.writeFileSync(entry, plugin(6))
    await version(6)
    await idle()
    await command('late')
    await until(() => js('window.lateStarted'), 'late command started')
    const oldToken = info().lease.token
    await plugins.reload('test.reload')
    await until(
      () => info().lease?.token !== oldToken && info().phase === 'active',
      'manual reload',
    )
    await until(
      () => js("window.lateError?.includes('stale_plugin_scope')"),
      'stale async write rejected',
    )
    assert.equal(await read(), original)
    enter('repeated reloads keep effects bounded')
    for (let i = 0; i < 12; i++) {
      const token = info().lease.token
      await plugins.reload('test.reload')
      await until(
        () => info().lease?.token !== token && info().phase === 'active',
        'reload ' + i,
      )
      await until(
        () => js("document.getElementById('plugins').dataset.jobs==='0'"),
        'idle job',
      )
      assert.deepEqual(await counts(), before)
    }
    await js("window.probes=0;window.dispatchEvent(new Event('plugin-probe'))")
    assert.equal(await js('window.probes'), 1)
    assert.equal(await read(), original)
    const cache = path.join(temporary, 'profile/plugins/artifacts')
    await until(
      () =>
        fs.readdirSync(cache).filter((n) => /^[a-f0-9]{64}$/.test(n)).length <=
        plugins.catalog().plugins.filter((p) => p.manifest).length + 1,
      'artifact pruning',
    )
    enter('hidden polling stays idle')
    await js("document.querySelector('#plugin-dock nav button').click()")
    await sleep(100)
    const ticks = await js('window.ticks')
    await sleep(180)
    assert.equal(await js('window.ticks'), ticks)
    await command('open')
    enter('rapid edits invalidate a preparing candidate')
    await js('window.rapidPreparing=false')
    fs.writeFileSync(
      entry,
      plugin(
        60,
        '',
        'export async function prepare(){window.rapidPreparing=true;await new Promise(r=>setTimeout(r,600))}',
      ),
    )
    await until(() => js('window.rapidPreparing'), 'rapid candidate preparing')
    fs.writeFileSync(entry, plugin(6))
    await until(() => info().reload === undefined, 'latest source captured')
    await until(
      () => js("document.getElementById('plugins').dataset.jobs==='0'"),
      'superseded candidate settled',
    )
    await version(6)
    assert.equal(
      await js('!!document.querySelector(\'[data-version="60"]\')'),
      false,
    )
    assert.deepEqual(await counts(), before)
    enter('cleanup timeout is bounded and reported')
    await js('window.hangCleanup=true')
    fs.writeFileSync(entry, plugin(61))
    await until(
      () =>
        info().phase === 'active' &&
        info().error?.includes('Plugin cleanup timed out'),
      'bounded cleanup rollback',
    )
    await js('window.hangCleanup=false')
    await version(6)
    assert.deepEqual(await counts(), before)
    enter('external MCP owner survives replacement')
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
    const running = peer.tool('run_frames', { n: 1000000, max_seconds: 10 })
    const agentId = await until(
      async () =>
        (await sessions.request('session.status')).result.operation_id,
      'MCP operation',
    )
    await plugins.reload('test.reload')
    await idle()
    const operation = await peer.tool('operation_status', {
      operation_id: agentId,
    })
    assert.equal(operation.state, 'running')
    await peer.tool('cancel_operation', { operation_id: agentId })
    assert.equal((await running).stop_reason, 'cancelled')
    await peer.close()
    peer = undefined
    enter('plugin-owned operation is released')
    const pluginToken = info().lease.token,
      owned = await plugins.request(pluginToken, 'execution.runFrames', {
        count: 1000000,
        timeoutMs: 10000,
      })
    assert.equal(owned.ok, true)
    await plugins.reload('test.reload')
    await idle()
    const released = await sessions.request('execution.status', identity, {
      operationId: owned.result.operation_id,
    })
    assert.equal(released.result.state, 'done')
    assert.equal(released.result.stop_reason, 'disconnected')

    enter('patch requires an explicit command')
    await command('patch')
    await until(async () => (await read()) === '11223344', 'explicit patch')
    assert.equal(await js('window.patches'), 1)
    await sessions.request('memory.write', identity, {
      region: region.id,
      offset: 240,
      hex: original,
    })
    enter('rollback failure is reported and recoverable')
    await js('window.failRollback=true')
    fs.writeFileSync(entry, plugin(7))
    await until(
      () =>
        info().state === 'error' && info().error?.includes('rollback failed'),
      'rollback failure',
    )
    assert.equal(
      await js("document.querySelector('[data-plugin-panel]')===null"),
      true,
    )
    await js('window.failRollback=false')
    fs.writeFileSync(entry, plugin(8))
    await idle()
    await command('open')
    await version(8)
    enter('incompatible transient state is not restored')
    await js(
      "(()=>{const input=document.querySelector('[data-value]');input.value='91';input.dispatchEvent(new Event('input'))})()",
    )
    fs.writeFileSync(
      entry,
      plugin(81).replace('stateVersion=1', 'stateVersion=2'),
    )
    await version(81)
    await idle()
    assert.equal(await js("document.querySelector('[data-value]').value"), '0')
    enter('session switch during preparation')
    fs.writeFileSync(
      entry,
      plugin(
        9,
        '',
        'export async function prepare(){window.preparing=true;await new Promise(r=>setTimeout(r,600))}',
      ),
    )
    await until(() => js('window.preparing'), 'candidate preparation')
    await js("document.getElementById('synthetic').click()")
    const switched = await until(async () => {
      const s = await sessions.request('session.status')
      return s.session !== identity.session ? s : false
    }, 'new native session')
    await idle()
    assert.notEqual(switched.session, identity.session)
    await command('open')
    assert.deepEqual(await counts(), before)
    enter('disable removes every SDK effect')
    await plugins.enable('test.reload', false)
    await until(
      () => js("document.getElementById('plugins').dataset.scopes==='0'"),
      'scope disposal',
    )
    assert.deepEqual(await counts(), { scopes: 0, listeners: 0, timers: 0 })
    const saved = await js('window.ticks')
    await sleep(150)
    assert.equal(await js('window.ticks'), saved)
    console.log('PLUGIN_RELOAD_PASS')
    clearTimeout(timer)
    if (peer) await peer.close()
    await plugins.close()
    await stopNative()
    app.exit(0)
  } catch (error) {
    console.error('RELOAD_FAILED:', stage, error)
    clearTimeout(timer)
    if (peer) await peer.close()
    await plugins.close()
    await stopNative()
    app.exit(1)
  }
})
app.on('will-quit', () =>
  fs.rmSync(temporary, { recursive: true, force: true }),
)
