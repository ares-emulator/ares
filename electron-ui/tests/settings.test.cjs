const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const setup = async t => {
  const { SettingsStore } = await import('../dist/settings/store.js');
  const { presentationSettings } = await import('../dist/settings/builtins.js');
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-preferences-'));
  t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  return { SettingsStore, presentationSettings, file: path.join(directory, 'preferences.json') };
};
const entry = (store, id) => store.catalog().entries.find(e => e.descriptor.id === id);

test('provisioned presentation settings survive updates, restarts, and removal/readdition of descriptors', async t => {
  const { SettingsStore, presentationSettings, file } = await setup(t);
  fs.writeFileSync(file, JSON.stringify({ version: 1, values: { 'presentation.theme': 'green', 'future.setting': { untouched: true }, 'presentation.panelLeft': true } }));
  const store = new SettingsStore(file, presentationSettings);
  assert.equal(entry(store, 'presentation.theme').applied, 'green');
  assert.equal(entry(store, 'presentation.theme').source, 'profile');
  await Promise.all([store.set('presentation.theme', 'paper'), store.set('presentation.scale', 'integer')]);
  const removed = new SettingsStore(file, presentationSettings.filter(d => d.id !== 'presentation.panelLeft'));
  await removed.set('presentation.scanlines', true);
  assert.deepEqual(JSON.parse(fs.readFileSync(file)).values['future.setting'], { untouched: true });
  const restarted = new SettingsStore(file, presentationSettings);
  assert.equal(entry(restarted, 'presentation.theme').value, 'paper');
  assert.equal(entry(restarted, 'presentation.scale').value, 'integer');
  assert.equal(entry(restarted, 'presentation.scanlines').value, true);
  assert.equal(entry(restarted, 'presentation.panelLeft').value, true);
});

test('invalid requests cannot change persisted values; malformed and future-version files remain intact', async t => {
  const { SettingsStore, presentationSettings, file } = await setup(t);
  for (const content of ['{broken', '{"version":2,"values":{}}']) {
    fs.writeFileSync(file, content);
    const store = new SettingsStore(file, presentationSettings);
    assert.match(store.catalog().error, /Cannot read/);
    await assert.rejects(store.set('presentation.theme', 'paper'));
    assert.equal(fs.readFileSync(file, 'utf8'), content);
  }
  fs.writeFileSync(file, '{"version":1,"values":{"presentation.theme":"invalid"}}');
  const store = new SettingsStore(file, presentationSettings);
  assert.equal(entry(store, 'presentation.theme').value, 'dark');
  assert.match(store.catalog().error, /Invalid saved/);
  for (const [id, value] of [['missing', true], ['presentation.theme', 'invalid'], ['presentation.scanlines', 'false']])
    await assert.rejects(store.set(id, value), /invalid_setting_value/);
  await store.set('presentation.theme', 'paper');
  assert.equal(store.catalog().error, undefined);
});

test('saved values remain pending when live application fails or requires another ROM', async t => {
  const { SettingsStore, file } = await setup(t);
  const descriptors = [
    { id: 'audio.volume', title: 'Volume', section: 'Audio', type: 'number', default: 0.4, min: 0, max: 1, apply: 'live' },
    { id: 'core.fastBoot', title: 'Fast boot', section: 'Core', type: 'boolean', default: true, apply: 'next-rom' },
  ];
  let fail = true;
  const store = new SettingsStore(file, descriptors, () => {}, async () => { if (fail) throw new Error('busy'); });
  await store.set('audio.volume', 0.8);
  assert.equal(entry(store, 'audio.volume').value, 0.8);
  assert.equal(entry(store, 'audio.volume').applied, 0.4);
  assert.equal(entry(store, 'audio.volume').pending, true);
  assert.match(store.catalog().error, /Saved.*busy/);
  fail = false;
  await store.set('audio.volume', 0.8);
  assert.equal(entry(store, 'audio.volume').pending, false);
  await store.set('core.fastBoot', false);
  assert.equal(entry(store, 'core.fastBoot').pending, true);
  for (const value of [NaN, Infinity, -1, 2, '0.5']) await assert.rejects(store.set('audio.volume', value));
  const restarted = new SettingsStore(file, descriptors);
  assert.equal(entry(restarted, 'core.fastBoot').applied, false);
  const snapshot = restarted.catalog();
  snapshot.entries[0].descriptor.default = 99;
  assert.equal(entry(restarted, 'audio.volume').descriptor.default, 0.4);
});

