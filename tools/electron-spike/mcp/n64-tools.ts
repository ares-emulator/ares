import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type ExecutionOperation, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
import type { Definition, Predicate, Registers, Stop } from '../services/n64-contracts.js';
import { GPR_NAMES as names } from '../services/n64-contracts.js';
const fields = ['pc', 'gpr', 'hi', 'lo', 'value'];
const ops = ['eq', 'ne', 'lt', 'le', 'gt', 'ge'] as const;
export function lossless(value: number | string): string {
  if (typeof value === 'number' && (!Number.isSafeInteger(value) || value < 0)) throw new ServiceError('invalid_address');
  const text = String(value).trim().replace(/_/g, '');
  if (!/^(?:0x[0-9a-f]+|0o[0-7]+|0b[01]+|[0-9]+)$/i.test(text) || text.length > 80) throw new ServiceError('invalid_address');
  const n = BigInt(text);
  if (n < 0 || n > 0xffffffffffffffffn) throw new ServiceError('invalid_address');
  return `0x${n.toString(16)}`;
}
export function conditions(value: unknown): Predicate[] {
  if (value == null) return [];
  let list: unknown[];
  if (Array.isArray(value)) list = value;
  else if (typeof value === 'object' && 'all' in value) {
    if (Object.keys(value).join(',') !== 'all' || !Array.isArray(value.all)) throw new ServiceError('invalid_condition');
    list = value.all;
  } else list = [value];
  if (list.length > 8) throw new ServiceError('invalid_condition');
  return list.map(item => {
    if (!item || typeof item !== 'object' || Object.keys(item).some(k => !['field', 'op', 'value', 'mask'].includes(k))) throw new ServiceError('invalid_condition');
    const c = item as Record<string, unknown>;
    if (typeof c.field !== 'string' || !['string', 'number'].includes(typeof c.value) || (c.mask !== undefined && !['string', 'number'].includes(typeof c.mask))) throw new ServiceError('invalid_condition');
    let field = c.field.toLowerCase(), index = 0;
    if (field.startsWith('gpr.') || field.startsWith('$')) {
      let reg = field.startsWith('$') ? field.slice(1) : field.slice(4);
      if (reg === 'fp') reg = 's8';
      index = names.indexOf(reg);
      if (index < 0 && /^r?\d+$/.test(reg)) index = Number(reg.replace(/^r/, ''));
      if (index < 0 || index > 31) throw new ServiceError('invalid_condition');
      field = 'gpr';
    }
    if (!fields.includes(field)) throw new ServiceError('invalid_condition');
    const aliases: Record<string, string> = { '==': 'eq', '!=': 'ne', '<': 'lt', '<=': 'le', '>': 'gt', '>=': 'ge' };
    let op = c.op ?? 'eq'; if (typeof op !== 'string') throw new ServiceError('invalid_condition');
    op = aliases[op] ?? op;
    if (!ops.includes(op as any)) throw new ServiceError('invalid_condition');
    let n = BigInt(lossless(c.value as string | number));
    if (field === 'pc' && n >= 0x80000000n && n <= 0xffffffffn) n |= 0xffffffff00000000n;
    return { field: field as Predicate['field'], index, op: op as Predicate['op'], value: lossless(n.toString()), mask: lossless((c.mask ?? '0xffffffffffffffff') as string | number) };
  });
}
export function formatRegisters(s: Registers, fpr = false, cop0 = false, verbose = false) {
  return { pc: s.pc, next_pc: s.next_pc, delay_slot: s.delay_slot, lo: s.lo, hi: s.hi,
    gpr: Object.fromEntries(names.map((name, i) => [name, s.gpr[i]])),
    ...(verbose ? { gpr_array: s.gpr } : {}), ...(cop0 ? { cop0: s.cop0 } : {}),
    ...(fpr ? { fpr: s.fpr, fcr0: s.fcr0, fcr31: s.fcr31, fpr_format: 'raw64' } : {}) };
}
export function formatStop(s: Stop, fpr = false, cop0 = false, verbose = false) {
  return { ...s, ...(s.registers ? { registers: formatRegisters(s.registers, fpr, cop0, verbose) } : {}) };
}
export function registerN64Tools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>,
  operation: (method: 'execution.runFrames' | 'execution.runUntilFrame' | 'execution.stepInstructions', amount: number, seconds: number) => Promise<ExecutionOperation>,
  rememberOperation: (id: number, identity: SessionIdentity) => void) {
  const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = (fn: () => Promise<unknown>) => fn().then(output, e => ({ ...output({ error: e instanceof ServiceError ? e.code : 'request_failed', message: e.message }), isError: true }));
  const flags = { fpr: z.boolean().default(false), cop0: z.boolean().default(false), verbose: z.boolean().default(false) };
  const addr = z.union([z.number().int().nonnegative(), z.string()]);
  const deadline = z.number().min(0.001).max(600).default(60);
  async function definitions(watch: boolean, identity: SessionIdentity): Promise<Definition[]> {
    const all: Definition[] = [];
    for (let offset = 0; offset < 256; offset += 32) {
      const page = unwrap(await client.request(watch ? 'n64.debug.watchList' : 'n64.debug.breakList', identity, { offset }));
      all.push(...page.definitions); if (offset + page.definitions.length >= page.total) break;
    }
    return all;
  }
  const display = (d: Definition) => ({ ...d, ...(d.kind === undefined ? {} : { kind: ['', 'read', 'write', 'access'][d.kind] }), condition: { all: d.condition.map(c => ({ field: c.field === 1 ? `gpr.${names[c.index]}` : fields[c.field], op: ops[c.op], value: c.value, mask: c.mask })) } });
  server.registerTool('cpu_registers', { description: 'N64 architectural registers at a native safe point. FPR values are raw 64-bit hex; COP0 Random is null.', inputSchema: z.object(flags).strict(), annotations: { readOnlyHint: true } }, async ({ fpr, cop0, verbose }) => call(async () => formatRegisters(unwrap(await client.request('n64.debug.registers', await status())), fpr, cop0, verbose)));
  server.registerTool('last_stop', { description: 'Immutable most recent N64 CPU stop. Watchpoints stop after the accessing instruction; breakpoints stop before fetch.', inputSchema: z.object(flags).strict(), annotations: { readOnlyHint: true } }, async ({ fpr, cop0, verbose }) => call(async () => formatStop(unwrap(await client.request('n64.debug.lastStop', await status())), fpr, cop0, verbose)));
  server.registerTool('step_instructions', { description: 'Execute 1–1000000 N64 CPU instructions with the interpreter. Delay slots count separately; debug hits and deadlines can stop early.', inputSchema: z.object({ n: z.number().int().min(1).max(1000000).default(1), max_seconds: deadline, ...flags }).strict() }, async ({ n, max_seconds, fpr, cop0, verbose }) => call(async () => {
    const result = await operation('execution.stepInstructions', n, max_seconds);
    const stop = result.debug_stop;
    return { ...result, ...(stop ? { debug_stop: formatStop(stop, fpr, cop0, verbose), ...(stop.registers ? { registers: formatRegisters(stop.registers, fpr, cop0, verbose) } : {}), ...(stop.hit ? { hit: stop.hit } : {}) } : {}) };
  }));
  server.registerTool('run_until_hit', { description: 'Run N64 until a CPU break/watch hit, max_frames VI boundaries, or the deadline. Hit snapshots omit stack unwinding.', inputSchema: z.object({ max_frames: z.number().int().min(1).max(1000000).default(600), max_seconds: deadline, ...flags }).strict() }, async ({ max_frames, max_seconds, fpr, cop0, verbose }) => call(async () => {
    await client.request('n64.debug.registers', await status()).then(unwrap);
    const result = await operation('execution.runFrames', max_frames, max_seconds);
    return { ...result, ...(result.debug_stop ? { debug_stop: formatStop(result.debug_stop, fpr, cop0, verbose), ...(result.debug_stop.hit ? { hit: result.debug_stop.hit } : {}) } : {}) };
  }));
  server.registerTool('break_add', { description: 'Add a before-instruction N64 RDRAM breakpoint. Physical, KSEG0 and KSEG1 aliases match; up to 8 unsigned predicates.', inputSchema: z.object({ addr, condition: z.unknown().nullable().default(null) }).strict() }, async ({ addr, condition }) => call(async () => {
    const identity = await status();
    const result = unwrap(await client.request('n64.debug.breakAdd', identity, { address: lossless(addr), condition: conditions(condition) }));
    return { ...result, breakpoints: (await definitions(false, identity)).map(display) };
  }));
  server.registerTool('watch_add', { description: 'Add a N64 CPU data watch over [addr, addr+length), including cache hits and semantic partial loads. Excludes DMA, instruction fetch and debugger accesses.', inputSchema: z.object({ addr, length: z.number().int().min(1).max(65536).default(4), kind: z.enum(['read', 'write', 'access']).default('write'), condition: z.unknown().nullable().default(null) }).strict() }, async ({ addr, length, kind, condition }) => call(async () => {
    const identity = await status(), address = lossless(addr);
    return unwrap(await client.request('n64.debug.watchAdd', identity, { address, length, kind: { read: 1, write: 2, access: 3 }[kind], condition: conditions(condition) }));
  }));
  for (const watch of [false, true]) {
    const prefix = watch ? 'watch' : 'break';
    server.registerTool(`${prefix}_list`, { description: `List N64 CPU ${prefix} definitions, predicates and hit counts.`, inputSchema: z.object({}).strict(), annotations: { readOnlyHint: true } }, async () => call(async () => ({ [watch ? 'watchpoints' : 'breakpoints']: (await definitions(watch, await status())).map(display) })));
    server.registerTool(`${prefix}_remove`, { description: `Remove a N64 CPU ${prefix} definition by ID.`, inputSchema: z.object({ id: z.number().int().min(1).max(0xffffffff) }).strict() }, async ({ id }) => call(async () => unwrap(await client.request(watch ? 'n64.debug.watchRemove' : 'n64.debug.breakRemove', await status(), { id }))));
  }
  server.registerTool('read_memory_va', { description: 'Read 1–1024 N64 RDRAM bytes via strict physical/KSEG aliases. CPU view reads resident D-cache without allocation; KSEG1 bypasses it.', inputSchema: z.object({ addr, length: z.number().int().min(1).max(1024).default(64), view: z.enum(['rdram', 'cpu']).default('rdram') }).strict(), annotations: { readOnlyHint: true } }, async ({ addr, length, view }) => call(async () => unwrap(await client.request('n64.memory.read', await status(), { address: lossless(addr), length, view }))));
  async function writeMany(writes: { address: string; view: 'cpu' | 'rdram'; hex: string }[], frames: number, seconds: number) {
    const identity = await status();
    const result = unwrap(await client.request('n64.memory.writeMany', identity, { writes, verifyAfterFrames: frames, timeoutMs: Math.ceil(seconds * 1000) }));
    let op = result.operation;
    if (op) rememberOperation(op.operation_id, identity);
    while (op?.state === 'running') {
      await new Promise(resolve => setTimeout(resolve, 20));
      op = unwrap(await client.request('execution.status', identity, { operationId: op.operation_id }));
    }
    return { ...result, operation: op };
  }
  const verification = { verify_after_frames: z.number().int().min(0).max(100000).default(0), max_seconds: deadline };
  server.registerTool('write_memory_many', { description: 'Prevalidate 1–32 ordered N64 writes, at most 1024 bytes total, before applying any. Readbacks follow all writes, so overlaps are visible. Optional bounded VI verification retains immutable final readbacks and actual stop reason. Effects are not rolled back.', inputSchema: z.object({ writes: z.array(z.object({ addr, hex: z.string().max(2048), view: z.enum(['cpu', 'rdram']).default('cpu') }).strict()).min(1).max(32), ...verification }).strict(), annotations: { destructiveHint: true } }, async ({ writes, verify_after_frames, max_seconds }) => call(() => writeMany(writes.map(w => ({ address: lossless(w.addr), hex: w.hex.trim(), view: w.view })), verify_after_frames, max_seconds)));
  server.registerTool('write_memory_va', { description: 'Write 1–1024 N64 bytes through CPU-cache or physical view. Requires initialized RDRAM; reports cache hazards and invalidates JIT, preserving resident I-cache. Optional delayed VI verification reports actual early stops and immutable readbacks.', inputSchema: z.object({ addr, hex_bytes: z.string().max(2048), view: z.enum(['cpu', 'rdram']).default('cpu'), ...verification }).strict(), annotations: { destructiveHint: true } }, async ({ addr, hex_bytes, view, verify_after_frames, max_seconds }) => call(async () => {
    const write = { address: lossless(addr), view, hex: hex_bytes.trim() };
    if (verify_after_frames) return writeMany([write], verify_after_frames, max_seconds);
    return unwrap(await client.request('n64.memory.write', await status(), write));
  }));
}

export { names as GPR_NAMES };
