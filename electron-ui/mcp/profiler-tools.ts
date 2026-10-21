import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
export function registerProfilerTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  const call = async (action: () => Promise<unknown>) => {
    try { return output(await action()); }
    catch (e) { return { ...output({ error: e instanceof ServiceError ? e.code : 'request_failed' }), isError: true }; }
  };
  server.registerTool('profiler_control', { description: 'Enable or disable connection-owned N64 CPU call-cost profiling. Works with the recompiler. Disconnect disables capture and retains collected statistics.', inputSchema: z.object({ enable: z.boolean().default(true) }).strict() }, async ({ enable }) => call(async () => unwrap(await client.request('n64.profiler.control', await status(), { enable }))));
  server.registerTool('profiler_clear', { description: 'Clear N64 CPU profiler statistics and rebase open calls. Requires capture ownership when enabled.', inputSchema: z.object({}).strict() }, async () => call(async () => ({ ...unwrap(await client.request('n64.profiler.clear', await status())), cleared: true })));
  server.registerTool('cpu_profiler_top', { description: 'N64 CPU functions ordered by exclusive cost. Cycle fields are 187.5 MHz master ticks. Frame means last VI-origin swap; open calls commit on return, swap or disable. Symbol names and wait classification are unavailable.', inputSchema: z.object({ window: z.enum(['continuous', 'frame']).default('continuous'), limit: z.number().int().min(1).max(100000).default(32) }).strict(), annotations: { readOnlyHint: true } }, async ({ window, limit }) => call(async () => ({ ...unwrap(await client.request('n64.profiler.top', await status(), { window, limit: Math.min(limit, 64) })), limit_effective: Math.min(limit, 64) })));
}
