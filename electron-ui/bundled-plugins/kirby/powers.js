export const PROFILE = 'kirby64-us-rev0'
export const ROM_SHA256 =
  '2f579751d7ad2824dfd8a6141570306bfaeda1cff40139ba231c30b8591d681c'
export const ADDRESSES = {
  power: '0x8012e850',
  hud: '0x800d6e90',
  lastShown: '0x800d6ea8',
}
const singles = ['Fire', 'Stone', 'Ice', 'Needle', 'Bomb', 'Spark', 'Cutter']
export const POWERS = [
  { id: 0, name: 'None', elements: [] },
  ...singles.map((name, i) => ({
    id: i + 1,
    name,
    elements: [name.toLowerCase()],
  })),
]
for (let a = 0; a < singles.length; a++)
  for (let b = a; b < singles.length; b++)
    POWERS.push({
      id: POWERS.length,
      name: `${singles[a]} + ${singles[b]}`,
      elements: [singles[a].toLowerCase(), singles[b].toLowerCase()],
    })
export function powerWrites(id) {
  if (!Number.isInteger(id) || id < 0 || id >= POWERS.length)
    throw new Error('Power id must be 0–35')
  const hex = id.toString(16).padStart(8, '0')
  return {
    writes: [
      { address: ADDRESSES.power, view: 'cpu', hex },
      { address: ADDRESSES.hud, view: 'cpu', hex },
      // The documented HUD routine redraws only when lastShown differs from hud.
      {
        address: ADDRESSES.lastShown,
        view: 'cpu',
        hex: id === 0 ? '00000001' : '00000000',
      },
    ],
    verifyAfterFrames: 0,
    timeoutMs: 1000,
  }
}
export async function selectPower(ctx, id) {
  if (
    ctx.system !== 'Nintendo 64' ||
    ctx.profile !== PROFILE ||
    ctx.game?.sha256 !== ROM_SHA256
  )
    throw new Error('This tool requires verified Kirby 64 US revision 0')
  const reply = await ctx.services.request(
    'n64.memory.writeMany',
    powerWrites(id),
  )
  if (!reply.ok) throw new Error(reply.error)
  if (!reply.result.immediate_verification.matches.every(Boolean))
    throw new Error('Power write verification failed')
  return reply.result
}
