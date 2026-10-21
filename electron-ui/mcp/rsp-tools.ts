import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
import { lossless } from './n64-tools.js';
export function registerRspTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>, remember: (id: number, identity: SessionIdentity) => void) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = (fn: () => Promise<unknown>) => fn().then(output, e => ({ ...output({ error: e instanceof ServiceError ? e.code : 'request_failed' }), isError: true }));
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  server.registerTool('rsp_debug', { description: 'Acquire/release the native N64 RSP instruction debugger. Cooperative safe-point stops, interpreter execution without changing configured JIT flags, exclusive with CPU operations/GDB/uploads/batches. Owner activity renews a 30-second lease; disable/disconnect/UI/human takeover release it. Enabling pauses at the current RSP instruction boundary without clearing hardware HALT.', inputSchema: z.object({ enable: z.boolean().default(true) }).strict() }, p => call(async () => unwrap(await client.request('n64.rsp.debug', await status(), p))));
  server.registerTool('rsp_break', { description: 'Manage up to 16 aligned IMEM breakpoints, 0..0xFFC. List is read-only; edits require this connection’s active RSP debugger. Stops occur before instructions; step/run suppress the starting PC once. Add/remove are idempotent.', inputSchema: z.object({ action: z.enum(['add', 'remove', 'clear', 'list']).default('list'), addr: z.union([z.number().int().nonnegative(), z.string()]).nullable().default(null) }).strict() }, p => call(async () => unwrap(await client.request('n64.rsp.break', await status(), { action: p.action, address: p.addr === null ? null : lossless(p.addr) }))));
  server.registerTool('rsp_step', { description: 'Grant 1..1M actual RSP instructions under native ownership, then freeze. Ack includes operation handle; poll rsp_registers/operation_status. Dual issue is disabled in debug mode. CPU/devices advance only as needed by the normal scheduler; pending DMA drains synchronously. A five-second operation deadline bounds a halted/no-progress RSP.', inputSchema: z.object({ count: z.number().int().min(1).max(1000000).default(1) }).strict() }, p => call(async () => {
    const identity = await status(), op = unwrap(await client.request('n64.rsp.step', identity, p)); remember(op.operation_id, identity);
    return { stepping: p.count, fromPc: op.rsp!.start_pc, operation: op };
  }));
  server.registerTool('rsp_run', { description: 'Run the owned RSP debugger to a before-instruction breakpoint, hardware HALT/BREAK, stop/cancel, or five-second deadline. Returns an operation handle immediately; poll rsp_registers/operation_status. No separate transport pump or scheduler.', inputSchema: z.object({}).strict() }, () => call(async () => {
    const identity = await status(), op = unwrap(await client.request('n64.rsp.run', identity)); remember(op.operation_id, identity); return { running: true, operation: op };
  }));
  server.registerTool('rsp_stop', { description: 'Freeze this connection’s RSP debugger at the next native safe point. Ends its active operation with rsp_stop; preserves architectural hardware HALT, branch pipeline and breakpoint definitions.', inputSchema: z.object({}).strict() }, () => call(async () => unwrap(await client.request('n64.rsp.stop', await status()))));
}
