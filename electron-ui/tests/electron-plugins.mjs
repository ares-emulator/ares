import { buildSync } from 'esbuild'
import { fileURLToPath } from 'node:url'
import { app, BrowserWindow } from 'electron'
import fs from 'node:fs'
import os from 'node:os'
import path from 'node:path'
import assert from 'node:assert/strict'
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-plugin-ui-'))
app.setPath('userData', path.join(temporary, 'profile'))
process.env.ARES_SAVE_DIR = path.join(temporary, 'profile', 'saves')
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
const overload = path.join(temporary, 'overload')
fs.mkdirSync(overload)
fs.writeFileSync(path.join(overload, 'manifest.json'), JSON.stringify({...sample, id:'test.overload', name:'Overload', capabilities:['memory.read'], contributes:{commands:[{id:'refresh',title:'Overload refresh'}]}}))
fs.writeFileSync(path.join(overload, 'plugin.js'), `export function activate(ctx){window.overloadCtx=ctx;let refreshing=false;ctx.ui.registerCommand('refresh',async()=>{if(refreshing)return;refreshing=true;try{const regions=await ctx.services.request('memory.regions');const r=regions.result.regions.find(r=>r.readable);window.overloadResults=await Promise.allSettled(Array.from({length:24},()=>ctx.services.request('memory.read',{region:r.id,offset:0,length:1})));window.overloadLater=await ctx.services.request('memory.read',{region:r.id,offset:0,length:1});}finally{refreshing=false;window.overloadRefreshes=(window.overloadRefreshes||0)+1;}});}`)
process.env.ARES_PLUGIN_DIRS = [template, failing, delayed, overload].join(path.delimiter)
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
    await until(() => js(`!!document.querySelector('[data-plugin-command="test.overload:refresh"]')`))
    for(let run=1;run<=2;run++) {
      await js(`document.querySelector('[data-plugin-command="test.overload:refresh"]').click()`)
      await until(() => js(`window.overloadRefreshes === ${run}`))
      const result=await js(`({statuses:window.overloadResults.map(r=>r.status),errors:window.overloadResults.filter(r=>r.status==='rejected').map(r=>({code:r.reason.code,limit:r.reason.limit})),later:window.overloadLater.ok})`)
      assert.equal(result.statuses.length,24);assert.equal(result.statuses.filter(s=>s==='fulfilled').length,8)
      assert.equal(result.errors.length,16);assert.ok(result.errors.every(e=>e.code==='request_overload'&&e.limit===8));assert.equal(result.later,true)
    }
    const hostToken=plugins.catalog().plugins.find(p=>p.manifest?.id==='test.overload').lease.token
    const hostRegion=(await sessions.request('memory.regions')).result.regions.find(r=>r.readable)
    const originalHostRequest=sessions.request
    sessions.request=async function(method,...args){if(method==='memory.read')await new Promise(r=>setTimeout(r,40));return originalHostRequest.call(this,method,...args)}
    try {
      const hostError=await js(`(async()=>{const p={region:${JSON.stringify(hostRegion.id)},offset:0,length:1};const accepted=Array.from({length:8},()=>window.ares.plugins.request(${JSON.stringify(hostToken)},'memory.read',p));let failure;try{await window.overloadCtx.services.request('memory.read',p)}catch(e){failure={code:e.code,limit:e.limit,pending:e.pending}}await Promise.all(accepted);return failure})()`)
      assert.deepEqual(hostError,{code:'request_overload',limit:8,pending:8})
      assert.equal(plugins.records.find(p=>p.manifest?.id==='test.overload').pending,0)
      await js(`window.pendingUnload=Promise.allSettled(Array.from({length:24},()=>window.overloadCtx.services.request('memory.read',{region:${JSON.stringify(hostRegion.id)},offset:0,length:1})));true`)
      await plugins.enable('test.overload',false)
      assert.equal(await js(`window.pendingUnload.then(results=>results.length)`),24)
      assert.equal(plugins.records.find(p=>p.manifest?.id==='test.overload').pending,0)
      await plugins.enable('test.overload',true)
      await until(()=>js(`!window.overloadCtx.signal.aborted`))
      assert.equal(await js(`window.overloadCtx.services.request('memory.regions').then(r=>r.ok)`),true,'a new generation works after pending unload')
    } finally {sessions.request=originalHostRequest}
    await plugins.enable('ares.examples.memory', true)
    await until(() => js(`!!document.querySelector('[data-plugin-command="ares.examples.memory:diff"]')`))
    await js(`document.querySelector('[data-plugin-command="ares.examples.memory:diff"]').click()`)
    await until(() => js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]')?.textContent.includes('Capture two')`))
    for(const action of ['before','after','compare']) {
      await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-action="${action}"]').click()`)
      await until(() => js(`!document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-action="before"]').disabled`))
    }
    assert.match(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]').textContent`), /0 matches/)
    await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-field="label"]').value='fixture label';document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-action="label"]').click()`)
    await until(() => js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]').textContent==='Label saved.'`))
    const diffAction = async (action) => {
      await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-action="${action}"]').click()`)
      await until(() => js(`!document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-action="before"]').disabled`))
    }
    const fixtureRegion = (await sessions.request('memory.regions')).result.regions.find(r=>r.writable)
    await diffAction('before')
    await sessions.request('memory.write', undefined, {region:fixtureRegion.id,offset:0,hex:'aa'})
    await diffAction('after'); await diffAction('compare')
    assert.match(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]').textContent`), /1 candidates total/)
    await diffAction('before')
    await sessions.request('memory.write', undefined, {region:fixtureRegion.id,offset:0,hex:'bbcc'})
    await diffAction('after'); await diffAction('narrow')
    assert.match(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]').textContent`), /1 candidates total/)
    assert.equal(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] tbody tr td').textContent`),'0x0')
    assert.equal(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] tbody tr td:last-child').textContent`),'fixture label')
    await diffAction('map')
    assert.match(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] tbody').textContent`), /2 bytes/)
    await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-field="capacity"]').value='1';document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-field="duration"]').value='3'`)
    await diffAction('record'); assert.equal((await sessions.request('session.status')).result.paused,true)
    await sessions.request('session.resume')
    await new Promise(r=>setTimeout(r,100))
    await diffAction('recordStatus')
    assert.match(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [data-recording]').textContent`), /2 overwritten/)
    await diffAction('frameBefore')
    assert.match(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]').textContent`), /frame [0-9]+/)
    await sessions.request('session.pause')
    await diffAction('clear')
    await js(`document.querySelector('#settings-dialog').close();document.querySelector('[data-plugin-panel="ares.examples.memory:diff"]').closest('.floating-panel').style.cssText+=';width:880px;height:620px;left:20px;top:20px;z-index:999'`)
    await new Promise(r=>setTimeout(r,100))
    assert.equal(await js(`document.querySelector('#settings-dialog').open`),false)
    fs.writeFileSync(fileURLToPath(new URL('../artifacts/memory-explorer.png', import.meta.url)), (await window.webContents.capturePage()).toPNG())
    const oldDiffToken=plugins.catalog().plugins.find(p=>p.manifest?.id==='ares.examples.memory').lease.token
    await plugins.reload('ares.examples.memory')
    await until(()=>plugins.catalog().plugins.find(p=>p.manifest?.id==='ares.examples.memory').lease?.token!==oldDiffToken)
    await until(()=>js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] [role="status"]')?.textContent.includes('Capture two')`))
    await diffAction('before');await sessions.request('memory.write',undefined,{region:fixtureRegion.id,offset:0,hex:'dd'});await diffAction('after');await diffAction('compare')
    assert.equal(await js(`document.querySelector('[data-plugin-panel="ares.examples.memory:diff"] tbody tr td:last-child').textContent`),'fixture label','labels survive plugin reload')
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
    assert.equal(await js(`!!document.querySelector('[data-plugin-panel="ares.examples.memory:memory"]')`), false, 'activation does not open a panel');
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
    await js("window.dispatchEvent(new KeyboardEvent('keydown', {code:'Comma',metaKey:true,bubbles:true,cancelable:true})); document.querySelector('[data-tab=plugins]').click()");
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
