import type { PreferencesBridge, SettingEntry, SettingsSnapshot } from './contracts.js';

/** Descriptor-driven controls; reusable by a settings plugin with the same API. */
export function mountSettingsForm(element: HTMLElement, bridge: PreferencesBridge, include = (_entry: SettingEntry) => true): () => void {
  const message = document.createElement('p');
  message.setAttribute('role', 'status');
  const sections = document.createElement('div');
  element.append(sections, message);
  type Row = { entry: SettingEntry; input: HTMLInputElement | HTMLSelectElement; note: HTMLElement; dirty: boolean; busy: boolean };
  const rows = new Map<string, Row>();
  let disposed = false, revision = -1, signature = '';
  const value = (row: Row) => row.entry.descriptor.type === 'boolean' ? (row.input as HTMLInputElement).checked
    : row.entry.descriptor.type === 'number' ? Number(row.input.value) : row.input.value;
  const run = async (row: Row, action: () => Promise<SettingsSnapshot>) => {
    if (row.busy) return;
    row.busy = true;
    row.input.disabled = true;
    try {
      const snapshot = await action();
      if (disposed) return;
      row.dirty = false;
      update(snapshot);
    } catch (error) { if (!disposed) message.textContent = String(error); }
    finally { row.busy = false; row.input.disabled = false; }
  };
  const update = (snapshot: SettingsSnapshot) => {
    if (disposed || snapshot.revision < revision) return;
    revision = snapshot.revision;
    const entries = snapshot.entries.filter(include);
    const nextSignature = JSON.stringify(entries.map(entry => entry.descriptor));
    if (signature !== nextSignature) {
      signature = nextSignature;
      sections.replaceChildren(); rows.clear();
      const groups = new Map<string, HTMLElement>();
      for (const entry of entries) {
        const descriptor = entry.descriptor;
        let group = groups.get(descriptor.section);
        if (!group) {
          const details = document.createElement('details'), summary = document.createElement('summary');
          summary.textContent = descriptor.section;
          details.append(summary); sections.append(details); groups.set(descriptor.section, details); group = details;
        }
        const field = document.createElement('div'), label = document.createElement('label');
        field.className = 'setting-field'; field.dataset.setting = descriptor.id;
        label.textContent = descriptor.title;
        let input: HTMLInputElement | HTMLSelectElement;
        if (descriptor.type === 'enum') {
          input = document.createElement('select');
          for (const option of descriptor.options) {
            const element = document.createElement('option'); element.value = option; element.textContent = option; input.append(element);
          }
        } else {
          input = document.createElement('input');
          input.type = descriptor.type === 'boolean' ? 'checkbox' : descriptor.type === 'number' ? 'number' : 'text';
          if (descriptor.type === 'number') { input.min = String(descriptor.min); input.max = String(descriptor.max); input.step = descriptor.integer ? '1' : 'any'; }
        }
        label.append(input); field.append(label);
        const save = document.createElement('button'), note = document.createElement('p');
        save.textContent = 'Save';
        const row: Row = { entry, input, note, dirty: false, busy: false }; rows.set(descriptor.id, row);
        input.oninput = () => { row.dirty = true; };
        save.onclick = () => { void run(row, () => bridge.set(descriptor.id, value(row))); };
        field.append(save);
        if (descriptor.control === 'key') {
          const record = document.createElement('button'); record.textContent = 'Record key';
          let recording = false;
          record.onclick = () => { recording = true; record.textContent = 'Press a key (Esc cancels)'; input.focus(); };
          input.onkeydown = event => {
            if (!recording) return;
            event.preventDefault(); event.stopPropagation();
            recording = false; record.textContent = 'Record key';
            if (event.code !== 'Escape') { input.value = event.code; row.dirty = true; }
          };
          input.onblur = () => { recording = false; record.textContent = 'Record key'; };
          field.append(record);
        }

        if (descriptor.file) {
          const browse = document.createElement('button'); browse.textContent = 'Choose file…';
          browse.onclick = () => { void run(row, () => bridge.chooseFile(descriptor.id)); };
          field.append(browse);
        }
        const reset = document.createElement('button'); reset.textContent = 'Reset';
        reset.onclick = () => { void run(row, () => bridge.reset(descriptor.id)); };
        field.append(reset, note); group.append(field);
      }
    }
    for (const entry of entries) {
      const row = rows.get(entry.descriptor.id)!; row.entry = entry;
      if (!row.dirty) {
        if (entry.descriptor.type === 'boolean') (row.input as HTMLInputElement).checked = entry.value === true;
        else row.input.value = String(entry.value);
      }
      row.note.textContent = `${entry.source} · ${entry.pending ? `Saved; applies ${entry.descriptor.apply === 'next-rom' ? 'on next ROM load' : entry.descriptor.apply === 'restart' ? 'after restart' : 'when applied'}` : entry.descriptor.apply === 'next-rom' ? 'Next ROM load' : entry.descriptor.apply === 'restart' ? 'Restart required' : 'Live'}`;
      row.note.dataset.pending = String(entry.pending);
    }
    message.textContent = snapshot.error ?? '';
  };
  const unsubscribe = bridge.changed(update);
  void bridge.catalog().then(update).catch(error => { if (!disposed) message.textContent = String(error); });
  return () => { disposed = true; unsubscribe(); element.replaceChildren(); };
}
