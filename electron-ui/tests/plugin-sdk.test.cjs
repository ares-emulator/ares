const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
test('scaffold ships complete browser-only SDK declarations with strict service/settings types', t => {
  const directory = fs.mkdtempSync('/tmp/ares-plugin-sdk-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  fs.cpSync(path.join(__dirname, '../templates/plugin-tsx'), directory, { recursive: true });
  fs.writeFileSync(path.join(directory, 'src/api.ts'), `
import type { PluginContext, ServiceResult, PluginSettingDescriptor } from './sdk.js';
export async function exercise(ctx: PluginContext) {
  const section: PluginSettingDescriptor[] = [{id:'count', title:'Count', type:'number', min:0, max:10, default:1}];
  const dispose = await ctx.settings.registerSection('general', section);
  ctx.scope.add(dispose);
  ctx.settings.changed(snapshot => console.log(snapshot.entries[0]?.source));
  ctx.settings.mount(document.createElement('div'));
  await ctx.services.request('n64.memory.read', {address:'0x80000000', view:'cpu', length:4});
  const settings: ServiceResult<'runtime.settings'> = {volume:0.4,mute:false,luminance:1,saturation:1,gamma:1};
  await ctx.services.request('runtime.configure', settings);
  const saved = await ctx.storage.get<{count:number}>('state');
  ctx.ui.registerPanel('typed', panel => panel.onVisibility(visible => console.log(visible, saved?.count)));
  ctx.scope.listen(document.body, 'click', () => console.log(ctx.signal.aborted));
  // @ts-expect-error unknown service method
  await ctx.services.request('missing.method');
  // @ts-expect-error service parameter type
  await ctx.services.request('memory.read', {region:'x', offset:0, length:'four'});
  // @ts-expect-error numeric setting cannot have a string default
  await ctx.settings.registerSection('bad', [{id:'count', title:'Count', type:'number', min:0, max:10, default:'bad'}]);
  // @ts-expect-error plugin fields cannot bind arbitrary native keys
  await ctx.settings.registerSection('bad', [{id:'flag', title:'Flag', type:'boolean', default:false, nativeKey:'Boot/Fast'}]);
}
`);
  const result = spawnSync(process.execPath, [path.join(path.dirname(require.resolve('typescript/package.json')), 'bin/tsc'), '-p', path.join(directory, 'tsconfig.json')], { encoding: 'utf8', timeout: 15000 });
  assert.equal(result.status, 0, result.stdout + result.stderr);
});
