import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity } from '../services/contracts.js';
import { lossless } from './n64-tools.js';
import { screenshotSchema } from './screenshot-tools.js';
import type { BatchStep } from '../services/batch-contracts.js';
export function registerBatchTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const addr = z.union([z.number().int().nonnegative(), z.string()]);
  const steps = z.discriminatedUnion('kind', [
    z.object({ kind: z.literal('state_load'), path: z.string().min(1).max(4096) }).strict(),
    z.object({ kind: z.literal('write'), addr, hex: z.string().max(2048), view: z.enum(['cpu', 'rdram']).default('cpu') }).strict(),
    z.object({ kind: z.literal('read'), addr, length: z.number().int().min(1).max(1024).default(64), view: z.enum(['cpu', 'rdram']).default('rdram') }).strict(),
    z.object({ kind: z.literal('run'), n: z.number().int().min(0).max(1000000) }).strict(),
    screenshotSchema.extend({ kind: z.literal('screenshot'), path: z.string().min(1).max(4096), inline: z.literal(false).default(false) }).strict()
  ]);
  server.registerTool('batch', { description: 'Ordered native-reserved state_load/write/run/screenshot/read steps. Bounds: 32 steps, 1M total VI frames, 16 KiB read/write bytes, 8 file screenshots, 600 seconds, 48 KiB result. No nesting/parallel execution or rollback. State loads remain paused; interrupted runs retain actual progress. UI pause, human input, deadline or disconnect revoke ownership. Reload a baseline explicitly for each candidate.', inputSchema: z.object({ steps: z.array(steps).min(1).max(32), max_seconds: z.number().min(0.001).max(600).default(60), stop_on_error: z.boolean().default(true) }).strict() }, async p => {
    const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
    try {
      const normalized: BatchStep[] = p.steps.map(s => {
        if (s.kind !== 'write' && s.kind !== 'read') return s;
        const { addr, ...rest } = s; return { ...rest, address: lossless(addr), ...(s.kind === 'write' ? { hex: s.hex.trim() } : {}) };
      });
      const r = await client.request('experiment.batch', await status(), { steps: normalized, timeoutMs: Math.ceil(p.max_seconds * 1000), stopOnError: p.stop_on_error });
      if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply');
      return output(r.result);
    } catch (e) { return { ...output({ error: e instanceof ServiceError ? e.code : 'batch_failed' }), isError: true }; }
  });
}
