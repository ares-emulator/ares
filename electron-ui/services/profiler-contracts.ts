export const PROFILER_METHODS = ['n64.profiler.control', 'n64.profiler.clear', 'n64.profiler.top'] as const;
export type ProfilerMethod = typeof PROFILER_METHODS[number];
export interface ProfilerState { enabled: boolean; owner: string }
export interface ProfilerEntry { addr: string; name: string; callCount: number; inclCycles: number; exclCycles: number; waitCycles: number; exclBytesIn: number; exclBytesOut: number }
export interface ProfilerTop extends ProfilerState {
  window: 'continuous' | 'frame'; entries: ProfilerEntry[]; timebase_hz: 187500000; cycle_unit: 'master_ticks'; frame_window: 'vi_origin_swaps';
  symbols_loaded: false; wait_classification: 'unavailable'; function_capacity: 65536; frame_count: number; accumulation_frozen: boolean;
  dropped_function_updates: number; dropped_stack_frames: number; truncated: boolean; total_functions: number; more: boolean;
}
export interface ProfilerServiceMap {
  'n64.profiler.control': { params: { enable: boolean }; result: ProfilerState };
  'n64.profiler.clear': { params: undefined; result: ProfilerState };
  'n64.profiler.top': { params: { window: 'continuous' | 'frame'; limit: number }; result: ProfilerTop };
}
const integer = (n: unknown, max = Number.MAX_SAFE_INTEGER): n is number => Number.isSafeInteger(n) && (n as number) >= 0 && (n as number) <= max;
const keys = (p: any, names: string) => p && typeof p === 'object' && Object.keys(p).sort().join(',') === names;
export function encodeProfiler(method: ProfilerMethod, p: any): string | undefined {
  if (method === 'n64.profiler.clear') return p === undefined ? '' : undefined;
  if (method === 'n64.profiler.control') return keys(p, 'enable') && typeof p.enable === 'boolean' ? ` ${+p.enable}` : undefined;
  if (keys(p, 'limit,window') && ['continuous', 'frame'].includes(p.window) && integer(p.limit, 64) && p.limit > 0) return ` ${p.window} ${p.limit}`;
}
export function validProfiler(method: ProfilerMethod, s: any): boolean {
  if (!s || typeof s.enabled !== 'boolean' || typeof s.owner !== 'string' || (s.owner !== '' && !/^[A-Za-z0-9_-]{1,64}$/.test(s.owner)) || s.enabled !== !!s.owner) return false;
  if (method !== 'n64.profiler.top') return true;
  return ['continuous', 'frame'].includes(s.window) && s.timebase_hz === 187500000 && s.cycle_unit === 'master_ticks' && s.frame_window === 'vi_origin_swaps' && s.symbols_loaded === false && s.wait_classification === 'unavailable' && s.function_capacity === 65536 && integer(s.frame_count, 1000) && s.accumulation_frozen === (s.frame_count === 1000) && integer(s.dropped_function_updates) && integer(s.dropped_stack_frames) && s.truncated === !!(s.dropped_function_updates || s.dropped_stack_frames) && integer(s.total_functions, 65536) && typeof s.more === 'boolean' && Array.isArray(s.entries) && s.entries.length <= 64 && s.total_functions >= s.entries.length && s.more === (s.total_functions > s.entries.length) && s.entries.every((e: ProfilerEntry) => e && typeof e.addr === 'string' && /^0x[0-9a-f]{1,8}$/.test(e.addr) && BigInt(e.addr) <= 0xffffffffn && typeof e.name === 'string' && ['callCount', 'inclCycles', 'exclCycles', 'waitCycles', 'exclBytesIn', 'exclBytesOut'].every(k => integer(e[k as keyof ProfilerEntry])) && e.exclCycles <= e.inclCycles && e.waitCycles <= e.inclCycles);
}
