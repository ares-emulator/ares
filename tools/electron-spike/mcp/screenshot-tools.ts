import { createHash } from 'node:crypto';
import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity, type SessionReply } from '../services/contracts.js';
export const screenshotSchema = z.object({ path: z.string().max(4096).nullable().default(null), source: z.enum(['framebuffer', 'presented']).default('framebuffer'), region: z.array(z.number().int().min(0).max(Number.MAX_SAFE_INTEGER)).length(4).nullable().default(null), scale: z.number().min(0.125).max(8).default(1), inline: z.boolean().default(false), overwrite: z.boolean().default(false) }).strict();
export function registerScreenshotTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const unwrap = <T>(reply: SessionReply<T>): T => { if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return reply.result; };
  const text = (value: unknown) => ({ type: 'text' as const, text: JSON.stringify(value) });
  server.registerTool('screenshot', { description: 'Native PNG screenshot: N64 framebuffer is pre-VI RDRAM; presented is the latest epoch-tagged native post-VI output, independent of canvas/video credits. Crop in source pixels then nearest-neighbor scale. Up to 4194304 pixels and 8 MiB PNG; file publication is atomic.', inputSchema: screenshotSchema }, async p => {
    let captureMetadata: Record<string, unknown> | undefined;
    let imageId: string | undefined; let identity: SessionIdentity | undefined;
    try {
      identity = await status();
      const result = unwrap(await client.request('artifact.screenshot', identity, p)); imageId = result.image_id;
      const { image_id, ...metadata } = result;
      const description = { ...metadata, ...(result.source === 'framebuffer' ? { colorDepth: result.color_depth } : { presented_at_frame: result.frame }) };
      captureMetadata = description;
      if (!p.inline) return { content: [text(description)] };
      if (!imageId) throw new ServiceError('invalid_reply');
      const bytes = Buffer.alloc(result.bytes);
      for (let offset = 0; offset < bytes.length;) {
        const offsets = Array.from({ length: Math.min(4, Math.ceil((bytes.length - offset) / 16384)) }, (_, i) => offset + i * 16384);
        const pages = await Promise.allSettled(offsets.map(async start => unwrap(await client.request('artifact.imageRead', identity, { imageId: imageId!, offset: start, length: Math.min(16384, bytes.length - start) }))));
        for (let i = 0; i < pages.length; i++) {
          const page = pages[i]; if (page.status === 'rejected') throw page.reason;
          const r = page.value;
          if (r.image_id !== imageId || r.offset !== offsets[i] || r.length !== Math.min(16384, bytes.length - offsets[i])) throw new ServiceError('invalid_reply');
          const data = Buffer.from(r.base64, 'base64'); if (data.length !== r.length) throw new ServiceError('invalid_reply');
          data.copy(bytes, r.offset); offset += r.length;
        }
      }
      if (createHash('sha256').update(bytes).digest('hex') !== result.sha256) throw new ServiceError('invalid_reply');
      return { content: [{ type: 'image' as const, mimeType: 'image/png', data: bytes.toString('base64') }, text(description)] };
    } catch (e) { return { content: [text({ ...captureMetadata, error: e instanceof ServiceError ? e.code : 'capture_failed' })], isError: true }; }
    finally { if (imageId && identity) await client.request('artifact.imageRelease', identity, { imageId }).catch(() => {}); }
  });
}