test('startup acknowledgement uses the loaded snapshot rather than a newer saved selection', async t => {
  const { SettingsStore, file } = await setup(t);
  const descriptor = { id: 'paths.importBml', title: 'Imported settings', section: 'Paths', type: 'string', default: '', apply: 'next-rom' };
  const store = new SettingsStore(file, [descriptor]);
  await store.set(descriptor.id, '/tmp/first.bml');
  const loading = store.catalog();
  await store.set(descriptor.id, '/tmp/second.bml');
  store.markApplied(loading, 'next-rom');
  const current = entry(store, descriptor.id);
  assert.equal(current.value, '/tmp/second.bml');
  assert.equal(current.applied, '/tmp/first.bml');
  assert.equal(current.pending, true);
  await assert.rejects(store.set(descriptor.id, '/tmp/file\nInjected: value'));
});

test('explicit launch overrides win without rewriting the profile, until explicitly changed', async t => {
  const { SettingsStore, presentationSettings, file } = await setup(t);
  const original = JSON.stringify({ version: 1, values: { 'presentation.theme': 'paper' } });
  fs.writeFileSync(file, original);
  const store = new SettingsStore(file, presentationSettings, undefined, undefined, { 'presentation.theme': 'green' });
  assert.equal(entry(store, 'presentation.theme').source, 'launch');
  assert.equal(entry(store, 'presentation.theme').value, 'green');
  assert.equal(fs.readFileSync(file, 'utf8'), original);
  await store.set('presentation.theme', 'dark');
  assert.equal(entry(store, 'presentation.theme').source, 'profile');
  assert.equal(entry(store, 'presentation.theme').value, 'dark');
});

test('firmware discovery binds profile overrides above read-only imports and reset restores inheritance', async t => {
  const { SettingsStore, file } = await setup(t);
  const { firmwareDescriptors, importedValues, parseNativeSettingsInfo, nativeSettingsBml } = await import('../dist/settings/native.js');
  const info = parseNativeSettingsInfo(JSON.stringify({ firmware: [
    { system: 'Game Boy Advance', type: 'BIOS', region: 'World', key: 'GameBoyAdvance/Firmware/BIOS.World' },
  ], imported: { 'GameBoyAdvance/Firmware/BIOS.World': '/vendor/boot.rom' } }));
  const descriptors = firmwareDescriptors(info), id = descriptors[0].id;
  const store = new SettingsStore(file, []);
  store.register(descriptors);
  store.inherit(importedValues(info, descriptors));
  assert.equal(entry(store, id).value, '/vendor/boot.rom');
  assert.equal(entry(store, id).source, 'imported');
  await store.set(id, '/profile/custom.rom');
  assert.equal(entry(store, id).source, 'profile');
  assert.match(nativeSettingsBml(store.catalog()), /GameBoyAdvance\n  Firmware\n    BIOS.World: \/profile\/custom.rom\n/);
  store.inherit(importedValues({ ...info, imported: { 'GameBoyAdvance/Firmware/BIOS.World': '/vendor/new.rom' } }, descriptors));
  assert.equal(entry(store, id).value, '/profile/custom.rom');
  await store.reset(id);
  assert.equal(entry(store, id).value, '/vendor/new.rom');
  assert.equal(entry(store, id).source, 'imported');
  assert.equal(Object.hasOwn(JSON.parse(fs.readFileSync(file)).values, id), false);
  assert.throws(() => parseNativeSettingsInfo('{"firmware":[{"key":"../bad"}],"imported":{}}'), /invalid_native_settings/);
  assert.throws(() => store.register([descriptors[0], descriptors[0]]), /invalid_setting_descriptor/);
});
