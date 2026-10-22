import fs from 'node:fs';
import path from 'node:path';
import { randomUUID } from 'node:crypto';
import { validateValue, type SettingDescriptor, type SettingValue, type SettingsSnapshot } from './contracts.js';

/** The main process owns writes. Unknown values survive descriptor removal and readdition. */
export class SettingsStore {
  private values: Record<string, unknown> = Object.create(null);
  private applied = new Map<string, SettingValue>();
  private descriptors = new Map<string, SettingDescriptor>();
  private revision = 0;
  private error?: string;
  private unreadable = false;
  private queue: Promise<unknown> = Promise.resolve();
  private applyError?: string;
  private launchValues: Record<string, unknown>;
  constructor(readonly filename: string, descriptors: readonly SettingDescriptor[], private changed: (snapshot: SettingsSnapshot) => void = () => {}, private applyLive: (id: string, value: SettingValue) => Promise<void> = async () => {}, launchValues: Record<string, unknown> = {}) {
    this.launchValues = Object.assign(Object.create(null), launchValues);
    for (const descriptor of descriptors) {
      if (!/^[a-zA-Z][a-zA-Z0-9._-]{0,127}$/.test(descriptor.id) || this.descriptors.has(descriptor.id) || !validateValue(descriptor, descriptor.default)) throw new Error('invalid_setting_descriptor');
      this.descriptors.set(descriptor.id, structuredClone(descriptor));
    }
    try {
      if (fs.statSync(filename).size > 1024 * 1024) throw new Error('settings_file_too_large');
      const data = JSON.parse(fs.readFileSync(filename, 'utf8'));
      if (!data || data.version !== 1 || !data.values || typeof data.values !== 'object' || Array.isArray(data.values)) throw new Error('invalid_settings_file');
      this.values = Object.assign(Object.create(null), data.values);
    } catch (error) {
      if ((error as NodeJS.ErrnoException).code !== 'ENOENT') {
        this.error = `Cannot read ${filename}: ${String(error)}`;
        this.unreadable = true;
      }
    }
    for (const descriptor of this.descriptors.values()) this.applied.set(descriptor.id, this.value(descriptor));
  }
  private value(descriptor: SettingDescriptor): SettingValue {
    const launch = this.launchValues[descriptor.id];
    if (validateValue(descriptor, launch)) return launch;
    const value = this.values[descriptor.id];
    return validateValue(descriptor, value) ? value : descriptor.default;
  }
  catalog(): SettingsSnapshot {
    const invalid = [...this.descriptors.values()].filter(d => Object.hasOwn(this.values, d.id) && !validateValue(d, this.values[d.id])).map(d => d.id);
    return {
      revision: this.revision,
      entries: [...this.descriptors.values()].map(descriptor => {
        const value = this.value(descriptor), applied = this.applied.get(descriptor.id) ?? descriptor.default;
        return { descriptor: structuredClone(descriptor), value, applied, source: validateValue(descriptor, this.launchValues[descriptor.id]) ? 'launch' : validateValue(descriptor, this.values[descriptor.id]) ? 'profile' : 'default', pending: value !== applied };
      }),
      ...(this.error || this.applyError || invalid.length ? { error: this.error ?? this.applyError ?? `Invalid saved settings (using defaults): ${invalid.join(', ')}` } : {}),
    };
  }
  /** Acknowledge the exact startup snapshot, even if desired values changed during load. */
  markApplied(snapshot: SettingsSnapshot, timing: 'next-rom' | 'restart'): void {
    for (const entry of snapshot.entries) {
      const descriptor = this.descriptors.get(entry.descriptor.id);
      if (descriptor?.apply === timing && validateValue(descriptor, entry.value)) this.applied.set(descriptor.id, entry.value);
    }
    this.revision++;
    this.changed(this.catalog());
  }
  set(id: string, value: unknown): Promise<SettingsSnapshot> {
    const operation = this.queue.then(() => this.write(id, value));
    this.queue = operation.catch(() => {});
    return operation;
  }
  private async write(id: string, value: unknown): Promise<SettingsSnapshot> {
    const descriptor = this.descriptors.get(id);
    if (!descriptor || !validateValue(descriptor, value)) throw new Error('invalid_setting_value');
    // Never overwrite a malformed/unsupported provisioned file with partial data.
    if (this.unreadable) throw new Error(this.error);
    const values = Object.assign(Object.create(null), this.values, { [id]: value });
    fs.mkdirSync(path.dirname(this.filename), { recursive: true });
    const temporary = `${this.filename}.${randomUUID()}.tmp`;
    try {
      fs.writeFileSync(temporary, JSON.stringify({ version: 1, values }, null, 2) + '\n', { flag: 'wx', mode: 0o600 });
      fs.renameSync(temporary, this.filename);
    } finally { fs.rmSync(temporary, { force: true }); }
    this.values = values;
    delete this.launchValues[id];
    this.applyError = undefined;
    if (descriptor.apply === 'live') {
      try {
        await this.applyLive(id, value);
        this.applied.set(id, value);
      } catch (error) {
        this.applyError = `Saved ${id}, but could not apply it: ${String(error)}`;
      }
    }
    this.revision++;
    const snapshot = this.catalog();
    this.changed(snapshot);
    return snapshot;
  }
}
