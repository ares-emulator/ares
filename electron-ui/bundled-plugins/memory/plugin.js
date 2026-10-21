export const stateVersion = 1
let state = { offset: '0x0', length: '64', hex: '', region: '' }
export function saveState() {
  return state
}
export function activate(ctx, restored) {
  state = restored ?? { offset: '0x0', length: '64', hex: '', region: '' }
  ctx.ui.registerPanel('memory', ({ element }) => {
    element.innerHTML =
      '<h2>Memory inspector</h2><button id="memory-refresh">Refresh regions</button><label>Region<select id="memory-region" aria-label="Memory region"></select></label><p id="memory-info">No regions loaded.</p><label>Region offset<input id="memory-offset" spellcheck="false"></label><label>Byte count<input id="memory-length" type="number" min="1" max="1024"></label><button id="memory-read" disabled>Read bytes</button><label>Hex bytes to write<textarea id="memory-bytes" maxlength="3072" spellcheck="false" rows="2"></textarea></label><button id="memory-write" disabled>Write bytes</button><pre id="memory-output" aria-live="polite"></pre>'
    const $ = (id) => element.querySelector('#memory-' + id),
      select = $('region'),
      info = $('info'),
      output = $('output'),
      read = $('read'),
      write = $('write')
    let regions = [],
      generation = 0
    $('offset').value = state.offset
    $('length').value = state.length
    $('bytes').value = state.hex
    const request = async (method, params) => {
      const r = await ctx.services.request(method, params)
      if (!r.ok) throw new Error(r.error)
      return r.result
    }
    const current = () => {
      const r = regions.find((r) => r.id === select.value)
      if (!r) throw new Error('Select a memory region.')
      return r
    }
    function describe() {
      const r = regions.find((r) => r.id === select.value)
      info.textContent = r
        ? `${r.size.toLocaleString()} bytes · region offsets · ${r.writable ? 'writable' : r.readable ? 'read only' : 'unavailable'}${r.unavailableReason ? ' · ' + r.unavailableReason : ''}`
        : 'No regions loaded.'
      read.disabled = !r?.readable
      write.disabled = !r?.writable
    }
    async function refresh() {
      const n = ++generation
      read.disabled = write.disabled = true
      try {
        const result = await request('memory.regions')
        if (ctx.signal.aborted || n !== generation) return
        regions = result.regions
        select.replaceChildren()
        for (const r of regions) {
          const option = document.createElement('option')
          option.value = r.id
          option.textContent = r.name
          select.append(option)
        }
        if (regions.some((r) => r.id === state.region))
          select.value = state.region
        describe()
      } catch (error) {
        if (!ctx.signal.aborted) info.textContent = String(error)
      }
    }
    const offset = () => {
      const value = $('offset').value.trim()
      if (
        !/^(?:0x[0-9a-f]+|[0-9]+)$/i.test(value) ||
        !Number.isSafeInteger(Number(value))
      )
        throw new Error('Enter a decimal or 0x hexadecimal region offset.')
      return Number(value)
    }
    async function show(fn) {
      const n = generation
      read.disabled = write.disabled = true
      try {
        const text = await fn()
        if (!ctx.signal.aborted && n === generation) output.textContent = text
      } catch (error) {
        if (!ctx.signal.aborted && n === generation)
          output.textContent = String(error)
      } finally {
        if (!ctx.signal.aborted && n === generation) describe()
      }
    }
    for (const [id, key] of [
      ['offset', 'offset'],
      ['length', 'length'],
      ['bytes', 'hex'],
    ])
      ctx.scope.listen($(id), 'input', () => {
        state[key] = $(id).value
      })
    ctx.scope.listen(select, 'change', () => {
      state.region = select.value
      ++generation
      output.textContent = ''
      describe()
    })
    ctx.scope.listen($('refresh'), 'click', () => void refresh())
    ctx.scope.listen(
      read,
      'click',
      () =>
        void show(async () => {
          const r = await request('memory.read', {
            region: current().id,
            offset: offset(),
            length: Number($('length').value),
          })
          return `Offset 0x${r.offset.toString(16)} (${r.length} bytes)\n${r.hex.match(/.{1,32}/g)?.join('\n')}`
        }),
    )
    ctx.scope.listen(
      write,
      'click',
      () =>
        void show(async () => {
          const r = await request('memory.write', {
            region: current().id,
            offset: offset(),
            hex: $('bytes').value.replace(/\s/g, ''),
          })
          return `Wrote ${r.written} bytes at region offset 0x${r.offset.toString(16)}.`
        }),
    )
    void refresh()
  })
  ctx.ui.registerCommand('open', () => ctx.ui.openPanel('memory'))
  ctx.ui.openPanel('memory')
}
