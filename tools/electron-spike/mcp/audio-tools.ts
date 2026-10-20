import { McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { ServiceError, type SessionClient, type SessionIdentity } from '../services/contracts.js';
export function registerAudioTools(server: McpServer, client: SessionClient, status: () => Promise<SessionIdentity>) {
  const output = (value: unknown) => ({ content: [{ type: 'text' as const, text: JSON.stringify(value) }] });
  const call = async (method: 'audio.capture' | 'audio.status' | 'artifact.audioSave', params?: any) => {
    try {
      const identity = await status(), reply = await client.request(method, identity, params);
      if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply');
      return output(reply.result);
    } catch (error) { return { ...output({ error: error instanceof ServiceError ? error.code : 'request_failed' }), isError: true }; }
  };
  server.registerTool('audio_capture', { description: 'Start a connection-owned post-mix stereo capture at 48000 Hz. Effective duration is capped at four seconds; samples freeze at the limit. Captures during controlled runs and with host output muted.', inputSchema: z.object({ seconds: z.number().min(0).max(3600).default(3) }).strict() }, ({ seconds }) => call('audio.capture', { frames: Math.floor(Math.min(seconds, 4) * 48000) }));
  server.registerTool('audio_status', { description: 'Read audio availability, sample-frame count, cap and completion. A full capture remains active until saved.', inputSchema: z.object({}).strict(), annotations: { readOnlyHint: true } }, () => call('audio.status'));
  server.registerTool('audio_save', { description: 'Save currently captured samples as a 16-bit stereo PCM WAV. Atomically replaces an existing path, matching the reference. A successful save consumes the capture; failure retains it for retry.', inputSchema: z.object({ path: z.string().min(1).max(4096) }).strict() }, p => call('artifact.audioSave', p));
}
