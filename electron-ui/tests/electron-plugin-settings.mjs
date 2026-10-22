import { app, BrowserWindow } from 'electron';
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const temporary = fs.mkdtempSync('/tmp/ares-plugin-settings-ui-');
const profile = path.join(temporary, 'profile'), source = path.join(temporary, 'plugin');
fs.mkdirSync(path.join(profile, 'plugins'), { recursive: true }); fs.mkdirSync(source);
fs.writeFileSync(path.join(source, 'manifest.json'), JSON.stringify({ id: 'test.preferences', name: 'Local preferences', version: '1.0.0', apiVersion: 1, entry: 'plugin.js', capabilities: ['settings.register'], contributes: { settings: [{ id: 'display', title: 'Display' }], panels: [{ id: 'own', title: 'Local settings' }], commands: [{ id: 'open', title: 'Open local settings' }] } }));
fs.writeFileSync(path.join(source, 'plugin.js'), `
export async function activate(context) {
  await context.settings.registerSection('display', [{id:'accent', title:'Accent', type:'enum', options:['blue','green'], default:'blue'}]);
  window.stagedPreferences = await context.settings.catalog();
  try { await context.settings.set('plugin:test.preferences:display:accent', 'green'); throw new Error('activation wrote settings'); }
  catch(error) { if (!String(error).includes('plugin_not_ready')) throw error; }
  window.settingsActivationDenied = true;
  if (!window.allowSettingsActivation) await new Promise(resolve => { window.finishSettingsActivation = resolve; });
  window.localSettings = context.settings;
  context.ui.registerPanel('own', ({element}) => context.settings.mount(element));
  context.ui.registerCommand('open', () => context.ui.openPanel('own'));
}`);
fs.writeFileSync(path.join(profile, 'plugins/enabled.json'), JSON.stringify({ directories: [source], enabled: ['ares.tools.settings', 'test.preferences'] }));
app.setPath('userData', profile);
delete process.env.ARES_SETTINGS; delete process.env.ARES_DISABLE_PLUGINS;
Object.assign(process.env, { ARES_PROFILE_DIR: profile, ARES_ROM: '', ARES_AUTOTEST: '', ARES_SILENT: '1', ARES_BACKGROUND_TEST: '1', ARES_DEFAULT_PLUGINS: '0', ARES_ENABLE_SERVICES: '0' });
const { preferences, plugins, sessions } = await import('../dist/main.js');
const until = async check => {
  for (let i = 0; i < 300; i++) {
    try { const result = await check(); if (result) return result; } catch {}
    await new Promise(resolve => setTimeout(resolve, 20));
  }
  throw new Error('plugin settings check timed out');
};
const file = path.join(profile, 'preferences.json'), id = 'plugin:test.preferences:display:accent';
const own = () => preferences.catalog().entries.filter(entry => entry.descriptor.id === id);
const local = () => plugins.catalog().plugins.find(plugin => plugin.manifest?.id === 'test.preferences');
const deadline = setTimeout(() => app.exit(1), 25000);
app.on('will-quit', () => { clearTimeout(deadline); fs.rmSync(temporary, { recursive: true, force: true }); });
app.whenReady().then(async () => {
  try {
    const window = await until(() => BrowserWindow.getAllWindows()[0]);
    const js = code => window.webContents.executeJavaScript(code);
    await until(() => js('window.settingsActivationDenied'));
    assert.equal(own().length, 0, 'activation stages metadata without publishing it');
    assert.equal(fs.existsSync(file), false, 'activation did not persist settings');
    assert.equal((await js('window.stagedPreferences')).entries.length, 1, 'register capability reads only own fields');
    await js('window.allowSettingsActivation = true; window.finishSettingsActivation()');
    await until(() => local()?.phase === 'active');
    await until(() => js(`!!document.querySelector('[data-plugin-command="ares.tools.settings:open"]')`));
    await js(`document.querySelector('[data-plugin-command="ares.tools.settings:open"]').click()`);
    const row = `[data-plugin-panel="ares.tools.settings:settings"] [data-setting="${id}"]`;
    await until(() => js(`!!document.querySelector(${JSON.stringify(row)})`));
    await js(`(() => { const row = document.querySelector(${JSON.stringify(row)}); row.querySelector('select').value = 'green'; row.querySelector('button').click(); })()`);
    await until(() => own()[0]?.value === 'green');
    assert.equal(JSON.parse(fs.readFileSync(file)).values[id], 'green');
    const identity = await sessions.request('session.status'), token = local().lease.token;
    await js('void (window.oldSettings = window.localSettings)');
    const replacement = await plugins.reload('test.preferences');
    const expectedToken = replacement.plugins.find(plugin => plugin.manifest?.id === 'test.preferences').candidate.lease.token;
    await until(() => local()?.phase === 'active' && local().lease.token === expectedToken && own().length === 1);
    assert.notEqual(expectedToken, token);
    assert.equal(own().length, 1); assert.equal(own()[0].value, 'green', 'reload retains profile values');
    assert.equal(await js(`window.oldSettings.set(${JSON.stringify(id)}, 'blue').then(() => false, () => true)`), true);
    assert.equal(JSON.parse(fs.readFileSync(file)).values[id], 'green');
    await plugins.enable('test.preferences', false);
    assert.equal(own().length, 0, 'lease revocation removes definitions');
    await plugins.enable('test.preferences', true);
    await until(() => local()?.phase === 'active' && own()[0]?.value === 'green');
    await js(`window.localSettings.reset(${JSON.stringify(id)})`);
    assert.equal(own()[0].value, 'blue');
    assert.equal((await sessions.request('session.status')).session, identity.session);
    assert.equal((await sessions.request('session.status')).epoch, identity.epoch);
    const beforeReload = fs.readFileSync(file, 'utf8');
    const beforeRendererToken = local().lease.token;
    window.webContents.once('did-finish-load', () => { void js('window.allowSettingsActivation = true; window.finishSettingsActivation?.()'); });
    window.webContents.reload();
    await until(() => local()?.phase === 'active' && local().lease.token !== beforeRendererToken && own().length === 1);
    await until(() => js(`!!document.querySelector('[data-plugin-panel="ares.tools.settings:settings"]')`));
    assert.equal(fs.readFileSync(file, 'utf8'), beforeReload, 'renderer reload never replays setting writes');
    assert.equal(await js("!!document.querySelector('#plugin-dock')"), false);
    console.log('PLUGIN_SETTINGS_PASS: staged local section, shared bundled form, profile persistence, reload/nonreplay, stale scopes, revoke/reenable/reset, durable workspace placement');
    app.quit();
  } catch (error) { console.error(error); app.exit(1); }
});
