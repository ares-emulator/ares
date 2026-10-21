export function activate(ctx) {
  ctx.ui.registerPanel('profile', ({ element }) => {
    const text = document.createElement('pre')
    text.textContent = `Verified profile: ${ctx.profile}\nSystem: ${ctx.system}\nSHA-256: ${ctx.game.sha256}`
    element.append(text)
  })
  ctx.ui.registerCommand('open', () => ctx.ui.openPanel('profile'))
}
