import { ServiceError, type ExecutionOperation, type SessionClient, type SessionIdentity } from '../services/contracts.js';
import type { ScreenshotParams } from '../services/screenshot-contracts.js';

// Capture is a separate safe-point request after the run completes. Anchor it to
// the operation's epoch and require its stopped frame at the native snapshot.
export async function runFramesWithScreenshot(client: SessionClient, run: () => Promise<ExecutionOperation>, capture: ScreenshotParams | null, identity: (id: number) => SessionIdentity) {
  const result = await run();
  if (capture === null) return result;
  try {
    const reply = await client.request('artifact.screenshot', identity(result.operation_id), { ...capture, expectedFrame: result.frame });
    if (!reply.ok || !reply.result) throw new ServiceError(reply.error || 'invalid_reply');
    return { ...result, screenshot: reply.result };
  } catch (error) {
    return { ...result, screenshot_error: error instanceof ServiceError ? error.code : 'capture_failed', emulation_already_advanced: result.elapsed_frames > 0 || (result.executed ?? 0) > 0 };
  }
}
