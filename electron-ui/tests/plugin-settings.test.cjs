const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const manifest = { id: 'test.preferences', name: 'Preferences', version: '1.0.0', apiVersion: 1, entry: 'plugin.js', capabilities: ['settings.register'], contributes: { settings: [{ id: 'display', title: 'Display' }] } };
const descriptor = { id: 'accent', title: 'Accent', type: 'enum', options: ['green', 'blue'], default: 'blue' };

test('plugin settings stage without publishing or writing, survive reload, and isolate declared namespaces', async t => {
  const { SettingsStore } = await import('../dist/settings/store.js');
  const { PluginSettingsService } = await import('../dist/settings/plugins.js');
  const { validateManifest } = await import('../dist/plugins/manifest.js');
  validateManifest(manifest);
  const directory = fs.mkdtempSync('/tmp/ares-plugin-settings-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  const filename = path.join(directory, 'preferences.json');
  const store = new SettingsStore(filename, []);
  const service = new PluginSettingsService(store, (id, value, reset, guard) => reset ? store.reset(id, guard) : store.set(id, value, guard), async () => { throw new Error('no file dialogs'); });
  const request = (token, active, data, m = manifest) => service.request(token, m, active, data, () => {});
  const id = 'plugin:test.preferences:display:accent';
  const staged = await request('first', false, { operation: 'register', section: 'display', descriptors: [descriptor] });
  assert.equal(staged.entries[0].descriptor.id, id);
  assert.equal(store.catalog().entries.length, 0, 'activation metadata is private until committed');
  assert.equal(fs.existsSync(filename), false, 'registration never writes defaults');
  await assert.rejects(request('first', false, { operation: 'set', id, value: 'green' }), /plugin_not_ready/);
  service.finish('first');
  await request('first', true, { operation: 'set', id, value: 'green' });
  await assert.rejects(request('first', true, { operation: 'set', id: 'audio.volume', value: 1 }), /settings_write_denied/);
  await assert.rejects(request('first', true, { operation: 'set', id: 'plugin:other:display:accent', value: 'green' }), /settings_write_denied/);
  service.revoke('first');
  assert.equal(store.catalog().entries.length, 0);
  assert.equal(JSON.parse(fs.readFileSync(filename)).values[id], 'green');
  const restored = await request('second', false, { operation: 'register', section: 'display', descriptors: [descriptor] });
  assert.equal(restored.entries[0].value, 'green');
  service.finish('second');
  await request('second', true, { operation: 'reset', id });
  assert.equal(store.catalog().entries[0].value, 'blue');
  const global = { ...manifest, id: 'test.manager', capabilities: ['settings.read', 'settings.write'] };
  await request('manager', true, { operation: 'set', id, value: 'green' }, global);
  assert.equal(store.catalog().entries[0].value, 'green');
  await assert.rejects(request('readonly', true, { operation: 'set', id, value: 'blue' }, { ...global, capabilities: ['settings.read'] }), /settings_write_denied/);
});

test('plugin descriptor validation rejects native bindings, malformed schemas and undeclared sections atomically', async t => {
  const { SettingsStore } = await import('../dist/settings/store.js');
  const { PluginSettingsService } = await import('../dist/settings/plugins.js');
  const directory = fs.mkdtempSync('/tmp/ares-plugin-settings-invalid-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  const store = new SettingsStore(path.join(directory, 'preferences.json'), []);
  const service = new PluginSettingsService(store, async () => { throw new Error('no writes'); }, async () => { throw new Error('no dialogs'); });
  for (const patch of [{ nativeKey: 'Boot/Fast' }, { file: { extensions: [] } }, { section: 'Firmware' }, { id: '../outside' }, { options: [] }, { apply: 'next-rom' }]) {
    await assert.rejects(service.request('invalid', manifest, false, { operation: 'register', section: 'display', descriptors: [descriptor, { ...descriptor, id: 'another', ...patch }] }, () => {}), /invalid_settings_descriptor/);
    assert.equal(store.catalog().entries.length, 0);
  }
  await assert.rejects(service.request('invalid', manifest, false, { operation: 'register', section: 'undeclared', descriptors: [descriptor] }, () => {}), /settings_registration_denied/);
});

test('revoked queued setting writes cannot mutate a profile', async t => {
  const { SettingsStore } = await import('../dist/settings/store.js');
  const directory = fs.mkdtempSync('/tmp/ares-plugin-settings-guard-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  let release;
  const busy = new Promise(resolve => { release = resolve; });
  const store = new SettingsStore(path.join(directory, 'preferences.json'), [{ id: 'value', title: 'Value', section: 'Test', type: 'number', min: 0, max: 10, default: 0, apply: 'live' }], () => {}, async () => busy);
  const first = store.set('value', 1);
  await new Promise(resolve => setImmediate(resolve));
  let active = true;
  const second = store.set('value', 2, () => { if (!active) throw new Error('stale_plugin_scope'); });
  active = false; release(); await first;
  await assert.rejects(second, /stale_plugin_scope/);
  assert.equal(store.catalog().entries[0].value, 1);
});

test('status polling cannot issue an intermediate plugin lease while replacement awaits native release', async t => {
  const { PluginRegistry } = await import('../dist/plugins/registry.js');
  const directory = fs.mkdtempSync('/tmp/ares-plugin-settings-reload-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  const source = path.join(directory, 'source'); fs.mkdirSync(source);
  fs.writeFileSync(path.join(source, 'manifest.json'), JSON.stringify(manifest));
  fs.writeFileSync(path.join(source, 'plugin.js'), 'export function activate() {}');
  const identity = { session: 'settings-reload', epoch: 1 };
  let blocked = false, began = false, release;
  const gate = new Promise(resolve => { release = resolve; });
  const client = { request: async () => {
    if (blocked) { began = true; await gate; }
    return { protocol: 1, id: 'release', ...identity, ok: true, result: { released: true } };
  } };
  const registry = new PluginRegistry(path.join(directory, 'bundled'), path.join(directory, 'profile'), client, () => identity);
  try {
    await registry.initialize([source], [manifest.id]);
    const status = { protocol: 1, id: 'status', ...identity, ok: true, result: { system: 'Synthetic', synthetic: true, paused: true, capabilities: [] } };
    registry.observe(status);
    const first = registry.attach().plugins.find(p => p.manifest?.id === manifest.id);
    registry.finish(first.lease.token);
    const staged = await registry.reload(manifest.id), token = staged.plugins.find(p => p.manifest?.id === manifest.id).candidate.lease.token;
    blocked = true;
    const replacement = registry.begin(token);
    while (!began) await new Promise(resolve => setImmediate(resolve));
    registry.observe(status);
    const pending = registry.catalog().plugins.find(p => p.manifest?.id === manifest.id);
    assert.equal(pending.lease, undefined); assert.equal(pending.phase, 'activating');
    blocked = false; release();
    assert.equal((await replacement).lease.token, token);
    registry.finish(token);
    assert.equal(registry.catalog().plugins.find(p => p.manifest?.id === manifest.id).lease.token, token);
  } finally { blocked = false; release(); await registry.close(); }
});
