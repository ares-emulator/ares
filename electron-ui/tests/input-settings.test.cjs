const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const pad = (buttons = [], axes = [], mapping = 'standard') => ({ mapping, connected: true, axes, buttons: Array.from({ length: 32 }, (_, i) => ({ pressed: buttons.includes(i) })) });

test('default mappings preserve keyboard, N64 exceptions, analog axes and standard gamepad buttons', async () => {
  const { defaultInputSettings, mapInput } = await import('../dist/settings/input.js');
  const defaults = defaultInputSettings();
  const input = (keys, system = 'Game Boy', pads = []) => mapInput(defaults, new Set(keys), system, pads, true, true);
  assert.equal(input(['KeyZ', 'KeyX']).mask, (1 << 4) | (1 << 5));
  assert.equal(input(['KeyZ', 'KeyX'], 'Nintendo 64').mask, (1 << 6) | (1 << 4));
  assert.equal(input(['ShiftRight', 'Digit0']).mask, (1 << 8) | (1 << 25));
  assert.deepEqual(input(['KeyA', 'KeyS', 'KeyL', 'KeyI']), { mask: 0, x: -32767, y: 32767, rx: 32767, ry: -32767 });
  assert.equal(input(['KeyA', 'KeyD']).x, 0);
  const standard = input([], 'Game Boy', [pad([0, 8, 12], [0.5, 0.1, -1, 0])]);
  assert.deepEqual(standard, { mask: (1 << 4) | (1 << 8) | 1, x: 16384, y: 0, rx: -32767, ry: 0 });
});

test('custom input settings select devices and bindings, bound analog values, invert axes and release on defocus', async () => {
  const { defaultInputSettings, mapInput } = await import('../dist/settings/input.js');
  const settings = { ...defaultInputSettings(), 'input.keyboard.south': 'KeyN,KeyM', 'input.gamepad.device': '1', 'input.gamepad.south': 7, 'input.gamepad.leftX.axis': 2, 'input.gamepad.leftX.invert': true, 'input.gamepad.deadzone': 0.2 };
  const pads = [pad([0], [1, 0, 0, 0]), pad([7], [0.1, NaN, 2, 0], '')];
  const mapped = mapInput(settings, new Set(['KeyN']), 'Game Boy', pads, true, true);
  assert.equal(mapped.mask & (1 << 4), 1 << 4); assert.equal(mapped.x, -32767); assert.equal(mapped.y, 0);
  assert.equal(mapInput(settings, new Set(['KeyZ']), 'Game Boy', [], true, true).mask, 0);
  assert.deepEqual(mapInput(settings, new Set(['KeyN']), 'Game Boy', pads, false, true), { mask: 0, x: 0, y: 0, rx: 0, ry: 0 });
  settings['input.gamepad.focus'] = 'canvas';
  assert.equal(mapInput(settings, new Set(), 'Game Boy', pads, true, false).mask, 0);
  settings['input.gamepad.device'] = 'disabled';
  assert.equal(mapInput(settings, new Set(), 'Game Boy', pads, true, true).x, 0);
});

test('input mappings persist across startup and reject malformed key lists and fractional button indices', async t => {
  const { SettingsStore } = await import('../dist/settings/store.js');
  const { inputDescriptors, appliedInputSettings, mapInput } = await import('../dist/settings/input.js');
  const directory = fs.mkdtempSync('/tmp/ares-input-settings-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  const filename = path.join(directory, 'preferences.json');
  const store = new SettingsStore(filename, inputDescriptors);
  await store.set('input.keyboard.south', 'KeyN'); await store.set('input.gamepad.south', -1);
  for (const value of ['Key N', 'KeyN\nKeyM', 'KeyN,,KeyM', ',KeyN']) await assert.rejects(store.set('input.keyboard.south', value));
  await assert.rejects(store.set('input.gamepad.south', 0.5));
  await assert.rejects(store.set('input.gamepad.deadzone', 1));
  const restarted = new SettingsStore(filename, inputDescriptors);
  const settings = appliedInputSettings(restarted.catalog());
  assert.equal(mapInput(settings, new Set(['KeyN']), 'Game Boy', [], true, true).mask, 1 << 4);
  await restarted.reset('input.keyboard.south');
  assert.equal(appliedInputSettings(restarted.catalog())['input.keyboard.south'], 'KeyZ');
});
