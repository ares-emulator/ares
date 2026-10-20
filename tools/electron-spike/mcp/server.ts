import { registerBatchTools } from './batch-tools.js';
import { registerStateTools } from './state-tools.js';
import { registerRecordingTools } from './recording-tools.js';
import { registerAudioTools } from './audio-tools.js';
import { screenshotSchema, registerScreenshotTools } from './screenshot-tools.js';
import { runFramesWithScreenshot } from './run-screenshot.js';
import { registerProfilerTools } from './profiler-tools.js';
import { registerDmaTools } from './dma-tools.js';
import { registerTraceTools } from './trace-tools.js';
import { registerInputTools } from './input-tools.js';
import { registerN64Tools } from './n64-tools.js';
import path from 'node:path';
import { McpServer } from '@modelcontextprotocol/server';
import { serveStdio } from '@modelcontextprotocol/server/stdio';
import * as z from 'zod/v4';
import { MAX_TRANSFER, ServiceError, type ExecutionOperation, type SessionIdentity, type SessionReply } from '../services/contracts.js';
import { LocalServiceClient } from '../services/local-endpoint.js';
import { MemoryClient, hexToBytes, type RegionHandle } from '../services/memory.js';

const args = process.argv.slice(2);
const take = (flag: string) => { const index = args.indexOf(flag); return index >= 0 ? args[index + 1] : undefined; };
const file = take('--connection'), instance = take('--instance');
if (!file || !path.isAbsolute(file) || !instance || args.length !== 4 || args.filter(x => x === '--connection').length !== 1 || args.filter(x => x === '--instance').length !== 1) {
  console.error('Usage: ares-mcp --connection /absolute/instance.json --instance UUID'); process.exit(2);
}
const local = new LocalServiceClient(file, instance);
const memory = new MemoryClient(local);
let regions: RegionHandle[] | undefined;
// Retain identity with recent operation handles, just as region handles do.
const operationIdentities = new Map<number, SessionIdentity>();
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
  server.registerTool('emu_status', { description: 'Emulator status, native VI coordinates and current execution operation.', inputSchema: z.object({ verbose: z.boolean().default(false) }).strict(), annotations: { readOnlyHint: true } }, async ({ verbose }) => call(async () => ({ ...await status(), ...(verbose ? { limits: ['Advanced captures and symbolication remain pending; VI runs support N64 and the synthetic fixture only.'] } : {}) })));
  server.registerTool('pause', { description: 'Pause at a native slice boundary and finish an owned operation with reason paused.', inputSchema: z.object({}).strict() }, async () => call(async () => {
    if (!local.identity) await status(); const reply = await local.request('session.pause'); return { ...unwrap(reply), session: reply.session, epoch: reply.epoch };
  }));
  server.registerTool('resume', { description: 'Resume ordinary execution; N64 keep_overrides preserves held controller leases.', inputSchema: z.object({ keep_overrides: z.boolean().default(false) }).strict() }, async ({ keep_overrides }) => call(async () => {
    if (!local.identity) await status(); const reply = await local.request('session.resume', undefined, keep_overrides ? { keepOverrides: true } : undefined); return { ...unwrap(reply), session: reply.session, epoch: reply.epoch };
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
  const maxSeconds = z.number().min(0.001).max(600).default(60);
  const flags = { fpr: z.boolean().default(false), cop0: z.boolean().default(false), verbose: z.boolean().default(false) };
  function rememberOperation(id: number, identity: SessionIdentity) {
    operationIdentities.delete(id); operationIdentities.set(id, identity);
    if (operationIdentities.size > 32) operationIdentities.delete(operationIdentities.keys().next().value!);
  }
  async function operation(method: 'execution.runFrames' | 'execution.runUntilFrame' | 'execution.stepInstructions', amount: number, seconds: number): Promise<ExecutionOperation> {
    if (!local.identity) await status();
    // Keep the starting identity through every poll. A ROM/reset never silently
    // rebinds this operation to the replacement session.
    const identity = { ...local.identity! };
    const params = method !== 'execution.runUntilFrame' ? { count: amount, timeoutMs: Math.ceil(seconds * 1000) } : { frame: amount, timeoutMs: Math.ceil(seconds * 1000) };
    let result = unwrap(await local.request(method, identity, params));
    rememberOperation(result.operation_id, identity);
    while (result.state === 'running') {
      await new Promise(resolve => setTimeout(resolve, 20));
      result = unwrap(await local.request('execution.status', identity, { operationId: result.operation_id }));
    }
    return result;
  }
  server.registerTool('run_frames', { description: 'Run exactly n active VI boundaries and stop; zero advances nothing. A partial starting frame counts toward the next boundary. N64 only (plus synthetic fixture).', inputSchema: z.object({ n: z.number().int().min(0).max(1000000), max_seconds: maxSeconds, screenshot_after: z.record(z.string(), z.unknown()).nullable().default(null), ...flags }).strict() }, async ({ n, max_seconds, screenshot_after, fpr, cop0 }) => call(async () => {
    if (fpr || cop0) throw new ServiceError('unavailable');
    const capture = screenshot_after === null ? null : screenshotSchema.parse(screenshot_after);
    if (capture?.inline) throw new ServiceError('invalid_request');
    return runFramesWithScreenshot(local, () => operation('execution.runFrames', n, max_seconds), capture, id => operationIdentities.get(id)!);
  }));
  server.registerTool('run_until_frame', { description: 'Run to absolute VI frame k in this epoch; past targets reject.', inputSchema: z.object({ k: z.number().int().min(0).max(Number.MAX_SAFE_INTEGER), max_seconds: maxSeconds }).strict() }, async ({ k, max_seconds }) => call(() => operation('execution.runUntilFrame', k, max_seconds)));
  server.registerTool('step', { description: 'Alias for run_frames(1), advancing to the next VI boundary.', inputSchema: z.object({}).strict() }, async () => call(() => operation('execution.runFrames', 1, 60)));
  server.registerTool('operation_status', { description: 'Poll running state or the immutable final result. The last 16 completed operations are retained.', inputSchema: z.object({ operation_id: z.number().int().min(1).max(Number.MAX_SAFE_INTEGER), ...flags }).strict(), annotations: { readOnlyHint: true } }, async ({ operation_id, fpr, cop0 }) => call(async () => {
    if (fpr || cop0) throw new ServiceError('unavailable');
    if (!local.identity) await status();
    return unwrap(await local.request('execution.status', operationIdentities.get(operation_id), { operationId: operation_id }));
  }));
  server.registerTool('cancel_operation', { description: 'Cancel this connection’s current operation, or a specific ID. UI pause can interrupt any owner.', inputSchema: z.object({ operation_id: z.number().int().min(1).max(Number.MAX_SAFE_INTEGER).nullable().default(null) }).strict() }, async ({ operation_id }) => call(async () => {
    if (operation_id === null || !local.identity) await status();
    return unwrap(await local.request('execution.cancel', operation_id === null ? undefined : operationIdentities.get(operation_id), { operationId: operation_id ?? 0 }));
  }));
  registerBatchTools(server, local, status);
  registerN64Tools(server, local, status, operation, rememberOperation);
  registerInputTools(server, local, status, rememberOperation);
  registerTraceTools(server, local, status, rememberOperation, id => operationIdentities.get(id));
  registerDmaTools(server, local, status);
  registerProfilerTools(server, local, status);
  registerScreenshotTools(server, local, status);
  registerAudioTools(server, local, status);
  registerRecordingTools(server, local, status);
  registerStateTools(server, local, status);
  server.server.onclose = () => local.close();
  return server;
}
const handle = serveStdio(createServer, { onerror: error => console.error(error.message) });
process.on('SIGTERM', () => { local.close(); void handle.close(); });
process.on('SIGINT', () => { local.close(); void handle.close(); });
