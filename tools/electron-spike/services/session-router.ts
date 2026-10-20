import { MemoryDumps } from './memory-dumps.js';
import type { DumpParams } from './inspection-contracts.js';
import { RdpArtifacts } from './rdp-artifacts.js';
import { Batches } from './batches.js';
import type { BatchParams } from './batch-contracts.js';
import { StateLoading } from './state-loading.js';
import { StateArtifacts } from './state-artifacts.js';
import { StateInspection } from './state-inspection.js';
import { STATE_METHODS } from './state-contracts.js';
import { RecordingArtifacts } from './recording-artifacts.js';
import { AudioArtifacts } from './audio-artifacts.js';
import { Screenshots } from './screenshots.js';
import { SCREENSHOT_METHODS } from './screenshot-contracts.js';
import type { ScreenshotParams } from './screenshot-contracts.js';
import { TraceArtifacts } from './trace-artifacts.js';
import type { TraceExportParams } from './artifact-contracts.js';
import { METHODS, PROTOCOL, encodeRequest, ServiceError, type SessionIdentity, type ServiceMethod, type ServiceParams, type ServiceResult, type SessionReply } from './contracts.js';
import type { NativeSessionClient } from './native-client.js';

function mainCapabilities(reply: SessionReply<any>): void {
  if (reply.ok && Array.isArray(reply.result?.capabilities)) for (const method of ['experiment.batch', 'artifact.traceExport', 'artifact.rdpExport', 'artifact.dump', ...SCREENSHOT_METHODS, ...STATE_METHODS, 'artifact.audioSave', 'artifact.videoStop']) if (!reply.result.capabilities.includes(method)) reply.result.capabilities.push(method);
}

