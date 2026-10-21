import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity } from '../services/contracts.js';
import { lossless } from './n64-tools.js';
export function registerStateTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const call = async (method: 'state.info' | 'state.list' | 'state.peek' | 'state.save' | 'state.load', params: any) => {
    const text = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
    try { const reply = await client.request(method, await status(), params); if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return text(reply.result); }
    catch (error) { return { ...text({ error: error instanceof ServiceError ? error.code : 'request_failed' }), isError: true }; }
  };
  const file = z.string().min(1).max(4096), readonly = { readOnlyHint: true };
  server.registerTool('state_load', { description: 'Preflight and restore a bounded N64 native state, starting a new epoch at frame zero, paused by default. Validates version/configuration size and available hash/ROM/layout sidecars. Failed preflight stays paused; competing resume, reset, pause or input cancels upload. No automatic replay.', inputSchema: z.object({ path: file, resume: z.boolean().default(false), keep_overrides: z.boolean().default(false) }).strict() }, p => call('state.load', { path: p.path, resume: p.resume, keepOverrides: p.keep_overrides }));
  server.registerTool('state_save', { description: 'Pause at a native safe point and atomically save a bounded N64 state. Refuses active execution operations. A hash/layout/ROM sidecar is enabled by default; sidecar failure is reported separately after a successful state file.', inputSchema: z.object({ path: file, overwrite: z.boolean().default(false), metadata: z.boolean().default(true) }).strict() }, p => call('state.save', p));
  server.registerTool('state_info', { description: 'Read a bounded native-state header without pausing, serializing or restoring the active emulator. Header support does not verify machine configuration.', inputSchema: z.object({ path: file }).strict(), annotations: readonly }, p => call('state.info', p));
  server.registerTool('state_list', { description: 'Nonrecursive native-state inventory: at most 4096 directory entries, 64 MiB total metadata hashing and 48 KiB result pages. Null directory resolves configured slots 1–9. Never mutates the emulator.', inputSchema: z.object({ directory: file.nullable().default(null), offset: z.number().int().min(0).max(4096).default(0), limit: z.number().int().min(1).max(1000).default(100) }).strict(), annotations: readonly }, p => call('state.list', p));
  server.registerTool('state_peek', { description: 'Read 1–1024 saved RDRAM bytes offline using physical/KSEG aliases. Requires v153 and a hash-validated layout sidecar, or explicit current-configuration assumption. CPU-cache view is unavailable. No active-machine load/restore.', inputSchema: z.object({ path: file, addr: z.union([z.number().int().nonnegative(), z.string()]), length: z.number().int().min(1).max(1024).default(64), view: z.enum(['rdram']).default('rdram'), assume_current_configuration: z.boolean().default(false) }).strict(), annotations: readonly }, p => {
    try { return call('state.peek', { path: p.path, address: lossless(p.addr), length: p.length, view: p.view, assumeCurrentConfiguration: p.assume_current_configuration }); }
    catch (error) { return { content: [{ type: 'text' as const, text: JSON.stringify({ error: error instanceof ServiceError ? error.code : 'invalid_address' }) }], isError: true }; }
  });
}
