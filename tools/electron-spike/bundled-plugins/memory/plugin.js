export function activate(ctx) {
  ctx.ui.registerPanel('memory', ({ element }) => {
    const select = document.createElement('select'),
      offset = document.createElement('input'),
      length = document.createElement('input'),
      hex = document.createElement('input'),
      read = document.createElement('button'),
      write = document.createElement('button'),
      output = document.createElement('pre')
    offset.value = '0'
    offset.setAttribute('aria-label', 'Memory offset')
    length.value = '16'
    length.setAttribute('aria-label', 'Read length')
    hex.placeholder = 'Hex bytes to write'
    read.textContent = 'Read'
    write.textContent = 'Write'
    element.append(select, offset, length, hex, read, write, output)
    const request = async (method, params) => {
      const reply = await ctx.services.request(method, params)
      if (!reply.ok) throw new Error(reply.error)
      return reply.result
    }
    const report = (fn) => async () => {
      try {
        const value = await fn()
        if (!ctx.signal.aborted)
          output.textContent = JSON.stringify(value, null, 2)
      } catch (e) {
        if (!ctx.signal.aborted) output.textContent = String(e)
      }
    }
    ctx.scope.listen(
      read,
      'click',
      report(() =>
        request('memory.read', {
          region: select.value,
          offset: Number(offset.value),
          length: Number(length.value),
        }),
      ),
    )
    ctx.scope.listen(
      write,
      'click',
      report(() =>
        request('memory.write', {
          region: select.value,
          offset: Number(offset.value),
          hex: hex.value,
        }),
      ),
    )
    void request('memory.regions')
      .then(({ regions }) => {
        if (ctx.signal.aborted) return
        for (const r of regions) {
          const option = document.createElement('option')
          option.value = r.id
          option.textContent = `${r.name || r.id} (${r.size} bytes)`
          select.append(option)
        }
      })
      .catch((e) => {
        if (!ctx.signal.aborted) output.textContent = String(e)
      })
  })
  ctx.ui.registerCommand('open', () => ctx.ui.openPanel('memory'))
}
