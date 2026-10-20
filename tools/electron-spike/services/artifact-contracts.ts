export const ARTIFACT_METHODS = ['artifact.traceExport'] as const;
export interface TraceExportParams { operationId: number; path: string; overwrite: boolean }
export interface PublishedArtifact { path: string; bytes: number; sha256: string; session: string; epoch: number; operation_id: number; entries: number; format: 'ares-trace-v1' }
export interface ArtifactServiceMap { 'artifact.traceExport': { params: TraceExportParams; result: PublishedArtifact } }
export function validTraceExport(p: any): p is TraceExportParams {
  return !!p && typeof p === 'object' && Object.keys(p).sort().join(',') === 'operationId,overwrite,path' && Number.isSafeInteger(p.operationId) && p.operationId > 0 && typeof p.path === 'string' && p.path.length > 0 && p.path.length <= 4096 && !p.path.includes('\0') && typeof p.overwrite === 'boolean';
}
export function validArtifact(s: any): s is PublishedArtifact {
  return !!s && typeof s.path === 'string' && s.path.length > 0 && s.path.length <= 4096 && Number.isSafeInteger(s.bytes) && s.bytes > 0 && s.bytes <= 64 * 1024 * 1024 && typeof s.sha256 === 'string' && /^[a-f0-9]{64}$/.test(s.sha256) && typeof s.session === 'string' && /^[A-Za-z0-9_-]{1,64}$/.test(s.session) && Number.isSafeInteger(s.epoch) && s.epoch > 0 && Number.isSafeInteger(s.operation_id) && s.operation_id > 0 && Number.isSafeInteger(s.entries) && s.entries >= 0 && s.entries <= 262144 && s.format === 'ares-trace-v1';
}
