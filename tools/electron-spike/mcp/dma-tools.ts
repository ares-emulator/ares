import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
import { lossless } from './n64-tools.js';
export function registerDmaTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  const address = z.union([z.number().int().nonnegative(), z.string()]);
  const offset = (value: number | string) => { const n = BigInt(lossless(value)); if (n > 0x100000000n) throw new ServiceError('invalid_address'); return Number(n); };
  server.registerTool('overlay_log', { description: 'N64 bounded PI DMA provenance: initiating PC, ROM offset and RDRAM write ranges. Services-enabled apps record from boot; first query otherwise enables future recording and marks missing history. Code candidates are not proof of overlays.', inputSchema: z.object({ since_id: z.number().int().min(0).max(Number.MAX_SAFE_INTEGER).default(0), limit: z.number().int().min(1).max(4096).default(1024), code_start: address.default(0x400), code_end: address.default(0x240000) }).strict(), annotations: { readOnlyHint: true } }, async ({ since_id, limit, code_start, code_end }) => {
    try {
      const identity = await status(), codeStart = offset(code_start), codeEnd = offset(code_end), effective = Math.min(limit, 128);
      let result = unwrap(await client.request('n64.dma.log', identity, { sinceId: since_id, limit: Math.min(effective, 16), codeStart, codeEnd }));
      const entries = [...result.entries];
      while (result.more && entries.length < effective) {
        result = unwrap(await client.request('n64.dma.log', identity, { sinceId: result.next_since_id, limit: Math.min(effective - entries.length, 16), codeStart, codeEnd }));
        entries.push(...result.entries);
      }
      return output({ ...result, entries, limit_effective: effective, session: identity.session });
    } catch (e) { return { ...output({ error: e instanceof ServiceError ? e.code : 'request_failed', message: e instanceof Error ? e.message : String(e) }), isError: true }; }
  });
}
