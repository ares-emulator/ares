const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
test('workspace trees, floating panels and shortcuts validate with strict complexity and geometry bounds', async () => {
  const { defaultLayout, validateLayout, leaves, transformArea, removeArea, validShortcut } = await import('../dist/workspace/model.js');
  const layout = defaultLayout();
  layout.root = { id: 'split', axis: 'x', ratio: 0.6, children: [layout.root, { id: 'memory', panel: 'plugin:ares.examples.memory:memory' }] };
  layout.floating.push({ id: 'floating', panel: 'plugin:ares.tools.settings:settings', mode: 'popup', open: false, x: 10, y: 20, width: 380, height: 400, shortcut: 'Mod+Shift+KeyM', interactive: true });
  assert.deepEqual(validateLayout(layout), layout);
  assert.equal(leaves(layout.root).length, 2);
  assert.equal(validShortcut('Mod+KeyO'), false, 'native menu shortcuts cannot be shadowed');
  assert.equal(validShortcut('KeyM'), false, 'plain gameplay keys cannot become popup shortcuts');
  for (const mutate of [
    copy => { copy.root.ratio = NaN; }, copy => { copy.root.children[1].id = 'video'; },
    copy => { copy.root.children[1].panel = 'video'; }, copy => { copy.floating[0].width = 0; },
    copy => { copy.floating.push({ ...copy.floating[0], id: 'other' }); },
    copy => { copy.extra = true; }, copy => { copy.floating[0].panel = '../outside'; },
  ]) { const copy = structuredClone(layout); mutate(copy); assert.throws(() => validateLayout(copy)); }
  const changed = transformArea(layout.root, 'memory', node => ({ id: node.id, panel: 'empty' }));
  assert.equal(leaves(changed)[1].panel, 'empty'); assert.equal(leaves(layout.root)[1].panel, 'plugin:ares.examples.memory:memory');
  assert.deepEqual(removeArea(layout.root, 'memory'), defaultLayout().root);
});
test('workspace provisioning survives reconstruction, while malformed files remain untouched', async t => {
  const { WorkspaceStore } = await import('../dist/workspace/store.js');
  const { defaultLayout } = await import('../dist/workspace/model.js');
  const directory = fs.mkdtempSync('/tmp/ares-workspace-store-'); t.after(() => fs.rmSync(directory, { recursive: true, force: true }));
  const filename = path.join(directory, 'workspace.json'), store = new WorkspaceStore(filename);
  assert.equal(fs.existsSync(filename), false, 'reading defaults never writes a profile');
  const layout = defaultLayout(); layout.root.panel = 'plugin:local.unavailable:panel';
  store.save(layout); assert.deepEqual(new WorkspaceStore(filename).load().layout, layout, 'missing plugins retain their placements');
  fs.writeFileSync(filename, '{broken');
  const broken = new WorkspaceStore(filename); assert.match(broken.load().error, /Cannot read workspace/);
  assert.throws(() => broken.save(defaultLayout())); assert.equal(fs.readFileSync(filename, 'utf8'), '{broken');
});
