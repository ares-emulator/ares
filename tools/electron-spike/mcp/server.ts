import path from 'node:path';
import { McpServer } from '@modelcontextprotocol/server';
import { serveStdio } from '@modelcontextprotocol/server/stdio';
import * as z from 'zod/v4';
import { MAX_TRANSFER, ServiceError, type SessionReply } from '../services/contracts';
import { LocalServiceClient } from '../services/local-endpoint';
import { MemoryClient, hexToBytes, type RegionHandle } from '../services/memory';

const args = process.argv.slice(2);
const take = (flag: string) => { const index = args.indexOf(flag); return index >= 0 ? args[index + 1] : undefined; };
const file = take('--connection'), instance = take('--instance');
if (!file || !path.isAbsolute(file) || !instance || args.length !== 4 || args.filter(x => x === '--connection').length !== 1 || args.filter(x => x === '--instance').length !== 1) {
  console.error('Usage: ares-mcp --connection /absolute/instance.json --instance UUID'); process.exit(2);
}
const local = new LocalServiceClient(file, instance);
const memory = new MemoryClient(local);
let regions: RegionHandle[] | undefined;
function unwrap<T>(r: SessionReply<T>): T { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; }
function address(value: number | string): number {
  const text = typeof value === 'string' ? value.trim().replace(/_/g, '') : String(value);
  if (!/^(?:0x[0-9a-f]+|0o[0-7]+|0b[01]+|[0-9]+)$/i.test(text)) throw new ServiceError('invalid_address');
  const result = Number(text);
  if (!Number.isSafeInteger(result) || result < 0 || result > 0xffffffff) throw new ServiceError('invalid_address');
  return result;
}
async function region(name: string): Promise<RegionHandle> {
  regions ??= await memory.regions();
  const matches = regions.filter(r => r.id === name || r.name === name);
  if (matches.length !== 1) throw new ServiceError(matches.length ? 'ambiguous_region' : 'unknown_region');
  return matches[0];
}
const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
const failure = (error: unknown) => ({ ...output({ error: error instanceof ServiceError ? error.code : 'request_failed', message: error instanceof Error ? error.message : String(error) }), isError: true });
function createServer(): McpServer {
  const server = new McpServer({ name: 'ares', version: '0.2.0' });
  const status = async () => { const reply = await local.request('session.status'); return { ...unwrap(reply), session: reply.session, epoch: reply.epoch }; };
  const call = (fn: () => Promise<unknown>) => fn().then(output, failure);
  server.registerTool('emu_status', { description: 'Generic emulator status and session identity. VI/CPU/debug fields are not implemented yet.', inputSchema: z.object({ verbose: z.boolean().default(false) }).strict(), annotations: { readOnlyHint: true } }, async ({ verbose }) => call(async () => ({ ...await status(), ...(verbose ? { limits: ['No VI coordinates, CPU registers, execution ownership or input overrides yet.'] } : {}) })));
  server.registerTool('pause', { description: 'Pause at the native outer-loop safe point.', inputSchema: z.object({}).strict() }, async () => call(async () => {
    if (!local.identity) await status(); const reply = await local.request('session.pause'); return { ...unwrap(reply), session: reply.session, epoch: reply.epoch };
  }));
  server.registerTool('resume', { description: 'Resume ordinary execution. Input override control is not implemented yet.', inputSchema: z.object({ keep_overrides: z.boolean().default(false) }).strict() }, async ({ keep_overrides }) => call(async () => {
    if (keep_overrides) throw new ServiceError('unavailable');
    if (!local.identity) await status(); const reply = await local.request('session.resume'); return { ...unwrap(reply), session: reply.session, epoch: reply.epoch };
  }));
  server.registerTool('memory_regions', { description: 'List debugger-region names, sizes, opaque IDs, access flags and session identity. Offsets are region-relative.', inputSchema: z.object({}).strict(), annotations: { readOnlyHint: true } }, async () => call(async () => {
    regions = await memory.regions(); return { regions: regions.map(({ identity, ...r }) => r), ...local.identity, max_transfer: MAX_TRANSFER };
  }));
  const addr = z.union([z.number().int().nonnegative(), z.string()]);
  server.registerTool('read_memory', { description: 'Read 1–1024 bytes at region-relative addr. Region accepts an exact name or opaque ID.', inputSchema: z.object({ region: z.string(), addr, length: z.number().int().min(1).max(MAX_TRANSFER).default(64) }).strict(), annotations: { readOnlyHint: true } }, async ({ region: name, addr, length }) => call(async () => {
    const r = await region(name), result = await memory.read(r, address(addr), length);
    return { region: r.name, region_id: r.id, addr: `0x${result.offset.toString(16).toUpperCase()}`, len: result.length, hex: result.hex, ...r.identity };
  }));
  server.registerTool('write_memory', { description: 'EXPERIMENTAL: write 1–1024 hex bytes at region-relative addr. Unavailable/read-only regions reject writes; no cache-coherence promise.', inputSchema: z.object({ region: z.string(), addr, hex_bytes: z.string().max(4096) }).strict(), annotations: { destructiveHint: true } }, async ({ region: name, addr, hex_bytes }) => call(async () => {
    const hex = hex_bytes.trim();
    if (!hex.length || hex.length > MAX_TRANSFER * 2) throw new ServiceError('invalid_request');
    const r = await region(name), result = await memory.write(r, address(addr), hexToBytes(hex));
    return { region: r.name, region_id: r.id, addr: `0x${result.offset.toString(16).toUpperCase()}`, written: result.written, ...r.identity };
  }));
  server.server.onclose = () => local.close();
  return server;
}
const handle = serveStdio(createServer, { onerror: error => console.error(error.message) });
process.on('SIGTERM', () => { local.close(); void handle.close(); });
process.on('SIGINT', () => { local.close(); void handle.close(); });
