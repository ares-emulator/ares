const { test } = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs/promises')
const path = require('node:path')
const os = require('node:os')
test('immutable plugin artifacts build TSX, preserve imports/assets and ignore generated outputs', async (t) => {
  const { buildArtifact } = await import('../dist/plugins/artifacts.js')
  const dir = await fs.mkdtemp(path.join(os.tmpdir(), 'ares-artifacts-'))
  t.after(() => fs.rm(dir, { recursive: true, force: true }))
  const source = path.join(dir, 'source'),
    cache = path.join(dir, 'cache')
  await fs.mkdir(path.join(source, 'src'), { recursive: true })
  const manifest = {
    id: 'test.tsx',
    name: 'TSX',
    version: '1.0.0',
    apiVersion: 1,
    entry: 'out/plugin.js',
    styles: 'theme.css',
    build: { entry: 'src/plugin.tsx', jsxFactory: 'h' },
    capabilities: [],
    contributes: {},
  }
  await fs.writeFile(
    path.join(source, 'manifest.json'),
    JSON.stringify(manifest),
  )
  await fs.writeFile(
    path.join(source, 'src/label.ts'),
    "export const label='Original'",
  )
  await fs.writeFile(
    path.join(source, 'src/plugin.tsx'),
    "import {label} from './label';const h=(tag,props,...children)=>({tag,children});export function activate(){return <p>{label}</p>}",
  )
  await fs.writeFile(path.join(source, 'theme.css'), 'p { color: red; }')
  const first = await buildArtifact(source, cache),
    bytes = await fs.readFile(path.join(first.root, 'out/plugin.js'), 'utf8')
  assert.match(bytes, /Original/)
  assert.doesNotMatch(bytes, /<p>/)
  await fs.mkdir(path.join(source, 'out'))
  await fs.writeFile(
    path.join(source, 'out/plugin.js'),
    'broken generated output',
  )
  assert.equal((await buildArtifact(source, cache)).hash, first.hash)
  await fs.writeFile(
    path.join(source, 'src/label.ts'),
    "export const label='Replacement'",
  )
  const next = await buildArtifact(source, cache)
  assert.notEqual(next.hash, first.hash)
  assert.equal(
    await fs.readFile(path.join(first.root, 'out/plugin.js'), 'utf8'),
    bytes,
  )
  assert.match(
    await fs.readFile(path.join(next.root, 'out/plugin.js'), 'utf8'),
    /Replacement/,
  )
  await fs.writeFile(path.join(source, 'src/plugin.tsx'), 'invalid syntax {')
  await assert.rejects(buildArtifact(source, cache))
  assert.equal(
    await fs.readFile(path.join(first.root, 'out/plugin.js'), 'utf8'),
    bytes,
  )
  await fs.writeFile(
    path.join(source, 'src/plugin.tsx'),
    'export function activate(){}',
  )
  await fs.writeFile(path.join(source, 'theme.css'), 'p { color: "unterminated')
  await assert.rejects(buildArtifact(source, cache))
  assert.equal(
    await fs.readFile(path.join(first.root, 'out/plugin.js'), 'utf8'),
    bytes,
  )
  await fs.writeFile(path.join(dir,'external.json'),JSON.stringify(manifest))
  await fs.unlink(path.join(source,'manifest.json'))
  await fs.symlink(path.join(dir,'external.json'),path.join(source,'manifest.json'))
  await assert.rejects(buildArtifact(source,cache),/manifest escapes/)
  assert.equal(
    (await fs.readdir(cache)).some(
      (n) => n.startsWith('.build-') || n.startsWith('.publish-'),
    ),
    false,
  )
})
test('candidate tokens cannot mutate; rejection preserves leases and owner-release failure blocks replacement', async (t) => {
  const { PluginRegistry } = await import('../dist/plugins/registry.js')
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'ares-candidate-'))
  t.after(() => fs.rm(root, { recursive: true, force: true }))
  const source = path.join(root, 'bundled/plugin')
  await fs.mkdir(source, { recursive: true })
  const manifest = {
    id: 'test.candidate',
    name: 'Candidate',
    version: '1.0.0',
    apiVersion: 1,
    entry: 'plugin.js',
    capabilities: ['memory.write'],
    contributes: {},
  }
  await fs.writeFile(
    path.join(source, 'manifest.json'),
    JSON.stringify(manifest),
  )
  await fs.writeFile(
    path.join(source, 'plugin.js'),
    'export function activate(){}',
  )
  let failRelease = false
  const calls = [],
    identity = { session: 'candidate', epoch: 1 },
    client = {
      request: async (method, id, params, owner) => {
        calls.push({ method, owner })
        if (failRelease && method === 'execution.release')
          throw new Error('release transport failed')
        return { ok: true }
      },
    }
  const registry = new PluginRegistry(
    path.join(root, 'bundled'),
    path.join(root, 'data'),
    client,
    () => identity,
  )
  t.after(() => registry.close())
  await registry.initialize()
  registry.attach()
  registry.observe({
    ok: true,
    ...identity,
    result: {
      system: 'Synthetic',
      paused: true,
      synthetic: true,
      capabilities: [],
    },
  })
  await registry.enable(manifest.id, true)
  const old = registry.catalog().plugins[0].lease.token
  registry.finish(old)
  await registry.reload(manifest.id)
  let candidate = registry.catalog().plugins[0].candidate
  await assert.rejects(
    registry.request(candidate.lease.token, 'memory.write', {}),
    /stale_plugin_scope/,
  )
  registry.reject(candidate.lease.token, 'invalid module')
  assert.equal(registry.catalog().plugins[0].lease.token, old)
  await registry.reload(manifest.id)
  candidate = registry.catalog().plugins[0].candidate
  failRelease = true
  await assert.rejects(
    registry.begin(candidate.lease.token),
    /release transport failed/,
  )
  assert.equal(registry.catalog().plugins[0].lease, undefined)
  assert.match(registry.catalog().plugins[0].error, /replacement blocked/)
  assert.equal(calls.filter((c) => c.method === 'memory.write').length, 0)
})
