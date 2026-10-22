import { defaultLayout, leaves, removeArea, shortcutFromEvent, transformArea, validateLayout, type Area, type FloatingPanel, type WorkspaceLayout } from './model.js';
export type Cleanup = () => void | Promise<void>;
export interface PanelMount { instanceId: string; element: HTMLElement; onVisibility(callback: (visible: boolean) => void): Cleanup }
interface Definition { title: string; mount(context: PanelMount): void | Cleanup; owner: symbol }
interface View { panel: string; element: HTMLElement; visible: boolean; visibility: Set<(visible: boolean) => void>; cleanup?: Cleanup }
const button = (text: string, title: string, action: () => void) => {
  const element = document.createElement('button'); element.textContent = text; element.title = title; element.setAttribute('aria-label', title); element.onclick = action; return element;
};
/** Placement survives plugin leases. Instances and effects do not. */
export class Workspace {
  private layout = defaultLayout();
  private confirmed = defaultLayout();
  private loaded = false;
  private revision = 0;
  private queue: Promise<unknown> = Promise.resolve();
  private definitions = new Map<string, Definition>();
  private views = new Map<string, View>();
  private known = new Map<string, string>();
  private editing = false;
  private focusedArea = 'video';
  private generation = 0;
  private top = 20;
  private ready: Promise<void>;
  constructor(private root: HTMLElement, private floatingRoot: HTMLElement, private video: HTMLElement,
    private changed: () => void, private error: (error: unknown) => void) {
    this.render();
    this.ready = window.ares.workspace.load().then(snapshot => {
      this.layout = snapshot.layout; this.confirmed = structuredClone(snapshot.layout); this.loaded = true; this.render();
      if (snapshot.error) error(snapshot.error);
    }).catch(error).finally(() => { this.loaded = true; });
    window.addEventListener('keydown', event => {
      if (document.querySelector('dialog[open]') || event.repeat) return;
      const shortcut = shortcutFromEvent(event);
      const target = shortcut && this.layout.floating.find(item => item.shortcut === shortcut);
      if (target) { event.preventDefault(); event.stopImmediatePropagation(); this.toggle(target.id); }
      else if (event.code === 'Escape' && !(event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement || (event.target as HTMLElement)?.isContentEditable)) {
        const popup = [...this.layout.floating].reverse().find(item => item.mode === 'popup' && item.open);
        if (popup) { event.preventDefault(); event.stopImmediatePropagation(); this.toggle(popup.id); }
        else if (this.editing) { event.preventDefault(); this.setEditing(false); }
      }
    }, true);
    window.addEventListener('resize', () => this.clampWindows());
  }
  snapshot(): WorkspaceLayout { return structuredClone(this.layout); }
  setKnown(panels: { key: string; title: string }[]): void {
    const known = new Map(panels.map(panel => [panel.key, panel.title]));
    if (JSON.stringify([...known]) === JSON.stringify([...this.known])) return;
    this.known = known; this.render();
  }
  panels(): { key: string; title: string }[] {
    return [...new Map([...this.known, ...[...this.definitions].map(([key, definition]) => [key, definition.title] as const)])].map(([key, title]) => ({ key, title }));
  }
  registerPanel(key: string, title: string, mount: Definition['mount']): Cleanup {
    if (this.definitions.has(key)) throw new Error('duplicate_workspace_panel');
    const owner = Symbol(key); this.definitions.set(key, { title, mount, owner });
    for (const [id, view] of this.views) if (view.panel === key) { this.views.delete(id); void this.disposeView(view).catch(this.error); }
    this.render();
    return async () => {
      if (this.definitions.get(key)?.owner !== owner) return;
      this.definitions.delete(key);
      const cleanup = [...this.views].filter(([, view]) => view.panel === key).map(([id, view]) => { this.views.delete(id); return this.disposeView(view); });
      this.render(); await Promise.all(cleanup);
    };
  }
  openPanel(key: string, automatic = false): void {
    if (automatic) return; // The saved workspace, not activation, owns visibility.
    const area = leaves(this.layout.root).find(area => area.panel === key);
    const floating = this.layout.floating.find(item => item.panel === key);
    if (area) { this.focusedArea = area.id; this.root.querySelector<HTMLElement>(`[data-area="${area.id}"] .area-content`)?.focus(); return; }
    if (floating) { if (!floating.open) this.toggle(floating.id); else this.focusFloating(floating.id); return; }
    // Activation may restore configured panels, but cannot create surprise windows.
    if (!automatic) this.addFloating(key, 'popup');
  }
  setEditing(value: boolean): void { this.editing = value; window.dispatchEvent(new Event('ares-input-release')); this.render(); }
  toggleEditing(): void { this.setEditing(!this.editing); }
  isEditing(): boolean { return this.editing; }
  reset(): void { this.commit(defaultLayout()); this.editing = false; this.render(); }
  split(id: string, axis: 'x' | 'y'): void {
    const next = this.snapshot();
    next.root = transformArea(next.root, id, area => 'panel' in area ? { id: crypto.randomUUID(), axis, ratio: 0.5, children: [area, { id: crypto.randomUUID(), panel: 'empty' }] } : area);
    this.commit(next);
  }
  selectPanel(id: string, panel: string): void {
    const next = this.snapshot();
    if (panel === 'video') next.root = this.replaceVideo(next.root, id);
    next.root = transformArea(next.root, id, area => ({ id: area.id, panel })); this.commit(next);
  }
  private replaceVideo(area: Area, target: string): Area {
    return 'panel' in area ? area.panel === 'video' && area.id !== target ? { ...area, panel: 'empty' } : area
      : { ...area, children: area.children.map(child => this.replaceVideo(child, target)) as [Area, Area] };
  }
  closeArea(id: string): void { const next = this.snapshot(); next.root = removeArea(next.root, id); this.commit(next); }
  addFloating(panel: string, mode: FloatingPanel['mode']): void {
    const next = this.snapshot();
    const shortcut = Array.from({ length: 8 }, (_, i) => `Mod+Shift+Digit${i + 1}`).find(key => !next.floating.some(item => item.shortcut === key)) ?? '';
    const item: FloatingPanel = { id: crypto.randomUUID(), panel, mode, open: true, x: 60 + next.floating.length * 24, y: 60 + next.floating.length * 24, width: 380, height: 400, shortcut, interactive: true };
    next.floating.push(item); this.commit(next); this.focusFloating(item.id);
  }
  updateFloating(id: string, patch: Partial<Omit<FloatingPanel, 'id'>>): void {
    const next = this.snapshot(); next.floating = next.floating.map(item => item.id === id ? { ...item, ...patch } : item); this.commit(next);
  }
  removeFloating(id: string): void { const next = this.snapshot(); next.floating = next.floating.filter(item => item.id !== id); this.commit(next); }
  toggle(id: string): void {
    const item = this.layout.floating.find(item => item.id === id); if (!item) return;
    window.dispatchEvent(new Event('ares-input-release'));
    this.updateFloating(id, { open: !item.open });
    if (!item.open) this.focusFloating(id); else document.getElementById('screen')?.focus({ preventScroll: true });
  }
  private focusFloating(id: string): void {
    if (this.layout.floating.find(item => item.id === id)?.interactive === false) return;
    const element = this.floatingRoot.querySelector<HTMLElement>(`[data-floating="${id}"]`);
    if (element) { element.style.zIndex = String(++this.top); element.querySelector<HTMLElement>('.area-content')?.focus({ preventScroll: true }); }
  }
  private commit(value: WorkspaceLayout): void {
    if (!this.loaded) { void this.ready.then(() => this.commit(value)); return; }
    let layout: WorkspaceLayout;
    try { layout = validateLayout(value); } catch (error) { this.error(error); return; }
    const revision = ++this.revision; this.layout = layout; this.render();
    this.queue = this.queue.then(async () => {
      try { const saved = await window.ares.workspace.save(layout); this.confirmed = saved.layout; }
      catch (error) { if (revision === this.revision) { this.layout = structuredClone(this.confirmed); this.render(); } this.error(error); }
    });
  }
  private options(select: HTMLSelectElement, value: string, allowVideo = true): void {
    const options = [...(allowVideo ? [{ key: 'video', title: 'Video' }, { key: 'empty', title: 'Choose panel…' }] : []), ...this.panels()];
    if (!options.some(item => item.key === value)) options.push({ key: value, title: 'Unavailable panel' });
    for (const option of options) { const element = document.createElement('option'); element.value = option.key; element.textContent = option.title; select.append(element); }
    select.value = value;
  }
  private view(id: string, panel: string, used: Set<string>): HTMLElement {
    used.add(id);
    let view = this.views.get(id);
    if (view?.panel !== panel) { if (view) void this.disposeView(view).catch(this.error); this.views.delete(id); view = undefined; }
    if (!view) {
      const element = document.createElement('article'); element.className = 'area-content'; element.tabIndex = -1;
      view = { panel, element, visible: true, visibility: new Set() }; this.views.set(id, view);
      const definition = this.definitions.get(panel);
      if (panel === 'video') element.append(this.video);
      else if (definition) {
        const parts = panel.split(':'); element.dataset.pluginPanel = `${parts[1]}:${parts[2]}`;
        try { view.cleanup = definition.mount({ instanceId: id, element, onVisibility: callback => { view!.visibility.add(callback); callback(view!.visible); return () => { view!.visibility.delete(callback); }; } }) || undefined; }
        catch (error) { this.error(error); element.textContent = 'Panel could not be loaded. See Plugins in Settings.'; }
      } else {
        element.classList.add('empty-area');
        const text = document.createElement('p'); text.textContent = panel === 'empty' ? 'Choose a panel above.' : `${this.known.get(panel) ?? 'This panel'} is unavailable. Enable its plugin or load a supported game.`;
        element.append(text);
      }
    }
    this.setVisible(view, true); return view.element;
  }
  private setVisible(view: View, visible: boolean): void {
    if (view.visible === visible) return;
    view.visible = visible; view.element.hidden = !visible; for (const callback of view.visibility) callback(visible);
  }
  private async disposeView(view: View): Promise<void> {
    this.setVisible(view, false); view.visibility.clear();
    if (view.panel === 'video' && view.element.contains(this.video)) document.getElementById('panel-parking')!.append(this.video);
    view.element.remove(); await view.cleanup?.();
  }
  private renderArea(area: Area, used: Set<string>): HTMLElement {
    const element = document.createElement('section'); element.dataset.area = area.id;
    if ('panel' in area) {
      element.className = 'workspace-area'; element.onpointerdown = () => { this.focusedArea = area.id; };
      const toolbar = document.createElement('header'); toolbar.className = 'area-toolbar';
      const select = document.createElement('select'); select.setAttribute('aria-label', 'Panel'); this.options(select, area.panel); select.onchange = () => this.selectPanel(area.id, select.value);
      toolbar.append(select, button('↔', 'Split side by side', () => this.split(area.id, 'x')), button('↕', 'Split above and below', () => this.split(area.id, 'y')));
      if (area.panel !== 'video' && area.panel !== 'empty') toolbar.append(button('◧', 'Show as overlay', () => this.addFloating(area.panel, 'overlay')), button('▣', 'Show as pop-up', () => this.addFloating(area.panel, 'popup')));
      if (leaves(this.layout.root).length > 1) toolbar.append(button('×', 'Close region', () => this.closeArea(area.id)));
      element.append(toolbar, this.view(`area:${area.id}`, area.panel, used)); return element;
    }
    element.className = `workspace-split split-${area.axis}`;
    const [first, second] = area.children.map(child => this.renderArea(child, used));
    first.style.flex = `${area.ratio} 0 0`; second.style.flex = `${1 - area.ratio} 0 0`;
    const divider = document.createElement('div'); divider.className = 'split-divider'; divider.setAttribute('role', 'separator'); divider.tabIndex = 0; divider.setAttribute('aria-label', area.axis === 'x' ? 'Resize columns' : 'Resize rows');
    divider.setAttribute('aria-orientation', area.axis === 'x' ? 'vertical' : 'horizontal');
    const apply = (ratio: number) => { area.ratio = Math.max(0.1, Math.min(0.9, ratio)); first.style.flex = `${area.ratio} 0 0`; second.style.flex = `${1 - area.ratio} 0 0`; divider.setAttribute('aria-valuenow', String(Math.round(area.ratio * 100))); };
    divider.onpointerdown = event => {
      event.preventDefault(); divider.setPointerCapture(event.pointerId); const rect = element.getBoundingClientRect();
      const move = (event: PointerEvent) => apply(area.axis === 'x' ? (event.clientX - rect.left) / rect.width : (event.clientY - rect.top) / rect.height);
      divider.onpointermove = move; divider.onpointerup = divider.onpointercancel = () => { divider.onpointermove = null; this.commit(this.snapshot()); };
    };
    divider.onkeydown = event => { if (['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown'].includes(event.code)) { event.preventDefault(); apply(area.ratio + (['ArrowLeft', 'ArrowUp'].includes(event.code) ? -0.05 : 0.05)); this.commit(this.snapshot()); } };
    apply(area.ratio); element.append(first, divider, second); return element;
  }
  private render(): void {
    const focused = document.activeElement;
    ++this.generation;
    this.root.classList.toggle('editing', this.editing); this.root.classList.toggle('has-layout', leaves(this.layout.root).length > 1 || leaves(this.layout.root)[0]?.panel !== 'video');
    this.floatingRoot.classList.toggle('editing', this.editing);
    const used = new Set<string>();
    this.root.replaceChildren(this.renderArea(this.layout.root, used));
    const windows: HTMLElement[] = [];
    for (const item of this.layout.floating) {
      const viewId = `floating:${item.id}`;
      if (!item.open) { const view = this.views.get(viewId); if (view) { used.add(viewId); this.setVisible(view, false); } continue; }
      const element = document.createElement('section'); element.className = `floating-panel ${item.mode}${!item.interactive ? ' passthrough' : ''}`;
      element.dataset.floating = item.id; element.setAttribute('role', 'dialog'); element.setAttribute('aria-label', this.definitions.get(item.panel)?.title ?? this.known.get(item.panel) ?? 'Plugin panel');
      element.style.zIndex = String(20 + windows.length);
      element.onpointerdown = () => { element.style.zIndex = String(++this.top); };
      const header = document.createElement('header'); header.className = 'floating-header';
      const title = document.createElement('span'); title.textContent = this.definitions.get(item.panel)?.title ?? this.known.get(item.panel) ?? 'Plugin panel';
      header.append(title, button('×', 'Hide panel', () => this.toggle(item.id)));
      const content = this.view(viewId, item.panel, used), grip = document.createElement('div'); grip.className = 'window-resize'; grip.setAttribute('aria-label', 'Resize panel');
      this.geometry(element, item);
      const drag = (event: PointerEvent, resizing: boolean) => {
        if ((event.target as HTMLElement).closest('button')) return;
        event.preventDefault(); const target = resizing ? grip : header; target.setPointerCapture(event.pointerId);
        const rect = element.getBoundingClientRect();
        const start = { ...item, x: rect.left, y: rect.top, width: rect.width, height: rect.height };
        // Pointer origin names differ from stored position to avoid shadowing.
        const originX = event.clientX, originY = event.clientY;
        target.onpointermove = event => {
          const dx = event.clientX - originX, dy = event.clientY - originY;
          if (resizing) { item.width = Math.max(180, Math.min(8192, start.width + dx)); item.height = Math.max(100, Math.min(8192, start.height + dy)); }
          else { item.x = Math.max(0, Math.min(innerWidth - 60, start.x + dx)); item.y = Math.max(0, Math.min(innerHeight - 36, start.y + dy)); }
          this.geometry(element, item);
        };
        target.onpointerup = target.onpointercancel = () => { target.onpointermove = null; this.commit(this.snapshot()); };
      };
      header.onpointerdown = event => drag(event, false); grip.onpointerdown = event => drag(event, true);
      element.append(header, content, grip); windows.push(element);
    }
    this.floatingRoot.replaceChildren(...windows);
    for (const [id, view] of this.views) if (!used.has(id)) { this.views.delete(id); void this.disposeView(view).catch(this.error); }
    if (focused instanceof HTMLElement && focused.isConnected && focused !== document.body) focused.focus({ preventScroll: true });
    this.changed();
  }
  private geometry(element: HTMLElement, item: FloatingPanel): void {
    element.style.left = `${Math.min(item.x, Math.max(0, innerWidth - 60))}px`; element.style.top = `${Math.min(item.y, Math.max(0, innerHeight - 36))}px`;
    element.style.width = `${Math.min(item.width, innerWidth)}px`; element.style.height = `${Math.min(item.height, innerHeight)}px`;
  }
  private clampWindows(): void { for (const item of this.layout.floating) { const element = this.floatingRoot.querySelector<HTMLElement>(`[data-floating="${item.id}"]`); if (element) this.geometry(element, item); } }
}
