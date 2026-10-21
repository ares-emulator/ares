const names = [
  'zero',
  'at',
  'v0',
  'v1',
  'a0',
  'a1',
  'a2',
  'a3',
  't0',
  't1',
  't2',
  't3',
  't4',
  't5',
  't6',
  't7',
  's0',
  's1',
  's2',
  's3',
  's4',
  's5',
  's6',
  's7',
  't8',
  't9',
  'k0',
  'k1',
  'gp',
  'sp',
  's8',
  'ra',
]
export function activate(ctx) {
  ctx.ui.registerPanel('stop', ({ element, onVisibility }) => {
    const title = document.createElement('h2')
    title.textContent = 'CPU stop and registers'
    const source = document.createElement('p'),
      stop = document.createElement('pre'),
      registers = document.createElement('pre')
    stop.dataset.debugStop = ''
    registers.dataset.debugRegisters = ''
    element.append(title, source, stop, registers)
    let pending = false
    async function refresh() {
      if (pending || ctx.signal.aborted) return
      pending = true
      try {
        const reply = await ctx.services.request('n64.debug.lastStop')
        if (!reply.ok) throw new Error(reply.error)
        const hit = reply.result
        let values = hit.registers
        if (!hit.valid) {
          const live = await ctx.services.request('n64.debug.registers')
          if (!live.ok) throw new Error(live.error)
          values = live.result
        }
        if (ctx.signal.aborted) return
        source.textContent = hit.valid
          ? 'Frozen registers from the last stop'
          : 'Live registers · no recorded stop'
        stop.textContent = JSON.stringify(hit, null, 2)
        if (!values) return
        registers.textContent = [
          `PC ${values.pc}   next ${values.next_pc}`,
          `HI ${values.hi}   LO ${values.lo}`,
          ...names.map((name, i) => `${name.padEnd(4)} ${values.gpr[i]}`),
        ].join('\n')
      } catch (error) {
        if (!ctx.signal.aborted) stop.textContent = String(error)
      } finally {
        pending = false
      }
    }
    onVisibility((visible) => {
      if (visible) void refresh()
    })
    ctx.scope.interval(() => void refresh(), 250)
  })
  ctx.ui.registerCommand('open', () => ctx.ui.openPanel('stop'))
}
