const { test } = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const os = require('node:os')
const path = require('node:path')
const { spawn } = require('node:child_process')
const { once } = require('node:events')
const { randomUUID, createHash } = require('node:crypto')
const source = process.env.ARES_PLUGIN_TEST_ROM
test(
  'verified Kirby profile survives rename and byte-order normalization; another ROM cannot activate it',
  { skip: !source, timeout: 30000 },
  async () => {
    const { NativeSessionClient } =
      await import('../dist/services/native-client.js')
    const { PluginRegistry } = await import('../dist/plugins/registry.js')
    const root = fs.mkdtempSync(path.join(os.tmpdir(), 'ares-plugin-game-'))
    const original = fs.readFileSync(source),
      hash = (bytes) => createHash('sha256').update(bytes).digest('hex')
    const originalHash = hash(original)
    try {
      for (const swapped of [false, true]) {
        const bytes = Buffer.from(original)
        if (swapped) bytes.swap16()
        const rom = path.join(
          root,
          swapped ? 'unrelated-name.v64' : 'unrelated-name.z64',
        )
        fs.writeFileSync(rom, bytes)
        const settings = path.join(root, 'settings.bml')
        fs.writeFileSync(settings, '')
        const identity = { session: randomUUID(), epoch: 1 }
        const child = spawn(
          process.env.ARES_HOST ||
            path.resolve(
              __dirname,
              '../../build-electron-make/bin/ares-electron-host',
            ),
          [
            '--rom',
            rom,
            '--system',
            'Nintendo 64',
            '--settings',
            settings,
            '--silent',
            '--paused',
            '--save-dir',
            path.join(root, 'saves'),
            '--control-session',
            identity.session,
          ],
          { stdio: ['pipe', 'pipe', 'pipe', 'pipe'] },
        )
        const closed = once(child, 'close')
        child.stdout.resume()
        child.stderr.resume()
        const client = new NativeSessionClient(
          child.stdin,
          child.stdio[3],
          identity,
          () => {},
        )
        try {
          const registry = new PluginRegistry(
            path.resolve(__dirname, '../bundled-plugins'),
            path.join(root, 'data'),
            client,
            () => client.identity,
          )
          await registry.initialize()
          registry.attach()
          const status = await client.request('session.status')
          assert.equal(status.result.game.sha256, originalHash)
          registry.observe(status)
          await registry.enable('ares.examples.kirby', true)
          const plugin = registry
            .catalog()
            .plugins.find((p) => p.manifest.id === 'ares.examples.kirby')
          assert.equal(plugin.state, 'active')
          assert.equal(plugin.lease.profile, 'kirby64-us-rev0')
          const token = plugin.lease.token
        registry.finish(token)
          registry.observe({
            ...status,
            result: {
              ...status.result,
              game: { ...status.result.game, sha256: '0'.repeat(64) },
            },
          })
          assert.equal(
            registry
              .catalog()
              .plugins.find((p) => p.manifest.id === 'ares.examples.kirby')
              .state,
            'waiting',
          )
          await assert.rejects(
            registry.request(token, 'session.status'),
            /stale_plugin_scope/,
          )
          registry.disconnect()
        } finally {
          client.close()
          child.stdin.end('quit\n')
          await closed
        }
      }
      assert.equal(hash(fs.readFileSync(source)), originalHash)
    } finally {
      fs.rmSync(root, { recursive: true, force: true })
    }
  },
)
