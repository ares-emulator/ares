import { app, BrowserWindow } from 'electron';
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const temporary = fs.mkdtempSync('/tmp/ares-settings-ui-');
const profile = path.join(temporary, 'profile');
fs.mkdirSync(profile);
const file = path.join(profile, 'preferences.json');
fs.writeFileSync(file, JSON.stringify({ version: 1, values: {
  'presentation.theme': 'green', 'presentation.scale': 'integer',
  'presentation.scanlines': true, 'presentation.panelLeft': true,
} }));
app.setPath('userData', profile);
Object.assign(process.env, { ARES_PROFILE_DIR: profile, ARES_ROM: '', ARES_AUTOTEST: '', ARES_SILENT: '1', ARES_BACKGROUND_TEST: '1', ARES_DISABLE_PLUGINS: '1', ARES_ENABLE_SERVICES: '0' });
await import('../dist/main.js');
const until = async check => {
  for (let i = 0; i < 300; i++) {
    try { const result = await check(); if (result) return result; } catch {}
    await new Promise(resolve => setTimeout(resolve, 20));
  }
  throw new Error('settings UI check timed out');
};
const deadline = setTimeout(() => app.exit(1), 25000);
app.on('will-quit', () => { clearTimeout(deadline); fs.rmSync(temporary, { recursive: true, force: true }); });
app.whenReady().then(async () => {
  try {
    const window = await until(() => BrowserWindow.getAllWindows()[0]);
    const js = code => window.webContents.executeJavaScript(code);
    await until(() => js("document.body.dataset.theme === 'green'"));
    assert.equal(await js("document.querySelector('#scale').value"), 'integer');
    assert.equal(await js("document.querySelector('#scanlines').checked"), true);
    assert.equal(await js("document.body.classList.contains('panel-left')"), true);
    await js("document.querySelector('#theme').value = 'paper'; document.querySelector('#theme').dispatchEvent(new Event('change'))");
    await until(() => js("document.body.dataset.theme === 'paper'"));
    assert.equal(JSON.parse(fs.readFileSync(file)).values['presentation.theme'], 'paper');
    assert.equal(await js("localStorage.getItem('ares-workbench')"), null);
    window.webContents.reload();
    await until(() => js("document.body.dataset.theme === 'paper' && !!window.ares"));
    await js("window.ares.preferences.set('presentation.scanlines', false)");
    await until(() => js("!document.querySelector('#scanlines').checked"));
    assert.equal(JSON.parse(fs.readFileSync(file)).values['presentation.scanlines'], false);
    assert.equal(await js("window.ares.preferences.set('presentation.theme', 'bad').then(() => false, () => true)"), true);
    const imported = path.join(temporary, 'imported.bml');
    const original = 'Boot\n  Fast: false\n';
    fs.writeFileSync(imported, original);
    await js(`window.ares.preferences.set('paths.importBml', ${JSON.stringify(imported)})`);
    assert.equal(await js("window.ares.preferences.catalog().then(s => s.entries.find(e => e.descriptor.id === 'paths.importBml').pending)"), true);
    await js("window.ares.command({type: 'synthetic'})");
    await until(() => js("window.ares.preferences.catalog().then(s => !s.entries.find(e => e.descriptor.id === 'paths.importBml').pending)"));
    assert.equal(fs.readFileSync(imported, 'utf8'), original);
    assert.equal(JSON.parse(fs.readFileSync(file)).values['paths.importBml'], imported);

    console.log('SETTINGS_UI_PASS: provisioned values applied; controls and bridge persist updates; reload retains settings; invalid values rejected; no browser-storage settings');
    app.quit();
  } catch (error) { console.error(error); app.exit(1); }
});
