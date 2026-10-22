import type { PreferencesBridge } from './settings/contracts.js';
import type {PluginBridge} from './plugins/contracts.js';
import type { SessionIdentity, ServiceMethod, ServiceParams, ServiceResult, SessionReply } from './services/contracts.js';

/** Only serializable application data crosses this sandbox bridge. */
export interface VideoFrame {
  width: number; height: number; sequence: number; inputSequence: number;
  nativeMs: number; queueMs: number; aspect: number; pixels: Uint8Array<ArrayBuffer>;
}
export interface FramePacket extends VideoFrame { generation: number; receivedMs: number }
export type FrameMessage = { reset: true; system: string } | (FramePacket & { reset?: false });
export interface StartOptions { rom?: string; system?: string; media?: string[]; hd?: boolean; silent?: boolean }
export interface RendererStats {
  system: string; frames: number; receivedFrames: number; maxPending: number; skipped: number;
  width: number; height: number; seconds: number; nativeFPS: number;
  bridgeToSubmitP50Ms: number; bridgeToSubmitP95Ms: number; uploadSubmitP95Ms: number;
  inputToFrameSubmitP95Ms: number; nativeQueueMs: number; glError: number; readback: number[] | null;
}
export type NativeCommand =
  | { type: 'synthetic'; hd?: boolean }
  | { type: 'input'; sequence: number; mask: number; x?: number; y?: number; rx?: number; ry?: number }
  | { type: 'key'; name: string; value: boolean }
  | { type: 'mouse'; x: number; y: number; mask: number }
  | { type: 'clear' }
  | { type: 'report'; report: RendererStats };
export interface AresBridge {
  plugins: PluginBridge;
  preferences: PreferencesBridge;
  session<M extends ServiceMethod>(method: M, identity?: SessionIdentity, params?: ServiceParams<M>): Promise<SessionReply<ServiceResult<M>>>;
  ready(): void;
  open(options: Pick<StartOptions, 'system'>): Promise<void>;
  systems(): Promise<string[]>;
  settings(): Promise<void>;
  command(data: NativeCommand): void;
  onSessionState(callback: (reply: SessionReply) => void): void;
  onStatus(callback: (text: string) => void): void;
}
