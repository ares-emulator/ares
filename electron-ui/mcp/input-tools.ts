import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
export function registerInputTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>, remember: (id: number, identity: SessionIdentity) => void) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = (fn: () => Promise<unknown>) => fn().then(output, e => ({ ...output({ error: e instanceof ServiceError ? e.code : 'request_failed', message: e.message }), isError: true }));
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  const buttons = ['a', 'b', 'z', 'start', 'up', 'down', 'left', 'right', 'c_up', 'c_down', 'c_left', 'c_right', 'l', 'r'] as const;
  const schema = Object.fromEntries(buttons.map(name => [name, z.boolean().default(false)]));
  server.registerTool('controller_set', { description: 'N64: replace one port’s held state, latched at the next VI or operation start. Connection-owned frame/wall leases release to host; disconnect revokes its holds.', inputSchema: z.object({ ...schema, port: z.number().int().min(1).max(4).default(1), x: z.number().int().min(-32767).max(32767).default(0), y: z.number().int().min(-32767).max(32767).default(0), timeout_frames: z.number().int().min(1).max(100000).nullable().default(null), timeout_seconds: z.number().min(0.001).max(3600).nullable().default(null) }).strict() }, async input => call(async () => {
    const values = input as Record<string, unknown>;
    let mask = 0; buttons.forEach((name, i) => { if (values[name]) mask |= 1 << i; });
    return unwrap(await client.request('n64.input.set', await status(), { port: input.port, buttons: mask, x: input.x, y: input.y, timeoutFrames: input.timeout_frames ?? 0, timeoutMs: input.timeout_seconds === null ? 0 : Math.ceil(input.timeout_seconds * 1000) }));
  }));
  server.registerTool('controller_tap', { description: 'Hold one N64 button for frames VI intervals in one owned operation, then release and stop. Timeout/cancel/hits clean up too. keep_override retains a neutral owned port; disconnect, reset and human takeover revoke it.', inputSchema: z.object({ button: z.string(), port: z.number().int().min(1).max(4).default(1), frames: z.number().int().min(1).max(100000).default(2), max_seconds: z.number().min(0.001).max(600).default(60), keep_override: z.boolean().default(false) }).strict() }, async ({ button, port, frames, max_seconds, keep_override }) => call(async () => {
    const index = buttons.indexOf(button.trim().toLowerCase().replaceAll('-', '_') as typeof buttons[number]);
    if (index < 0) throw new ServiceError('invalid_button');
    const identity = await status();
    let result = unwrap(await client.request('n64.input.tap', identity, { port, button: index, frames, timeoutMs: Math.ceil(max_seconds * 1000), keepOverride: keep_override }));
    remember(result.operation_id, identity);
    while (result.state === 'running') {
      await new Promise(resolve => setTimeout(resolve, 20));
      result = unwrap(await client.request('execution.status', identity, { operationId: result.operation_id }));
    }
    return result;
  }));
  server.registerTool('controller_release', { description: 'Release this connection’s held port, or all its ports. A foreign owner’s specific port returns busy.', inputSchema: z.object({ port: z.number().int().min(1).max(4).nullable().default(null) }).strict() }, async ({ port }) => call(async () => unwrap(await client.request('n64.input.release', await status(), { port: port ?? 0 }))));
  server.registerTool('controller_state', { description: 'N64 pending and VI-latched input for all four ports, owners, frame leases and human takeover count.', inputSchema: z.object({}).strict(), annotations: { readOnlyHint: true } }, async () => call(async () => unwrap(await client.request('n64.input.state', await status()))));
  server.registerTool('controller_host_mode', { description: 'Select whether unoverridden N64 ports receive neutral host input during controlled operations (default true).', inputSchema: z.object({ neutral_during_operations: z.boolean().default(true) }).strict() }, async ({ neutral_during_operations }) => call(async () => unwrap(await client.request('n64.input.hostMode', await status(), { neutral: neutral_during_operations }))));
}