/** Electron main owns this router, independently of renderer/plugin lifetimes. */
export class SessionRouter {
  private batches = new Batches();
  private stateLoading = new StateLoading();
  private stateArtifacts = new StateArtifacts();
  private states = new StateInspection();
  private recording = new RecordingArtifacts();
  private audio = new AudioArtifacts();
  private screenshots = new Screenshots();
  private artifacts = new TraceArtifacts();
  private rdpArtifacts = new RdpArtifacts();
  private memoryDumps = new MemoryDumps();
  private nextArtifactId = 0;
  private client?: NativeSessionClient;
  private stateTimer?: ReturnType<typeof setInterval>;
  constructor(private onSessionState?: (reply: SessionReply) => void) {}
  attach(client: NativeSessionClient): void {
    this.detach(); this.client = client;
    if (!this.onSessionState) return;
    // CPU break/watch hits can stop ordinary play without a request or a video
    // frame. Main observes that state independently of the renderer's lifetime.
    let pending = false;
    this.stateTimer = setInterval(async () => {
      if (pending || this.client !== client) return;
      pending = true;
      try {
        const reply = await client.request('session.status');
        mainCapabilities(reply);
        if (this.client === client && reply.ok) this.onSessionState?.(reply);
      } catch { /* NativeSessionClient owns disconnect/timeout handling. */ }
      finally { pending = false; }
    }, 250);
    this.stateTimer.unref();
  }
  detach(): void {
    this.memoryDumps.close(); this.rdpArtifacts.close(); this.batches.close(); this.artifacts.close(); this.screenshots.close(); this.audio.close(); this.recording.close(); this.states.close(); this.stateArtifacts.close(); this.stateLoading.close();
    clearInterval(this.stateTimer); this.stateTimer = undefined;
    this.client?.close(); this.client = undefined;
  }
  async request<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>, owner = 'ui'): Promise<SessionReply<ServiceResult<M>>> {
    if (!METHODS.includes(method)) throw new ServiceError('invalid_request');
    if (method === 'execution.release') { this.memoryDumps.release(owner); this.rdpArtifacts.release(owner); this.batches.release(owner); this.artifacts.release(owner); this.screenshots.release(owner); this.audio.release(owner); this.recording.release(owner); this.states.release(owner); this.stateArtifacts.release(owner); this.stateLoading.release(owner); }
    if (!this.client) throw new ServiceError('native_host_disconnected');
    const client = this.client;
    if (method.startsWith('artifact.') || method.startsWith('state.') || method === 'experiment.batch') {
      const startingIdentity = { ...(identity ?? client.identity) };
      encodeRequest('validation', startingIdentity, method, params, owner);
      const envelope = { protocol: PROTOCOL, id: `artifact${++this.nextArtifactId}`, ...startingIdentity };
      try {
        let result;
        if (method === 'experiment.batch') result = await this.batches.run(client, this, startingIdentity, params as BatchParams, owner, () => this.client === client, child => { this.stateLoading.release(child); this.screenshots.release(child); });
        else if (method === 'state.load') result = await this.stateLoading.load(client, startingIdentity, params as { path: string; resume: boolean; keepOverrides: boolean }, owner, () => this.client === client);
        else if (method === 'state.save') result = await this.stateArtifacts.save(client, startingIdentity, params as { path: string; overwrite: boolean; metadata: boolean }, owner, () => this.client === client);
        else if (method === 'state.info' || method === 'state.list' || method === 'state.peek') result = await this.states.inspect(client, startingIdentity, method, params, owner, () => this.client === client);
        else if (method === 'artifact.dump') result = await this.memoryDumps.save(client, startingIdentity, params as DumpParams, owner, () => this.client === client);
        else if (method === 'artifact.rdpExport') result = await this.rdpArtifacts.export(client, startingIdentity, params as TraceExportParams, owner, () => this.client === client);
        else if (method === 'artifact.traceExport') result = await this.artifacts.export(client, startingIdentity, params as TraceExportParams, owner, () => this.client === client);
        else if (method === 'artifact.videoStop') { const p = params as { path: string; overwrite: boolean }; result = await this.recording.stop(client, startingIdentity, p.path, p.overwrite, owner, () => this.client === client); }
        else if (method === 'artifact.audioSave') result = await this.audio.save(client, startingIdentity, (params as { path: string }).path, owner, () => this.client === client);
        else if (method === 'artifact.screenshot') result = await this.screenshots.capture(client, startingIdentity, params as ScreenshotParams, owner, () => this.client === client);
        else {
          if (startingIdentity.session !== client.identity.session || startingIdentity.epoch !== client.identity.epoch) throw new ServiceError('stale_epoch');
          const p = params as { imageId: string; offset: number; length: number };
          result = method === 'artifact.imageRead' ? this.screenshots.read(p.imageId, p.offset, p.length, owner, startingIdentity) : this.screenshots.releaseImage(p.imageId, owner);
        }
        return { ...envelope, ...((method === 'state.load' || method === 'experiment.batch') ? { epoch: (result as { epoch: number }).epoch } : {}), ok: true, result } as SessionReply<ServiceResult<M>>;
      } catch (error) { return { ...envelope, ...client.identity, ok: false, error: error instanceof ServiceError ? error.code : 'write_failed' }; }
    }
    // The authenticated parent may pause/cancel its batch child. Other callers
    // keep their own generation and remain subject to native ownership checks.
    const child = (method === 'session.pause' || method === 'execution.cancel') ? this.batches.child(owner) : undefined;
    const reply = await client.request(method, identity, params, child ?? owner);
    const cancelled = reply.result as { owner?: string; stop_reason?: string };
    if (child && reply.ok && (method === 'session.pause' || cancelled.owner === child && cancelled.stop_reason === 'cancelled')) this.batches.interrupt(owner, method === 'session.pause' ? 'paused' : 'cancelled');
    if (method.startsWith('session.')) mainCapabilities(reply);
    if (this.client === client && reply.ok && method.startsWith('session.')) this.onSessionState?.(reply as SessionReply);
    if (this.client === client && reply.ok && method.startsWith('execution.') &&
        (method !== 'execution.status' || (reply.result as { state?: string })?.state === 'done')) {
      const state = await client.request('session.status');
      mainCapabilities(state);
      if (this.client === client && state.ok) this.onSessionState?.(state);
    }
    return reply;
  }
}
