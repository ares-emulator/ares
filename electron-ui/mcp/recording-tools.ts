import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity } from '../services/contracts.js';
export function registerRecordingTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const call = async (method: 'video.record.start' | 'video.record.status' | 'artifact.videoStop', params?: any) => {
    const text = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
    try { const reply = await client.request(method, await status(), params); if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply'); return text(reply.result); }
    catch (error) { return { ...text({ error: error instanceof ServiceError ? error.code : 'request_failed' }), isError: true }; }
  };
  server.registerTool('video_start', { description: 'Record bounded native presented frames without advancing emulation or using canvas credits. Raw BGR24 AVI, no audio; fps sets playback rate. Reset/disconnect revoke recordings.', inputSchema: z.object({ max_frames: z.number().int().min(1).max(36000).default(600), max_seconds: z.number().min(0.001).max(3600).default(60), max_bytes: z.number().int().min(1048576).max(268435456).default(134217728), fps: z.number().int().min(1).max(120).default(60), discard_previous: z.boolean().default(false) }).strict() }, p => call('video.record.start', { maxFrames: p.max_frames, maxBytes: p.max_bytes, fps: p.fps, timeoutMs: Math.ceil(p.max_seconds * 1000), discard: p.discard_previous }));
  server.registerTool('video_status', { description: 'Read recording limits, dimensions, actual native frame tags and stop reason.', inputSchema: z.object({}).strict(), annotations: { readOnlyHint: true } }, () => call('video.record.status'));
  server.registerTool('video_stop', { description: 'Stop and atomically export a native presented-frame recording as AVI. Failed publication retains frames for retry.', inputSchema: z.object({ path: z.string().min(1).max(4096), overwrite: z.boolean().default(false) }).strict() }, p => call('artifact.videoStop', p));
}
