// Uses the same scoped settings API and form available to trusted local plugins.
export function activate(context) {
  context.ui.registerPanel('settings', ({ element }) => context.settings.mount(element));
  context.ui.registerCommand('open', () => context.ui.openPanel('settings'));
}
