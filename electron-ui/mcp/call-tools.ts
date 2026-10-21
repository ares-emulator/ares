import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
import { lossless } from './n64-tools.js';
export function registerCallTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>, remember: (id: number, identity: SessionIdentity) => void) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = (fn: () => Promise<unknown>) => fn().then(output, e => ({ ...output({ error: e instanceof ServiceError ? e.code : 'request_failed' }), isError: true }));
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  const address = z.union([z.number().int().nonnegative(), z.string()]);
  server.registerTool('call_function', { description: 'Experimental bounded N64 RDRAM function call with up to four raw integer arguments and t9=entry. Requires paused operation-free CPU, aligned RDRAM stack, no delay/EXL/ERL context. Private before-fetch return gate, no code patches; valid JR/JALR return plus completed delay slot and original SP required. Success returns v0/v1 and restores caller GPR/HI/LO/PC, Status, raw FPR/FCR31; LLbit cleared. Other COP0/time, memory, device/interrupt effects remain. Interruptions retain the actual stop and caller context for explicit call_recover. Bound: 1M instructions, 10000 VI, 600 seconds.', inputSchema: z.object({ addr: address, args: z.array(address).max(4).nullable().default(null), max_instructions: z.number().int().min(1).max(1000000).default(100000), max_frames: z.number().int().min(1).max(10000).default(60), max_seconds: z.number().min(0.001).max(600).default(5) }).strict() }, p => call(async () => {
    const identity = await status();
    let op = unwrap(await client.request('n64.cpu.call', identity, { address: lossless(p.addr), args: (p.args ?? []).map(lossless), maxInstructions: p.max_instructions, maxFrames: p.max_frames, timeoutMs: Math.ceil(p.max_seconds * 1000) }));
    remember(op.operation_id, identity);
    while (op.state === 'running') { await new Promise(resolve => setTimeout(resolve, 20)); op = unwrap(await client.request('execution.status', identity, { operationId: op.operation_id })); }
    return { ...op, ...op.call };
  }));
  server.registerTool('call_recover', { description: 'Explicitly restore/discard the retained CPU caller context after an interrupted call. Paused operation-free same epoch only; original owner/UI, or a new authenticated connection after owner disconnect. Restores only documented CPU state and clears LLbit; memory/device/time/other COP0 effects remain. New calls require prior recovery/discard. Reset/state load invalidate the context.', inputSchema: z.object({ restore: z.boolean().default(true) }).strict() }, p => call(async () => unwrap(await client.request('n64.cpu.callRecover', await status(), p))));
}
