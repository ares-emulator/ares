/** Stable IDs are the persistence contract; moving a control does not rename it. */
export type SettingValue = string | boolean | number;
export type SettingDescriptor = {
  id: string; title: string; section: string; apply: 'live' | 'next-rom' | 'restart';
  control?: 'key';
  file?: { extensions: string[] };
  /** Host-owned native binding; plugins cannot choose arbitrary BML keys. */
  nativeKey?: string;
} & (
  | { type: 'boolean'; default: boolean }
  | { type: 'number'; default: number; min: number; max: number; integer?: boolean }
  | { type: 'string'; default: string; maxLength?: number }
  | { type: 'enum'; default: string; options: readonly string[] }
);
export interface SettingEntry {
  descriptor: SettingDescriptor;
  value: SettingValue;
  applied: SettingValue;
  source: 'default' | 'profile' | 'launch' | 'imported';
  pending: boolean;
}
export interface SettingsSnapshot { revision: number; entries: SettingEntry[]; error?: string }
export interface PreferencesBridge {
  catalog(): Promise<SettingsSnapshot>;
  set(id: string, value: SettingValue): Promise<SettingsSnapshot>;
  reset(id: string): Promise<SettingsSnapshot>;
  chooseFile(id: string): Promise<SettingsSnapshot>;
  changed(callback: (snapshot: SettingsSnapshot) => void): () => void;
}
export function validateValue(descriptor: SettingDescriptor, value: unknown): value is SettingValue {
  switch (descriptor.type) {
    case 'boolean': return typeof value === 'boolean';
    case 'number': return typeof value === 'number' && Number.isFinite(value) && (!descriptor.integer || Number.isInteger(value)) && value >= descriptor.min && value <= descriptor.max;
    case 'enum': return typeof value === 'string' && descriptor.options.includes(value);
    case 'string': return typeof value === 'string' && (!descriptor.nativeKey || value.trim() === value) && (descriptor.control !== 'key' || /^(?:[A-Za-z][A-Za-z0-9]{0,31}(?:,[A-Za-z][A-Za-z0-9]{0,31}){0,3})?$/.test(value)) && value.length <= (descriptor.maxLength ?? 4096) && !/[\u0000-\u001f\u007f]/.test(value);
  }
}
