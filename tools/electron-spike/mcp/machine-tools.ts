import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity } from '../services/contracts.js';
import { registerIndex } from '../services/machine-contracts.js';
import { lossless } from './n64-tools.js';
export function registerMachineTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = (fn: () => Promise<unknown>) => fn().then(output, e => ({ ...output({ error: e instanceof ServiceError ? e.code : 'request_failed' }), isError: true }));
  const addr = z.union([z.number().int().nonnegative(), z.string()]);
  for (const [name, method, description] of [
    ['rsp_registers', 'n64.rsp.registers', 'N64 RSP scalar/vector/accumulator, status and DMA snapshot at a native safe point. Direct fields, including semaphore, are observed without MMIO read side effects. RSP instruction debugging is not yet available.'],
    ['rdp_state', 'n64.rdp.state', 'Native N64 DPC command/status snapshot. Direct registers only; does not reconstruct or capture submitted commands.'],
    ['peripheral_registers', 'n64.peripheral.registers', 'Native N64 VI/AI/PI/SI/RI fields, with no MMIO read side effects or guest execution.']
  ] as const) server.registerTool(name, { description, inputSchema: z.object({}).strict(), annotations: { readOnlyHint: true } }, () => call(async () => {
    const r = await client.request(method, await status()); if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result;
  }));
  server.registerTool('disassemble', { description: 'Disassemble 1–128 aligned N64 RDRAM instructions, strict physical/KSEG aliases. CPU view observes resident D-cache without allocation, not instruction-cache fetch state. Branch targets are lossless; indirect targets use explicitly labelled current registers. No MMIO/TLB reads.', inputSchema: z.object({ addr, count: z.number().int().min(1).max(128).default(32), view: z.enum(['cpu', 'rdram']).default('cpu') }).strict(), annotations: { readOnlyHint: true } }, p => call(async () => {
    const r = await client.request('n64.cpu.disassemble', await status(), { address: lossless(p.addr), count: p.count, view: p.view }); if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result;
  }));
  server.registerTool('cpu_set_registers', { description: 'Experimental atomic N64 GPR/PC/HI/LO/LLbit/Status mutation, only paused and operation-free. All values validate before changes. zero is read-only; aligned PC edits reset pipeline/delay slot and stale stop context. Drops JIT translations. Status uses the architectural setter. Numeric aliases/fp accepted; duplicate aliases reject.', inputSchema: z.object({ gpr: z.record(z.string(), addr).nullable().default(null), pc: addr.nullable().default(null), hi: addr.nullable().default(null), lo: addr.nullable().default(null), llbit: z.number().int().min(0).max(1).nullable().default(null), status: addr.nullable().default(null) }).strict(), annotations: { destructiveHint: true } }, p => call(async () => {
    const gpr = Object.entries(p.gpr ?? {}).map(([name, value]) => {
      const index = registerIndex(name); if (index <= 0) throw new ServiceError('invalid_register'); return { index, value: lossless(value) };
    });
    const r = await client.request('n64.cpu.setRegisters', await status(), { gpr, pc: p.pc === null ? null : lossless(p.pc), hi: p.hi === null ? null : lossless(p.hi), lo: p.lo === null ? null : lossless(p.lo), llbit: p.llbit, status: p.status === null ? null : lossless(p.status) });
    if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result;
  }));
}
