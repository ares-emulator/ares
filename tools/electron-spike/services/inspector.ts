import { MemoryClient, hexToBytes, type RegionHandle } from './memory.js';
import type { SessionClient } from './contracts.js';
const bridge = (window as unknown as { ares: { session: SessionClient['request'] } }).ares;
const memory = new MemoryClient({ request: (method, identity, params) => bridge.session(method, identity, params) });
const select = document.getElementById('memory-region') as HTMLSelectElement;
const info = document.getElementById('memory-info')!;
const output = document.getElementById('memory-output')!;
const write = document.getElementById('memory-write') as HTMLButtonElement;
const read = document.getElementById('memory-read') as HTMLButtonElement;
let regions: RegionHandle[] = [], generation = 0;
const current = (): RegionHandle => {
  const region = regions.find(region => region.id === select.value);
  if (!region) throw new Error('Select a memory region.');
  return region;
};
function describe(): void {
  const region = regions.find(region => region.id === select.value);
  info.textContent = region ? `${region.size.toLocaleString()} bytes · region offsets · ${region.writable ? 'writable' : region.readable ? 'read only' : 'unavailable'}${region.unavailableReason ? ' · ' + region.unavailableReason : ''}` : 'No regions loaded.';
  read.disabled = !region?.readable;
  write.disabled = !region?.writable;
}
async function refresh(): Promise<void> {
  const token = ++generation;
  read.disabled = write.disabled = true;
  output.textContent = '';
  try {
    const next = await memory.regions();
    if (token !== generation) return;
    regions = next; select.replaceChildren();
    for (const region of regions) { const option = document.createElement('option'); option.value = region.id; option.textContent = region.name; select.append(option); }
    describe();
  } catch (error) { if (token === generation) { info.textContent = String(error); regions = []; select.replaceChildren(); } }
}
function offset(): number {
  const value = (document.getElementById('memory-offset') as HTMLInputElement).value.trim();
  if (!/^(?:0x[0-9a-f]+|[0-9]+)$/i.test(value) || !Number.isSafeInteger(Number(value))) throw new Error('Enter a decimal or 0x hexadecimal region offset.');
  return Number(value);
}
async function show(operation: () => Promise<string>): Promise<void> {
  const token = generation;
  read.disabled = write.disabled = true;
  try { const result = await operation(); if (token === generation) output.textContent = result; }
  catch (error) { if (token === generation) output.textContent = String(error); }
  finally { if (token === generation) describe(); }
}
document.getElementById('memory-refresh')!.onclick = refresh;
select.onchange = () => { ++generation; output.textContent = ''; describe(); };
read.onclick = () => show(async () => {
  const start = offset(), length = Number((document.getElementById('memory-length') as HTMLInputElement).value);
  const result = await memory.read(current(), start, length);
  return `Offset 0x${result.offset.toString(16)} (${result.length} bytes)\n${result.hex.match(/.{1,32}/g)?.join('\n')}`;
});
write.onclick = () => show(async () => {
  const hex = (document.getElementById('memory-bytes') as HTMLTextAreaElement).value.replace(/\s/g, '');
  const result = await memory.write(current(), offset(), hexToBytes(hex));
  return `Wrote ${result.written} bytes at region offset 0x${result.offset.toString(16)}.`;
});
window.addEventListener('ares-session-reset', refresh);
void refresh();
