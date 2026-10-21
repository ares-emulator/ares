import type { AresBridge, RendererStats } from './bridge.js';
declare global {
  interface Window {
    ares: AresBridge;
    /** Opt-in smoke harness diagnostics. */
    stats(): RendererStats;
    sendInput(mask: number, x?: number, y?: number, rx?: number, ry?: number): void;
  }
}
