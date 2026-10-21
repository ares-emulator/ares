import { POWERS, ADDRESSES, selectPower } from './powers.js'
export function activate(ctx) {
  ctx.ui.registerPanel('powers', ({ element, onVisibility }) => {
    element.classList.add('kirby-powers')
    const heading = document.createElement('h2')
    heading.textContent = 'Kirby 64 · Powers'
    const note = document.createElement('p')
    note.textContent =
      'Choose a power while Kirby is controllable. Updates the ability and HUD; hat/body palette stays unchanged.'
    const current = document.createElement('p')
    current.className = 'power-current'
    current.textContent = 'Current power: reading…'
    const output = document.createElement('p')
    output.setAttribute('role', 'status')
    output.setAttribute('aria-live', 'polite')
    const grid = document.createElement('div')
    grid.className = 'power-grid'
    let busy = false,
      reading = false,
      selected = -1
    const buttons = []
    const mark = (id) => {
      selected = id
      current.textContent = `Current power: ${POWERS[id]?.name ?? 'unavailable'}`
      for (const button of buttons)
        button.setAttribute(
          'aria-pressed',
          String(Number(button.dataset.powerId) === id),
        )
    }
    const refresh = async () => {
      if (reading || busy || ctx.signal.aborted) return
      reading = true
      try {
        const r = await ctx.services.request('n64.memory.read', {
          address: ADDRESSES.power,
          view: 'cpu',
          length: 4,
        })
        if (!ctx.signal.aborted && r.ok) mark(parseInt(r.result.hex, 16))
      } catch (error) {
        if (!ctx.signal.aborted) output.textContent = String(error)
      } finally {
        reading = false
      }
    }
    for (const power of POWERS) {
      const button = document.createElement('button')
      button.type = 'button'
      button.dataset.powerId = String(power.id)
      button.title = `${power.name} · ID ${power.id}`
      button.setAttribute('aria-label', power.name)
      button.setAttribute('aria-pressed', 'false')
      const icons = document.createElement('span')
      icons.className = 'power-icons'
      icons.setAttribute('aria-hidden', 'true')
      if (!power.elements.length) icons.textContent = '—'
      for (const name of power.elements) {
        const img = document.createElement('img')
        img.src = new URL(`./icons/${name}.png`, import.meta.url).href
        img.alt = ''
        img.width = 24
        img.height = 32
        icons.append(img)
      }
      const label = document.createElement('span')
      label.textContent = power.name
      button.append(icons, label)
      buttons.push(button)
      grid.append(button)
      ctx.scope.listen(button, 'click', async () => {
        if (busy) return
        busy = true
        for (const b of buttons) b.disabled = true
        output.textContent = `Selecting ${power.name}…`
        try {
          await selectPower(ctx, power.id)
          if (!ctx.signal.aborted) {
            mark(power.id)
            output.textContent = `Selected ${power.name}.`
          }
        } catch (error) {
          if (!ctx.signal.aborted)
            output.textContent = String(error).includes('busy')
              ? 'The emulator is busy with another operation. Try again when it finishes.'
              : `Could not select power: ${String(error)}`
        } finally {
          busy = false
          if (!ctx.signal.aborted) for (const b of buttons) b.disabled = false
        }
      })
    }
    element.append(heading, note, current, grid, output)
    onVisibility((visible) => {
      if (visible) void refresh()
    })
    ctx.scope.interval(() => void refresh(), 300)
  })
  ctx.ui.registerCommand('open', () => ctx.ui.openPanel('powers'))
  for (const power of POWERS)
    ctx.ui.registerCommand(`power-${power.id}`, () =>
      selectPower(ctx, power.id).then(() => {}),
    )
}
