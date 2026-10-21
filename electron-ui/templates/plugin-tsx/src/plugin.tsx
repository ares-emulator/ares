import type { PluginContext } from './sdk.js'
// A small DOM JSX factory: plugins choose their own framework and bundle it.
function h(
  tag: string,
  props: Record<string, string> | null,
  ...children: (Node | string)[]
): HTMLElement {
  const element = document.createElement(tag)
  for (const [key, value] of Object.entries(props || {}))
    element.setAttribute(key, value)
  element.append(...children)
  return element
}
declare global {
  namespace JSX {
    interface IntrinsicElements {
      [tag: string]: Record<string, string>
    }
  }
}
export function activate(ctx: PluginContext) {
  ctx.ui.registerPanel('hello', ({ element }) => {
    element.append(<p>Hello from {ctx.system}</p>)
  })
  ctx.ui.registerCommand('open', () => ctx.ui.openPanel('hello'))
}
