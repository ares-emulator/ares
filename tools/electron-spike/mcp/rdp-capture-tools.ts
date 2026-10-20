import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
export function registerRdpCaptureTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>, remember: (id: number, identity: SessionIdentity) => void) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const unwrap = <T>(r: SessionReply<T>): T => { if (!r.ok || !r.result) throw new ServiceError(r.error || 'invalid_reply'); return r.result; };
  server.registerTool('rdp_capture', { description: 'N64 exact next-VI-interval capture at RDP submission before backend dispatch, with full 64-bit words, DPC batches and RSP task snapshots at HALT-clear. Requires a frame boundary. Storage caps: 65536 words, 512 batches, 64 tasks; explicit truncation/drop counts. Optional atomic JSON export contains all retained words/batches/tasks. Inline words cap at 1024; legacy next_frame=false viewer capture is not yet available.', inputSchema: z.object({ path: z.string().nullable().default(null), next_frame: z.boolean().default(true), max_seconds: z.number().min(0.001).max(600).default(60), max_commands: z.number().int().min(1).max(1000000).default(1000000), dmem_snapshots: z.boolean().default(false), limit: z.number().int().min(0).max(4096).default(4096), overwrite: z.boolean().default(false) }).strict() }, async p => {
    try {
      if (!p.next_frame) throw new ServiceError('legacy_capture_unavailable');
      const identity = await status();
      let operation = unwrap(await client.request('n64.rdp.captureStart', identity, { maxWords: Math.min(p.max_commands, 65536), dmem: p.dmem_snapshots, timeoutMs: Math.ceil(p.max_seconds * 1000) }));
      remember(operation.operation_id, identity);
      while (operation.state === 'running') {
        await new Promise(resolve => setTimeout(resolve, 20));
        operation = unwrap(await client.request('execution.status', identity, { operationId: operation.operation_id }));
      }
      const first = unwrap(await client.request('n64.rdp.capturePage', identity, { operationId: operation.operation_id, kind: 'words', offset: 0, limit: 64 }));
      const words = first.records.slice(0, Math.min(p.limit, 1024));
      let next = words.length;
      while (next < first.word_count && next < Math.min(p.limit, 1024)) {
        const page = unwrap(await client.request('n64.rdp.capturePage', identity, { operationId: operation.operation_id, kind: 'words', offset: next, limit: Math.min(64, Math.min(p.limit, 1024) - next) }));
        words.push(...page.records); next += page.records.length;
      }
      const { records, kind, offset, next_offset, total, ...summary } = first;
      let artifact: Record<string, unknown> = {};
      if (p.path !== null) {
        try { const result = unwrap(await client.request('artifact.rdpExport', identity, { operationId: operation.operation_id, path: p.path, overwrite: p.overwrite })); artifact = { path: result.path, artifact: result }; }
        catch (e) { artifact = { file_error: e instanceof ServiceError ? e.code : 'write_failed' }; }
      }
      return output({ ...operation, ...summary, ...artifact, words, words_inline: words.length, next_offset: next < first.word_count ? next : null, inline_limit_effective: Math.min(p.limit, 1024), capture_point: 'RDP::render before backend dispatch; RSP task at HALT-clear' });
    } catch (e) { return { ...output({ error: e instanceof ServiceError ? e.code : 'request_failed' }), isError: true }; }
  });
}
