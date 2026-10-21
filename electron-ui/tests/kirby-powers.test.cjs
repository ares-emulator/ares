const { test } = require('node:test')
const assert = require('node:assert/strict')
const fs = require('node:fs')
const path = require('node:path')
test('Kirby powers follow the verified 0–35 mapping and batch only documented CPU-view words', async () => {
  const { POWERS, powerWrites, selectPower, ROM_SHA256, PROFILE } =
    await import('../bundled-plugins/kirby/powers.js')
  assert.equal(POWERS.length, 36)
  for (const [id, name] of [
    [0, 'None'],
    [1, 'Fire'],
    [7, 'Cutter'],
    [14, 'Fire + Cutter'],
    [16, 'Stone + Ice'],
    [21, 'Ice + Ice'],
    [35, 'Cutter + Cutter'],
  ])
    assert.equal(POWERS[id].name, name)
  assert.equal(new Set(POWERS.map((p) => p.name)).size, 36)
  for (const p of POWERS) {
    assert.equal(p.id, POWERS.indexOf(p))
    for (const icon of p.elements)
      assert.ok(
        fs.existsSync(
          path.join(__dirname, '../bundled-plugins/kirby/icons', icon + '.png'),
        ),
      )
    const batch = powerWrites(p.id)
    assert.equal(batch.verifyAfterFrames, 0)
    assert.deepEqual(
      batch.writes.map((w) => w.address),
      ['0x8012e850', '0x800d6e90', '0x800d6ea8'],
    )
    assert.ok(batch.writes.every((w) => w.view === 'cpu'))
    assert.equal(batch.writes[0].hex, p.id.toString(16).padStart(8, '0'))
    assert.equal(batch.writes[1].hex, batch.writes[0].hex)
    assert.notEqual(batch.writes[2].hex, batch.writes[0].hex)
  }
  for (const bad of [-1, 36, 1.5, '14', NaN])
    assert.throws(() => powerWrites(bad))
  let requests = 0
  const ctx = {
    system: 'Nintendo 64',
    profile: PROFILE,
    game: { sha256: ROM_SHA256 },
    services: {
      request: async (method, params) => {
        requests++
        assert.equal(method, 'n64.memory.writeMany')
        return {
          ok: true,
          result: { immediate_verification: { matches: [true, true, true] } },
        }
      },
    },
  }
  await selectPower(ctx, 14)
  assert.equal(requests, 1)
  await assert.rejects(
    selectPower({ ...ctx, game: { sha256: '0'.repeat(64) } }, 14),
    /verified/,
  )
  assert.equal(requests, 1)
  await assert.rejects(
    selectPower(
      {
        ...ctx,
        services: { request: async () => ({ ok: false, error: 'busy' }) },
      },
      14,
    ),
    /busy/,
  )
})
