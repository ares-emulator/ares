/** Host-owned placement of ordinary plugin panels; no extension-specific layout code. */
export type Area = { id: string; panel: string } | { id: string; axis: 'x' | 'y'; ratio: number; children: [Area, Area] };
export interface FloatingPanel {
  id: string; panel: string; mode: 'overlay' | 'popup'; open: boolean;
  x: number; y: number; width: number; height: number; shortcut: string; interactive: boolean;
}
export interface WorkspaceLayout { version: 1; root: Area; floating: FloatingPanel[] }
export interface WorkspaceSnapshot { layout: WorkspaceLayout; error?: string }
export interface WorkspaceBridge { load(): Promise<WorkspaceSnapshot>; save(layout: WorkspaceLayout): Promise<WorkspaceSnapshot> }
export const defaultLayout = (): WorkspaceLayout => ({ version: 1, root: { id: 'video', panel: 'video' }, floating: [] });
const object = (value: unknown, fields: string[]): boolean => !!value && typeof value === 'object' && !Array.isArray(value) && Object.keys(value).sort().join(',') === [...fields].sort().join(',');
const identifier = (value: unknown): value is string => typeof value === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(value);
export const panelKey = (plugin: string, panel: string) => `plugin:${plugin}:${panel}`;
export function validPanel(value: unknown): value is string {
  return typeof value === 'string' && (value === 'video' || value === 'empty' || /^plugin:[a-z][a-z0-9.-]{0,95}:[a-z][a-z0-9.-]{0,63}$/.test(value));
}
export const validShortcut = (value: unknown): value is string => typeof value === 'string' && (value === '' || !['Mod+KeyO', 'Mod+KeyP', 'Mod+Shift+KeyR', 'Mod+Shift+KeyL', 'Mod+KeyQ', 'Mod+KeyW', 'Mod+KeyH', 'Mod+KeyM', 'Mod+KeyC', 'Mod+KeyV', 'Mod+KeyX', 'Mod+KeyA', 'Mod+KeyZ', 'Mod+Shift+KeyZ'].includes(value) && /^(?:Mod|Alt)(?:\+Shift)?\+(?:Key[A-Z]|Digit[0-9]|F(?:[1-9]|1[0-2]))$/.test(value));
export function validateLayout(input: unknown): WorkspaceLayout {
  const value = input as Record<string, unknown>;
  if (!object(value, ['version', 'root', 'floating']) || value.version !== 1 || !Array.isArray(value.floating) || value.floating.length > 8) throw new Error('invalid_workspace');
  const ids = new Set<string>(), shortcuts = new Set<string>(); let areas = 0, videos = 0;
  const id = (value: unknown) => { if (!identifier(value) || ids.has(value)) throw new Error('invalid_workspace_id'); ids.add(value); };
  const area = (value: unknown, depth: number): void => {
    if (!value || typeof value !== 'object' || depth > 8 || ++areas > 31) throw new Error('workspace_too_complex');
    const node = value as Record<string, unknown>; id(node.id);
    if (object(node, ['id', 'panel'])) { if (!validPanel(node.panel)) throw new Error('invalid_workspace_panel'); if (node.panel === 'video') videos++; return; }
    if (!object(node, ['id', 'axis', 'ratio', 'children']) || !['x', 'y'].includes(String(node.axis)) || typeof node.ratio !== 'number' || !Number.isFinite(node.ratio) || node.ratio < 0.1 || node.ratio > 0.9 || !Array.isArray(node.children) || node.children.length !== 2) throw new Error('invalid_workspace_split');
    node.children.forEach(child => area(child, depth + 1));
  };
  area(value.root, 0);
  for (const item of value.floating) {
    if (!object(item, ['id', 'panel', 'mode', 'open', 'x', 'y', 'width', 'height', 'shortcut', 'interactive']) || !validPanel(item.panel) || item.panel === 'video' || item.panel === 'empty' || !['overlay', 'popup'].includes(String(item.mode)) || typeof item.open !== 'boolean' || typeof item.interactive !== 'boolean' || !validShortcut(item.shortcut)) throw new Error('invalid_workspace_overlay');
    id(item.id);
    if (item.shortcut && shortcuts.has(item.shortcut)) throw new Error('duplicate_workspace_shortcut');
    shortcuts.add(item.shortcut);
    for (const key of ['x', 'y', 'width', 'height']) if (typeof item[key] !== 'number' || !Number.isFinite(item[key]) || Number(item[key]) < (key === 'width' ? 180 : key === 'height' ? 100 : 0) || Number(item[key]) > 8192) throw new Error('invalid_workspace_geometry');
  }
  if (videos > 1) throw new Error('one_video_region_per_workspace');
  return structuredClone(value) as unknown as WorkspaceLayout;
}
export function leaves(root: Area): Extract<Area, { panel: string }>[] { return 'panel' in root ? [root] : root.children.flatMap(leaves); }
export function transformArea(root: Area, id: string, change: (area: Area) => Area): Area {
  if (root.id === id) return change(root);
  return 'panel' in root ? root : { ...root, children: root.children.map(child => transformArea(child, id, change)) as [Area, Area] };
}
export function removeArea(root: Area, id: string): Area {
  if ('panel' in root) return root;
  if (root.children.some(child => child.id === id)) return root.children.find(child => child.id !== id)!;
  return { ...root, children: root.children.map(child => removeArea(child, id)) as [Area, Area] };
}
export function shortcutFromEvent(event: { metaKey: boolean; ctrlKey: boolean; altKey: boolean; shiftKey: boolean; code: string }): string {
  if (!event.metaKey && !event.ctrlKey && !event.altKey) return '';
  const shortcut = `${event.metaKey || event.ctrlKey ? 'Mod' : 'Alt'}${event.shiftKey ? '+Shift' : ''}+${event.code}`;
  return validShortcut(shortcut) ? shortcut : '';
}
