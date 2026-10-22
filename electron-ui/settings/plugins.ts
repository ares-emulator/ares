import type { PluginManifest } from '../plugins/contracts.js';
import { validateValue, type SettingDescriptor, type SettingsSnapshot, type PluginSettingsRequest } from './contracts.js';
import type { SettingsStore } from './store.js';
const identifier = /^[a-z][a-z0-9-]*(?:\.[a-z][a-z0-9-]*)*$/;
const title = (value: unknown): value is string => typeof value === 'string' && value.length > 0 && value.length <= 80 && !/[\u0000-\u001f]/.test(value);
export const pluginSettingsPrefix = (id: string) => `plugin:${id}:`;
/** Activation stages only metadata. Commit/revoke follow the native plugin lease. */
export class PluginSettingsService {
  private registrations = new Map<string, Map<string, { descriptors: SettingDescriptor[]; published: boolean }>>();
  constructor(private store: SettingsStore,
    private set: (id: string, value: unknown, reset: boolean, guard: () => void) => Promise<SettingsSnapshot>,
    private choose: (id: string, guard: () => void) => Promise<SettingsSnapshot>) {}
  private descriptors(token: string): SettingDescriptor[] { return [...(this.registrations.get(token)?.values() ?? [])].flatMap(section => section.descriptors); }
  finish(token: string): void {
    const pending = [...(this.registrations.get(token)?.values() ?? [])].filter(section => !section.published);
    if (!pending.length) return;
    this.store.register(pending.flatMap(section => section.descriptors));
    for (const section of pending) section.published = true;
  }
  revoke(token: string): void {
    const sections = this.registrations.get(token);
    this.registrations.delete(token);
    if (sections) this.store.remove([...sections.values()].filter(section => section.published).flatMap(section => section.descriptors.map(d => d.id)));
  }
  async request(token: string, manifest: PluginManifest, active: boolean, request: PluginSettingsRequest, guard: () => void): Promise<SettingsSnapshot> {
    if (!request || typeof request !== 'object' || Array.isArray(request)) throw new Error('invalid_settings_request');
    const exact = (keys: string) => Object.keys(request).sort().join(',') === keys;
    const prefix = pluginSettingsPrefix(manifest.id);
    const all = manifest.capabilities.includes('settings.read');
    const register = manifest.capabilities.includes('settings.register');
    const catalog = () => {
      const snapshot = this.store.preview(this.descriptors(token));
      if (!all) snapshot.entries = snapshot.entries.filter(entry => entry.descriptor.id.startsWith(prefix));
      return snapshot;
    };
    if (request.operation === 'register' && exact('descriptors,operation,section')) {
      const section = manifest.contributes.settings?.find(section => section.id === request.section);
      if (!register || !section) throw new Error('settings_registration_denied');
      const existing = this.registrations.get(token) ?? new Map();
      if (existing.has(request.section)) throw new Error('settings_section_already_registered');
      if (!Array.isArray(request.descriptors) || !request.descriptors.length || request.descriptors.length > 32 || this.descriptors(token).length + request.descriptors.length > 128) throw new Error('invalid_settings_descriptors');
      const ids = new Set<string>();
      const descriptors: SettingDescriptor[] = request.descriptors.map(raw => {
        if (!raw || typeof raw !== 'object' || Array.isArray(raw)) throw new Error('invalid_settings_descriptor');
        const allowed = ['id', 'title', 'type', 'default', 'apply', ...(raw.type === 'number' ? ['min', 'max', 'integer'] : raw.type === 'enum' ? ['options'] : raw.type === 'string' ? ['maxLength', 'control'] : [])];
        if (Object.keys(raw).some(key => !allowed.includes(key)) || typeof raw.id !== 'string' || raw.id.length > 64 || !identifier.test(raw.id) || ids.has(raw.id) || !title(raw.title) || raw.apply !== undefined && raw.apply !== 'live') throw new Error('invalid_settings_descriptor');
        if (raw.type === 'number' && (!Number.isFinite(raw.min) || !Number.isFinite(raw.max) || raw.min > raw.max || raw.integer !== undefined && typeof raw.integer !== 'boolean')) throw new Error('invalid_settings_descriptor');
        if (raw.type === 'enum' && (!Array.isArray(raw.options) || raw.options.length < 1 || raw.options.length > 32 || !raw.options.every(title) || new Set(raw.options).size !== raw.options.length)) throw new Error('invalid_settings_descriptor');
        if (raw.type === 'string' && (raw.maxLength !== undefined && (!Number.isInteger(raw.maxLength) || raw.maxLength < 1 || raw.maxLength > 4096) || raw.control !== undefined && raw.control !== 'key')) throw new Error('invalid_settings_descriptor');
        ids.add(raw.id);
        const descriptor = { ...raw, id: `${prefix}${section.id}:${raw.id}`, section: `${manifest.name} / ${section.title}`, apply: 'live' as const } as SettingDescriptor;
        if (!['boolean', 'number', 'string', 'enum'].includes(descriptor.type) || !validateValue(descriptor, descriptor.default)) throw new Error('invalid_settings_descriptor');
        return descriptor;
      });
      if (active) this.store.register(descriptors);
      existing.set(request.section, { descriptors, published: active }); this.registrations.set(token, existing);
      return catalog();
    }
    if (request.operation === 'unregister' && exact('operation,section') && register) {
      const section = this.registrations.get(token)?.get(request.section);
      this.registrations.get(token)?.delete(request.section);
      if (section?.published) this.store.remove(section.descriptors.map(d => d.id));
      return catalog();
    }
    if (request.operation === 'catalog' && exact('operation') && (all || register)) return catalog();
    if (['set', 'reset', 'chooseFile'].includes(request.operation)) {
      if (!active) throw new Error('plugin_not_ready');
      if (!('id' in request) || typeof request.id !== 'string' || !exact(request.operation === 'set' ? 'id,operation,value' : 'id,operation')) throw new Error('invalid_settings_request');
      const own = register && this.descriptors(token).some(descriptor => descriptor.id === request.id);
      if (!own && !(all && manifest.capabilities.includes('settings.write'))) throw new Error('settings_write_denied');
      guard();
      if (request.operation === 'chooseFile') await this.choose(request.id, guard);
      else await this.set(request.id, request.operation === 'set' ? request.value : undefined, request.operation === 'reset', guard);
      guard(); return catalog();
    }
    throw new Error('settings_access_denied');
  }
}
