export interface RuntimeSettings { volume: number; mute: boolean; luminance: number; saturation: number; gamma: number }
export const RUNTIME_METHODS = ['runtime.settings', 'runtime.configure'] as const;
export interface RuntimeServiceMap {
  'runtime.settings': { params: undefined; result: RuntimeSettings };
  'runtime.configure': { params: RuntimeSettings; result: RuntimeSettings };
}
const bounded = (value: unknown, min: number, max: number): value is number => typeof value === 'number' && Number.isFinite(value) && value >= min && value <= max;
export function validRuntimeSettings(value: unknown): value is RuntimeSettings {
  const p = value as RuntimeSettings;
  return !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === 'gamma,luminance,mute,saturation,volume'
    && bounded(p.volume, 0, 1) && typeof p.mute === 'boolean' && bounded(p.luminance, 0, 2) && bounded(p.saturation, 0, 2) && bounded(p.gamma, 0.1, 3);
}
export function encodeRuntime(method: string, params: unknown): string | undefined {
  if (method === 'runtime.settings') return params === undefined ? '' : undefined;
  if (!validRuntimeSettings(params)) return;
  return ` ${params.volume} ${params.mute ? 1 : 0} ${params.luminance} ${params.saturation} ${params.gamma}`;
}
