import { mountSettingsForm } from '../settings/form.js';
import { shortcutFromEvent } from './model.js';
import { Workspace } from './runtime.js';
const element = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;
const dialog = element<HTMLDialogElement>('settings-dialog');
let toastTimer: ReturnType<typeof setTimeout> | undefined;
export function notify(error: unknown): void {
  const status = element<HTMLOutputElement>(dialog.open ? 'settings-feedback' : 'status'); status.textContent = String(error); status.hidden = false;
  clearTimeout(toastTimer); toastTimer = setTimeout(() => { element('status').hidden = true; element('settings-feedback').hidden = true; }, 8000);
}
let workspace: Workspace;
export function showSettings(tab = 'general'): void {
  window.dispatchEvent(new Event('ares-input-release'));
  for (const page of document.querySelectorAll<HTMLElement>('[data-page]')) page.hidden = page.dataset.page !== tab;
  for (const button of document.querySelectorAll<HTMLElement>('[data-tab]')) button.setAttribute('aria-selected', String(button.dataset.tab === tab));
  if (!dialog.open) dialog.showModal();
}
export function closeSettings(): void { dialog.close(); element<HTMLCanvasElement>('screen').focus({ preventScroll: true }); }
element('close-settings').onclick = closeSettings;
dialog.addEventListener('cancel', event => { event.preventDefault(); closeSettings(); });
for (const button of document.querySelectorAll<HTMLElement>('[data-tab]')) button.onclick = () => showSettings(button.dataset.tab);
mountSettingsForm(element('settings-general'), window.ares.preferences, entry => ['Presentation', 'Audio', 'Video'].includes(entry.descriptor.section));
mountSettingsForm(element('settings-input'), window.ares.preferences, entry => entry.descriptor.id.startsWith('input.'));
mountSettingsForm(element('settings-firmware'), window.ares.preferences, entry => entry.descriptor.id.startsWith('firmware.') || entry.descriptor.id === 'paths.importBml');
mountSettingsForm(element('settings-core'), window.ares.preferences, entry => !!entry.descriptor.nativeKey && !['Presentation', 'Audio', 'Video', 'Firmware', 'Paths'].includes(entry.descriptor.section));
mountSettingsForm(element('settings-plugins'), window.ares.preferences, entry => entry.descriptor.id.startsWith('plugin:'));
function updateWorkspaceSettings(): void {
  if (!workspace) return;
  element('edit-layout').textContent = workspace.isEditing() ? 'Finish customizing' : 'Customize layout';
  const select = element<HTMLSelectElement>('floating-panel'), selected = select.value;
  select.replaceChildren();
  for (const panel of workspace.panels()) { const option = document.createElement('option'); option.value = panel.key; option.textContent = panel.title; select.append(option); }
  if ([...select.options].some(option => option.value === selected)) select.value = selected;
  element<HTMLButtonElement>('add-floating').disabled = !select.value;
  const list = element('floating-settings'); list.replaceChildren();
  for (const item of workspace.snapshot().floating) {
    const card = document.createElement('section'); card.className = 'floating-card'; card.dataset.floatingSettings = item.id;
    const title = document.createElement('h2'); title.textContent = workspace.panels().find(panel => panel.key === item.panel)?.title ?? 'Unavailable panel';
    const mode = document.createElement('select');
    for (const value of ['popup', 'overlay'] as const) { const option = document.createElement('option'); option.value = value; option.textContent = value === 'popup' ? 'Pop-up window' : 'Overlay'; mode.append(option); }
    mode.value = item.mode; mode.onchange = () => workspace.updateFloating(item.id, { mode: mode.value as 'popup' | 'overlay' });
    const shortcutLabel = document.createElement('label'); shortcutLabel.textContent = 'Shortcut';
    const shortcut = document.createElement('input'); shortcut.readOnly = true; shortcut.value = item.shortcut; shortcut.placeholder = 'No shortcut';
    const record = document.createElement('button'); record.textContent = 'Record shortcut';
    let recording = false;
    record.onclick = () => { recording = true; shortcut.value = 'Press a shortcut…'; shortcut.focus(); };
    shortcut.onkeydown = event => {
      if (!recording) return;
      event.preventDefault(); event.stopPropagation();
      if (event.code === 'Escape') { recording = false; shortcut.value = item.shortcut; return; }
      const key = shortcutFromEvent(event);
      if (key) { recording = false; workspace.updateFloating(item.id, { shortcut: key }); }
    };
    shortcut.onblur = () => { recording = false; shortcut.value = item.shortcut; };
    const clear = document.createElement('button'); clear.textContent = 'Clear shortcut'; clear.onclick = () => workspace.updateFloating(item.id, { shortcut: '' });
    shortcutLabel.append(shortcut);
    const interaction = document.createElement('label'), checkbox = document.createElement('input'); checkbox.type = 'checkbox'; checkbox.checked = item.interactive; checkbox.onchange = () => workspace.updateFloating(item.id, { interactive: checkbox.checked });
    interaction.append(checkbox, ' Accept mouse input');
    const toggle = document.createElement('button'); toggle.textContent = item.open ? 'Hide' : 'Show'; toggle.onclick = () => workspace.toggle(item.id);
    const remove = document.createElement('button'); remove.textContent = 'Remove'; remove.onclick = () => workspace.removeFloating(item.id);
    card.append(title, mode, shortcutLabel, record, clear, interaction, toggle, remove); list.append(card);
  }
}
workspace = new Workspace(element('workspace'), element('floating-layer'), element('viewport'), updateWorkspaceSettings, notify);
export { workspace };
element('edit-layout').onclick = () => { workspace.toggleEditing(); closeSettings(); };
element('video-only').onclick = () => workspace.reset();
element('add-floating').onclick = () => { workspace.addFloating(element<HTMLSelectElement>('floating-panel').value, element<HTMLSelectElement>('floating-mode').value as 'popup' | 'overlay'); };
let actionHandler: (action: 'open' | 'pause' | 'reset') => void = () => {};
export function onSessionAction(callback: typeof actionHandler): void { actionHandler = callback; }
function action(name: 'settings' | 'open' | 'pause' | 'reset' | 'edit-workspace' | 'reset-workspace'): void {
  if (name === 'settings') { if (dialog.open) closeSettings(); else showSettings(); }
  else if (name === 'edit-workspace') { if (dialog.open) dialog.close(); workspace.toggleEditing(); }
  else if (name === 'reset-workspace') workspace.reset();
  else actionHandler(name);
}
window.ares.onAction(action);
window.addEventListener('keydown', event => {
  if (!(event.metaKey || event.ctrlKey)) return;
  const name = event.code === 'Comma' ? 'settings' : event.code === 'KeyL' && event.shiftKey ? 'edit-workspace' : undefined;
  if (name) { event.preventDefault(); event.stopImmediatePropagation(); action(name); }
}, true);
export function blocksGameInput(): boolean { return dialog.open || workspace.isEditing(); }
