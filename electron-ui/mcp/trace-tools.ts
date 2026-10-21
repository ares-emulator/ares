import { traceEntries as entries } from '../services/trace-format.js';
import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply, type ServiceParams } from '../services/contracts.js';
import { GPR_NAMES, lossless } from './n64-tools.js';
import type { TracePage } from '../services/trace-contracts.js';
export function registerTraceTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity & { frame?: number }>, remember: (id: number, identity: SessionIdentity) => void, identityFor: (id: number) => SessionIdentity | undefined) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = (fn: () => Promise<unknown>) => fn().then(output, e => ({ ...output({ error: e instanceof ServiceError ? e.code : 'request_failed', message: e.message }), isError: true }));
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  const address = z.union([z.number().int().nonnegative(), z.string()]);
  const bound = address.nullable().default(null);
  const common = {
    start_frame: address.nullable().default(null), n_frames: z.number().int().min(1).max(100000).default(1), filter_start: bound, filter_end: bound,
    max_seconds: z.number().min(0.001).max(600).default(60), path: z.string().nullable().default(null), max_records: z.number().int().min(1).max(2000000).default(2000000),
    inline_limit: z.number().int().min(0).max(20000).default(20000), overwrite: z.boolean().default(false),
  };
  function canonical(value: string | number | null, fallback: string) {
    if (value === null) return fallback;
    let n = BigInt(lossless(value)); if (n >= 0x80000000n && n <= 0xffffffffn) n |= 0xffffffff00000000n;
    return lossless(n.toString());
  }
  async function page(id: number, offset: number, requested: number, identity: SessionIdentity): Promise<TracePage & { inline_limit_effective: number }> {
    const effective = Math.min(requested, 1024), records: TracePage['records'] = [];
    let result = unwrap(await client.request('n64.trace.page', identity, { operationId: id, offset, limit: Math.max(1, Math.min(64, effective)) }));
    const first = result;
    while (records.length < effective) {
      records.push(...result.records.slice(0, effective - records.length));
      if (result.next_offset === null || records.length >= effective) break;
      result = unwrap(await client.request('n64.trace.page', identity, { operationId: id, offset: result.next_offset, limit: Math.min(64, effective - records.length) }));
    }
    const next = offset + records.length;
    return { ...first, records, next_offset: next < first.record_count ? next : null, inline_limit_effective: effective };
  }
  async function trace(kind: 'pc' | 'calls', input: any) {
    const state = await status(), identity = { session: state.session, epoch: state.epoch };
    let start = state.frame;
    if (input.start_frame !== null && input.start_frame !== 'now') {
      const n = BigInt(lossless(input.start_frame));
      if (n > BigInt(Number.MAX_SAFE_INTEGER)) throw new ServiceError('invalid_target'); start = Number(n);
    }
    if (start === undefined) throw new ServiceError('unavailable');
    let mask = 0;
    for (const raw of input.registers ?? []) {
      let name = raw.toLowerCase().replace(/^\$/, ''); if (name === 'fp') name = 's8';
      let n = GPR_NAMES.indexOf(name); if (n < 0 && /^r?\d+$/.test(name)) n = Number(name.replace(/^r/, ''));
      if (n < 0 || n > 31) throw new ServiceError('invalid_register'); mask = (mask | (1 << n)) >>> 0;
    }
    const params: ServiceParams<'n64.trace.start'> = { kind, startFrame: start, frames: input.n_frames, low: canonical(input.filter_start, '0x0'), high: canonical(input.filter_end, '0xffffffffffffffff'), targetLow: canonical(input.target_start ?? null, '0x0'), targetHigh: canonical(input.target_end ?? null, '0xffffffffffffffff'), maxRecords: input.max_records, registerMask: mask, f12: input.f12 ?? false, timeoutMs: Math.ceil(input.max_seconds * 1000) };
    const started = unwrap(await client.request('n64.trace.start', identity, params));
    let operation = started.operation; remember(operation.operation_id, identity);
    while (operation.state === 'running') {
      await new Promise(resolve => setTimeout(resolve, 20));
      operation = unwrap(await client.request('execution.status', identity, { operationId: operation.operation_id }));
    }
    const result = await page(operation.operation_id, 0, input.inline_limit, identity);
    const { records, ...summary } = result;
    let artifact: Record<string, unknown> = {};
    if (input.path !== null) {
      try {
        const published = unwrap(await client.request('artifact.traceExport', identity, { operationId: operation.operation_id, path: input.path, overwrite: input.overwrite }));
        artifact = { path: published.path, artifact: published };
      } catch (e) { artifact = { file_error: e instanceof ServiceError ? e.code : 'write_failed' }; }
    }
    return { ...operation, ...artifact, ...summary, kind: operation.kind, trace_kind: kind, entries: entries(result), entries_inline: result.records.length, total_entries: result.record_count, record_capacity: started.storage_record_limit,
      interval: { start_frame: params.startFrame, end_frame: params.startFrame + params.frames, epoch: identity.epoch }, filter: { start: params.low, end: params.high },
      ...(kind === 'pc' ? { records: result.record_count } : {}),
      ...(kind === 'calls' ? { calls: result.record_count, retained: result.record_count, excluded_by_callsite: result.excluded_callsite, excluded_by_target: result.excluded_target, capture_point: 'before call instruction; delay slot may still change arguments' } : {}),
      ...(result.next_offset === null ? {} : { more: 'use trace_page with operation_id and next_offset' }) };

  }
  server.registerTool('trace_pc', { description: 'N64 per-frame executing-PC histogram, sorted by frame/PC. Owned interpreter run, bounded counters/storage and paging. Optional JSON export is atomic and bounded.', inputSchema: z.object(common).strict() }, async input => call(() => trace('pc', input)));
  server.registerTool('trace_calls', { description: 'N64 linked calls (JAL/JALR/taken branch-link) with pre-instruction targets/selected GPRs and raw f12. Owned interpreter run; optional atomic JSON export.', inputSchema: z.object({ ...common, target_start: bound, target_end: bound, registers: z.array(z.string()).max(32).nullable().default(null), f12: z.boolean().default(false) }).strict() }, async input => call(() => trace('calls', input)));
  server.registerTool('trace_page', { description: 'Page the most recent completed trace by operation ID. Returns at most 1024 entries per MCP response, with explicit next_offset; old captures return stale_trace.', inputSchema: z.object({ operation_id: z.number().int().min(1).max(Number.MAX_SAFE_INTEGER), offset: z.number().int().min(0).default(0), limit: z.number().int().min(1).max(20000).default(20000) }).strict(), annotations: { readOnlyHint: true } }, async ({ operation_id, offset, limit }) => call(async () => {
    const identity = identityFor(operation_id) ?? await status();
    const result = await page(operation_id, offset, limit, identity);
    const { records, ...summary } = result;
    return { ...summary, entries: entries(result), total: result.record_count, record_epoch: result.epoch };
  }));
}
