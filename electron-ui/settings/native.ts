import { validateValue, type SettingDescriptor, type SettingsSnapshot } from './contracts.js';
export interface NativeSettingsInfo {
  firmware: { system: string; type: string; region: string; key: string }[];
  imported: Record<string, string>;
}
/** Native discovery comes from the compiled emulator registry, not a second system list. */
export function parseNativeSettingsInfo(text: string): NativeSettingsInfo {
  const data = JSON.parse(text) as NativeSettingsInfo;
  if (!data || !Array.isArray(data.firmware) || data.firmware.length > 256 || !data.imported || typeof data.imported !== 'object' || Array.isArray(data.imported)) throw new Error('invalid_native_settings');
  for (const field of data.firmware) {
    if (!field || !['system', 'type', 'region', 'key'].every(k => typeof field[k as keyof typeof field] === 'string') || !/^[A-Za-z0-9.+-]+\/Firmware\/[A-Za-z0-9.+-]+$/.test(field.key)) throw new Error('invalid_native_settings');
  }
  if (!Object.values(data.imported).every(v => typeof v === 'string')) throw new Error('invalid_native_settings');
  return data;
}
export function firmwareDescriptors(info: NativeSettingsInfo): SettingDescriptor[] {
  return info.firmware.map(field => ({
    id: `firmware.${field.key.replace('/Firmware/', '.')}`, title: `${field.system} — ${field.type} (${field.region})`,
    section: 'Firmware', type: 'string', default: '', file: { extensions: [] }, nativeKey: field.key, apply: 'next-rom',
  }));
}
export function importedValues(info: NativeSettingsInfo, descriptors: readonly SettingDescriptor[]): Record<string, string | boolean | number> {
  const values: Record<string, string | boolean | number> = Object.create(null);
  for (const descriptor of descriptors) {
    if (!descriptor.nativeKey || !Object.hasOwn(info.imported, descriptor.nativeKey)) continue;
    const raw = info.imported[descriptor.nativeKey];
    const value = descriptor.type === 'boolean' ? raw === 'true' ? true : raw === 'false' ? false : null
      : descriptor.type === 'number' ? raw.trim() ? Number(raw) : null : raw;
    if (validateValue(descriptor, value)) values[descriptor.id] = value;
  }
  return values;
}
/** A small BML writer: names come from trusted bindings, values are single-line. */
export function nativeSettingsBml(snapshot: SettingsSnapshot): string {
  type Node = { value?: string; children: Map<string, Node> };
  const root: Node = { children: new Map() };
  for (const { descriptor, value } of snapshot.entries) {
    if (!descriptor.nativeKey) continue;
    if (!validateValue(descriptor, value) || /[\r\n\u0000]/.test(String(value))) throw new Error('invalid_native_setting_value');
    let node = root;
    for (const key of descriptor.nativeKey.split('/')) {
      if (!/^[A-Za-z0-9.+-]+$/.test(key)) throw new Error('invalid_native_setting_key');
      if (!node.children.has(key)) node.children.set(key, { children: new Map() });
      node = node.children.get(key)!;
    }
    node.value = String(value);
  }
  const write = (node: Node, depth: number): string => [...node.children].map(([name, child]) =>
    `${'  '.repeat(depth)}${name}${child.value === undefined ? '' : `: ${child.value}`}\n${write(child, depth + 1)}`).join('');
  return write(root, 0);
}
