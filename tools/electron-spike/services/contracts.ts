import { RECORDING_METHODS, encodeRecording, validRecording, type RecordingServiceMap } from './recording-contracts.js';
import { AUDIO_METHODS, encodeAudio, validAudio, type AudioServiceMap } from './audio-contracts.js';
import { FRAME_METHODS, encodeFrame, validFrame, type FrameServiceMap } from './frame-contracts.js';
import { SCREENSHOT_METHODS, validScreenshotRequest, validScreenshotResult, type ScreenshotServiceMap } from './screenshot-contracts.js';
import { ARTIFACT_METHODS, validArtifact, validTraceExport, type ArtifactServiceMap } from './artifact-contracts.js';
import { PROFILER_METHODS, encodeProfiler, validProfiler, type ProfilerMethod, type ProfilerServiceMap } from './profiler-contracts.js';
import { encodeDma, validDma, type DmaServiceMap } from './dma-contracts.js';
import { TRACE_METHODS, encodeTrace, validTracePage, type TraceServiceMap } from './trace-contracts.js';
import { INPUT_METHODS, encodeInput, validInput, type InputMethod, type InputServiceMap } from './input-contracts.js';
import { N64_METHODS, encodeN64, validN64, type N64Method, type N64ServiceMap, type Stop, validStop } from './n64-contracts.js';
/** Shared contract for Electron routing, the inspector and the future MCP adapter. */
export const PROTOCOL = 1 as const;
export const MAX_TRANSFER = 1024;
export const NATIVE_METHODS = ['session.status', 'session.pause', 'session.resume', 'session.reset', 'memory.regions', 'memory.read', 'memory.write', 'execution.runFrames', 'execution.runUntilFrame', 'execution.status', 'execution.cancel', 'execution.release', 'execution.stepInstructions', ...N64_METHODS, ...INPUT_METHODS.filter(m => m !== 'n64.input.tap'), ...TRACE_METHODS, 'n64.dma.log', ...PROFILER_METHODS, 'n64.input.tap', ...FRAME_METHODS, ...AUDIO_METHODS, ...RECORDING_METHODS] as const;
export const METHODS = [...NATIVE_METHODS, ...ARTIFACT_METHODS, ...SCREENSHOT_METHODS, 'artifact.audioSave', 'artifact.videoStop'] as const;
export type ServiceMethod = typeof METHODS[number];
export type SessionMethod = Extract<ServiceMethod, `session.${string}`>;
export interface SessionIdentity { session: string; epoch: number }
export interface SessionStatus { paused: boolean; system: string; synthetic: boolean; capabilities: ServiceMethod[]; frame?: number; frame_kind?: string; at_frame_boundary?: boolean; executor?: string; operation_id?: number | null }
export interface ExecutionOperation {
  port?: number; button?: string; held_frames?: number; released?: boolean; ownership?: 'host' | 'injected';
  operation_id: number; kind: string; owner: string; state: 'running' | 'done'; stopped: boolean;
  stop_reason: string; epoch: number; start_epoch: number; start_frame: number; target_frame: number;
  frame: number; frame_kind: string; elapsed_frames: number; start_at_frame_boundary: boolean;
  at_frame_boundary: boolean; executor: string; slices: number; elapsed_seconds: number; executed?: number; debug_stop?: Stop;
}
export interface MemoryRegion { id: string; name: string; size: number; readable: boolean; writable: boolean; addressing: 'region-offset'; unavailableReason?: string }
export interface MemoryRead { region: string; offset: number; length: number; hex: string }
export interface MemoryWritten { region: string; offset: number; written: number }
export interface ServiceMap extends RecordingServiceMap, AudioServiceMap, N64ServiceMap, InputServiceMap, TraceServiceMap, DmaServiceMap, ProfilerServiceMap, ArtifactServiceMap, FrameServiceMap, ScreenshotServiceMap {
  'execution.stepInstructions': { params: { count: number; timeoutMs: number }; result: ExecutionOperation };
  'session.status': { params: undefined; result: SessionStatus };
  'session.pause': { params: undefined; result: SessionStatus };
  'session.resume': { params: { keepOverrides: boolean } | undefined; result: SessionStatus };
  'session.reset': { params: undefined; result: SessionStatus };
  'execution.runFrames': { params: { count: number; timeoutMs: number }; result: ExecutionOperation };
  'execution.runUntilFrame': { params: { frame: number; timeoutMs: number }; result: ExecutionOperation };
  'execution.status': { params: { operationId: number }; result: ExecutionOperation };
  'execution.cancel': { params: { operationId: number }; result: ExecutionOperation };
  'execution.release': { params: undefined; result: { released: boolean } };
  'memory.regions': { params: undefined; result: { regions: MemoryRegion[] } };
  'memory.read': { params: { region: string; offset: number; length: number }; result: MemoryRead };
  'memory.write': { params: { region: string; offset: number; hex: string }; result: MemoryWritten };
}
export type ServiceParams<M extends ServiceMethod> = ServiceMap[M]['params'];
export type ServiceResult<M extends ServiceMethod> = ServiceMap[M]['result'];
export interface SessionReply<Result = SessionStatus> extends SessionIdentity {
  protocol: typeof PROTOCOL; id: string; ok: boolean; result?: Result; error?: string;
}
export interface SessionClient {
  request<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>, owner?: string): Promise<SessionReply<ServiceResult<M>>>;
}
export class ServiceError extends Error {
  constructor(public readonly code: string) { super(code); this.name = 'ServiceError'; }
}
const token = (value: unknown): value is string => typeof value === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(value);
const integer = (value: unknown): value is number => Number.isSafeInteger(value) && (value as number) >= 0;
const hex = (value: unknown): value is string => typeof value === 'string' && /^(?:[a-fA-F0-9]{2})+$/.test(value) && value.length <= MAX_TRANSFER * 2;
const keys = (p: unknown, names: string[]): boolean => !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === names.sort().join(',');
export function encodeRequest<M extends ServiceMethod>(id: string, identity: SessionIdentity, method: M, params?: ServiceParams<M>, owner = 'ui'): string {
  if (!identity || typeof identity !== 'object' || !token(id) || !token(identity.session) || !integer(identity.epoch) || identity.epoch < 1 ||
      !METHODS.includes(method) || !token(owner)) throw new ServiceError('invalid_request');
  let suffix = '';
  // Main-only methods use this validator but never enter the native wire.
  if (method.startsWith('video.record.') || method === 'artifact.videoStop') {
    const encoded = encodeRecording(method, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method.startsWith('audio.') || method === 'artifact.audioSave') {
    const encoded = encodeAudio(method, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (SCREENSHOT_METHODS.includes(method as typeof SCREENSHOT_METHODS[number])) {
    if (!validScreenshotRequest(method, params)) throw new ServiceError('invalid_request');
  } else if (method.startsWith('video.')) {
    const encoded = encodeFrame(method, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method === 'artifact.traceExport') {
    if (!validTraceExport(params)) throw new ServiceError('invalid_request');
  } else if (method === 'session.resume' && params !== undefined) {
    const p = params as { keepOverrides: boolean };
    if (!keys(p, ['keepOverrides']) || typeof p.keepOverrides !== 'boolean') throw new ServiceError('invalid_request');
    suffix = ` ${p.keepOverrides ? 1 : 0}`;
  } else if (PROFILER_METHODS.includes(method as ProfilerMethod)) {
    const encoded = encodeProfiler(method as ProfilerMethod, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method === 'n64.dma.log') {
    const encoded = encodeDma(params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method.startsWith('n64.trace.')) {
    const encoded = encodeTrace(method, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method.startsWith('n64.input.')) {
    const encoded = encodeInput(method as InputMethod, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (N64_METHODS.includes(method as N64Method)) {
    const encoded = encodeN64(method as N64Method, params);
    if (encoded === undefined) throw new ServiceError('invalid_request');
    suffix = encoded;
  } else if (method === 'memory.read') {
    const p = params as ServiceParams<'memory.read'>;
    if (!keys(p, ['region', 'offset', 'length']) || !token(p.region) || !integer(p.offset) || !integer(p.length) || p.length < 1 || p.length > MAX_TRANSFER) throw new ServiceError('invalid_request');
    suffix = ` ${p.region} ${p.offset} ${p.length}`;
  } else if (method === 'memory.write') {
    const p = params as ServiceParams<'memory.write'>;
    if (!keys(p, ['region', 'offset', 'hex']) || !token(p.region) || !integer(p.offset) || !hex(p.hex)) throw new ServiceError('invalid_request');
    suffix = ` ${p.region} ${p.offset} ${p.hex}`;
  } else if (method === 'execution.runFrames' || method === 'execution.runUntilFrame' || method === 'execution.stepInstructions') {
    const p = params as { count?: number; frame?: number; timeoutMs: number };
    const field = method === 'execution.runUntilFrame' ? 'frame' : 'count';
    if (!keys(p, [field, 'timeoutMs']) || !integer(p[field]) || !integer(p.timeoutMs) || p.timeoutMs < 1 || p.timeoutMs > 600000 || (field === 'count' && p.count! > 1000000)) throw new ServiceError('invalid_request');
    suffix = ` ${p[field]} ${p.timeoutMs}`;
  } else if (method === 'execution.status' || method === 'execution.cancel') {
    const p = params as { operationId: number };
    if (!keys(p, ['operationId']) || !integer(p.operationId)) throw new ServiceError('invalid_request');
    suffix = ` ${p.operationId}`;
  } else if (params !== undefined) throw new ServiceError('invalid_request');
  return `control ${PROTOCOL} ${id} ${identity.session} ${identity.epoch} ${method}${suffix} owner=${owner}\n`;
}
export function decodeReply<M extends ServiceMethod = 'session.status'>(line: string, method: M = 'session.status' as M): SessionReply<ServiceResult<M>> {
  let r: SessionReply<any>;
  try { r = JSON.parse(line); } catch { throw new ServiceError('invalid_reply'); }
  if (!r || r.protocol !== PROTOCOL || !token(r.id) || !token(r.session) || !integer(r.epoch) || r.epoch < 1 || typeof r.ok !== 'boolean') throw new ServiceError('invalid_reply');
  if (r.ok) {
    const s = r.result;
    let valid = false;
    if (method.startsWith('audio.') || method === 'artifact.audioSave') valid = validAudio(method, s) && (method !== 'artifact.audioSave' || s.epoch === r.epoch);
    if (method.startsWith('video.record.') || method === 'artifact.videoStop') valid = validRecording(method, s);
    else if (method.startsWith('video.')) valid = validFrame(method, s) && (method !== 'video.begin' || s.epoch === r.epoch);
    if (SCREENSHOT_METHODS.includes(method as typeof SCREENSHOT_METHODS[number])) valid = validScreenshotResult(method, s) && (method !== 'artifact.screenshot' || s.epoch === r.epoch);
    if (method === 'artifact.traceExport') valid = validArtifact(s) && s.session === r.session && s.epoch === r.epoch;
    if (N64_METHODS.includes(method as N64Method)) valid = validN64(method as N64Method, s);
    if (PROFILER_METHODS.includes(method as ProfilerMethod)) valid = validProfiler(method as ProfilerMethod, s);
    if (method === 'n64.dma.log') valid = validDma(s) && s.epoch === r.epoch;
    if (method === 'n64.trace.page') valid = validTracePage(s) && s.epoch === r.epoch;
    if (method === 'n64.trace.start' && s && integer(s.storage_record_limit) && s.storage_record_limit > 0 && s.storage_record_limit <= 262144) {
      decodeReply(JSON.stringify({ ...r, result: s.operation }), 'execution.status'); valid = true;
    }
    if (method.startsWith('n64.input.') && method !== 'n64.input.tap') valid = validInput(s);
    if (method === 'n64.input.tap') { decodeReply(JSON.stringify(r), 'execution.status'); valid = s.kind === 'controller_tap'; }
    if (method.startsWith('session.')) valid = !!s && typeof s.paused === 'boolean' && typeof s.system === 'string' && typeof s.synthetic === 'boolean' && Array.isArray(s.capabilities) && s.capabilities.every((m: ServiceMethod) => METHODS.includes(m));
    if (method === 'execution.release') valid = !!s && s.released === true;
    if (method.startsWith('execution.') && method !== 'execution.release') valid = !!s && integer(s.operation_id) && s.operation_id > 0 && token(s.owner) && typeof s.kind === 'string' && ['running', 'done'].includes(s.state) && s.stopped === (s.state === 'done') && typeof s.stop_reason === 'string' && integer(s.epoch) && integer(s.start_epoch) && integer(s.start_frame) && integer(s.target_frame) && integer(s.frame) && integer(s.elapsed_frames) && typeof s.start_at_frame_boundary === 'boolean' && typeof s.at_frame_boundary === 'boolean' && typeof s.frame_kind === 'string' && typeof s.executor === 'string' && integer(s.slices) && Number.isFinite(s.elapsed_seconds) && s.elapsed_seconds >= 0 && (s.executed === undefined || integer(s.executed)) && (s.debug_stop === undefined || validStop(s.debug_stop));
    if (method === 'memory.regions') valid = !!s && Array.isArray(s.regions) && s.regions.length <= 128 && s.regions.every((region: MemoryRegion) => region && token(region.id) && typeof region.name === 'string' && integer(region.size) && typeof region.readable === 'boolean' && typeof region.writable === 'boolean' && region.addressing === 'region-offset' && (region.unavailableReason === undefined || typeof region.unavailableReason === 'string'));
    if (method === 'memory.read') valid = !!s && token(s.region) && integer(s.offset) && hex(s.hex) && s.length === s.hex.length / 2;
    if (method === 'memory.write') valid = !!s && token(s.region) && integer(s.offset) && integer(s.written) && s.written > 0 && s.written <= MAX_TRANSFER;
    if (valid && s.kind === 'controller_tap') valid = integer(s.port) && s.port >= 1 && s.port <= 4 && ['a', 'b', 'z', 'start', 'up', 'down', 'left', 'right', 'c_up', 'c_down', 'c_left', 'c_right', 'l', 'r'].includes(s.button) && s.held_frames === s.elapsed_frames && s.released === (s.state === 'done') && ['host', 'injected'].includes(s.ownership);
    if (!valid || r.error !== undefined) throw new ServiceError('invalid_reply');
  } else if (typeof r.error !== 'string' || r.result !== undefined) throw new ServiceError('invalid_reply');
  return r;
}
