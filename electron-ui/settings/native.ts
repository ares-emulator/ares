import { validateValue, type SettingDescriptor, type SettingsSnapshot } from './contracts.js';
export interface NativeSettingsInfo {
  features?: { n64Gpu: boolean };
  firmware: { system: string; type: string; region: string; key: string }[];
  imported: Record<string, string>;
}
/** Native discovery comes from the compiled emulator registry, not a second system list. */
export function parseNativeSettingsInfo(text: string): NativeSettingsInfo {
  const data = JSON.parse(text) as NativeSettingsInfo;
  if (!data || !Array.isArray(data.firmware) || data.firmware.length > 256 || !data.imported || typeof data.imported !== 'object' || Array.isArray(data.imported)) throw new Error('invalid_native_settings');
  if (data.features !== undefined && (!data.features || typeof data.features.n64Gpu !== 'boolean')) throw new Error('invalid_native_settings');
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


/** Only options consumed by the extracted loaders are exposed. */
export function coreDescriptors(info: NativeSettingsInfo): SettingDescriptor[] {
  const boolean = (id: string, title: string, section: string, nativeKey: string, value: boolean): SettingDescriptor =>
    ({ id, title, section, nativeKey, type: 'boolean', default: value, apply: 'next-rom' });
  return [
    boolean('core.fastBoot', 'Fast boot (supported systems)', 'Core', 'Boot/Fast', true),
    { id: 'core.preferredRegions', title: 'Preferred regions (comma separated)', section: 'Core', nativeKey: 'Boot/Prefer', type: 'string', default: 'NTSC-U', maxLength: 80, apply: 'next-rom' },
    boolean('core.forceInterpreter', 'Force interpreter (supported cores)', 'Core', 'Developer/ForceInterpreter', false),
    boolean('core.pixelAccuracy', 'Pixel accuracy (supported cores)', 'Core', 'Video/PixelAccuracy', false),
    boolean('core.colorEmulation', 'Color emulation (supported cores)', 'Core', 'Video/ColorEmulation', true),
    boolean('core.interframeBlending', 'Interframe blending (supported cores)', 'Core', 'Video/InterframeBlending', true),
    boolean('n64.expansionPak', 'Expansion Pak', 'Nintendo 64', 'Nintendo64/ExpansionPak', true),
    { id: 'n64.controllerPak', title: 'Controller Pak capacity', section: 'Nintendo 64', nativeKey: 'Nintendo64/ControllerPakBankString', type: 'enum', options: ['32KiB (Default)', '128KiB (Datel 1Meg)', '512KiB (Datel 4Meg)', '1984KiB (Maximum)'], default: '32KiB (Default)', apply: 'next-rom' },
    boolean('n64.homebrewMode', 'Homebrew mode', 'Nintendo 64', 'Developer/HomebrewMode', false),
    boolean('n64.deterministicEntropy', 'Deterministic entropy', 'Nintendo 64', 'Developer/DeterministicEntropy', false),
    boolean('gba.player', 'Game Boy Player', 'Game Boy Advance', 'GameBoyAdvance/Player', false),
    boolean('snes.deepBlackBoost', 'Deep black boost', 'Super Famicom', 'SuperFamicom/DeepBlackBoost', false),
    boolean('megaDrive.tmss', 'TMSS boot', 'Mega Drive', 'MegaDrive/TMSS', false),
    ...(info.features?.n64Gpu ? [
      { id: 'n64.quality', title: 'Render quality', section: 'Nintendo 64', nativeKey: 'Nintendo64/Quality', type: 'enum' as const, options: ['SD', 'HD', 'UHD'], default: 'SD', apply: 'next-rom' as const },
      boolean('n64.supersampling', 'Supersampling', 'Nintendo 64', 'Nintendo64/Supersampling', false),
    ] : []),
  ];
}
