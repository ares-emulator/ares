import type { MemoryRange } from './contracts.js';
export const MEMORY_TOOL_METHODS = ['memory.snapshot.capture', 'memory.recorder.start', 'memory.recorder.stop', 'memory.capture.status', 'memory.capture.read', 'memory.capture.release', 'memory.snapshot.compare', 'memory.capture.freeze', 'memory.query.start', 'memory.query.advance', 'memory.query.page', 'memory.query.release', 'memory.snapshot.changes'] as const;
export interface CaptureMetadata { first_frame: number | null; last_frame: number | null; last_copy_ns: number; max_copy_ns: number; copied_bytes: number; transferred_bytes: number; handle: number; epoch: number; frame_kind: string; active: boolean; pending: boolean; capacity: number; duration: number; next_sequence: number; first_sequence: number; dropped: number; ranges: MemoryRange[] }
export interface MemoryMatch { offset: number; old: number | null; new: number | null; old_hex: string; new_hex: string }
export interface QueryStatus { handle: number; epoch: number; state: 'running' | 'done' | 'failed'; scanned: number; total: number; candidate_count: number; candidate_limit: number; error: string | null }
export interface QueryParams { before: number; after: number; range: number; type: 'u8' | 'u16' | 'u32' | 'f32'; endian: 'little' | 'big'; filter: 'changed' | 'unchanged' | 'increased' | 'decreased' | 'delta' | 'equals'; value: number; candidates: number }
export interface MemoryToolsMap {
  'memory.capture.freeze': { params: { handle: number; sequence: number }; result: CaptureMetadata };
  'memory.query.start': { params: QueryParams; result: QueryStatus };
  'memory.query.advance': { params: { handle: number }; result: QueryStatus };
  'memory.query.page': { params: { handle: number; cursor: number; limit: number }; result: { handle: number; epoch: number; matches: MemoryMatch[]; next_cursor: number | null } };
  'memory.query.release': { params: { handle: number }; result: { released: true } };
  'memory.snapshot.changes': { params: { before: number; after: number; range: number; cursor: number; limit: number; granularity: 'run' | 'line' | 'page' }; result: { semantics: 'contents'; granularity: 'run' | 'line' | 'page'; changes: { offset: number; length: number; continues_before: boolean; continues_after: boolean }[]; next_cursor: number | null } };
  'memory.snapshot.capture': {params: {reads: MemoryRange[]; scheduling: 'immediate' | 'boundary'}; result: CaptureMetadata};
  'memory.recorder.start': {params: {reads: MemoryRange[]; capacity: number; duration: number}; result: CaptureMetadata};
  'memory.recorder.stop': {params: {handle: number}; result: CaptureMetadata};
  'memory.capture.status': {params: {handle: number}; result: CaptureMetadata};
  'memory.capture.release': {params: {handle: number}; result: {released: true}};
  'memory.capture.read': {params: {handle: number; sequence: number; range: number; offset: number; length: number}; result: {handle:number;epoch:number;sequence:number;frame:number;range:number;offset:number;hex:string}};
  'memory.snapshot.compare': {params: {before:number;after:number;range:number;type:'u8'|'u16'|'u32'|'f32';endian:'little'|'big';filter:'changed'|'unchanged'|'increased'|'decreased'|'delta'|'equals';cursor:number;limit:number;value:number};result:{semantics:'contents';matches:{offset:number;old:number|null;new:number|null;old_hex:string;new_hex:string}[];next_cursor:number|null}};
}
const n=(v:unknown,max=0xffffffff):v is number=>Number.isSafeInteger(v)&&(v as number)>=0&&(v as number)<=max;
const keys=(p:any,k:string)=>p&&typeof p==='object'&&Object.keys(p).sort().join(',')===k;
export function encodeMemoryTools(method:string,p:any):string|undefined {
  if(method==='memory.snapshot.capture'||method==='memory.recorder.start') {
    const recording=method==='memory.recorder.start', max=recording?65536:16*1024*1024;
    if(!keys(p,recording?'capacity,duration,reads':'reads,scheduling')||!Array.isArray(p.reads)||!p.reads.length||p.reads.length>64||p.reads.some((r:any)=>!keys(r,'length,offset,region')||typeof r.region!=='string'||!/^[A-Za-z0-9_-]{1,64}$/.test(r.region)||!n(r.offset)||!n(r.length,max)||!r.length)||p.reads.reduce((s:number,r:any)=>s+r.length,0)>max)return;
    if(recording? !n(p.capacity,1024)||!p.capacity||!n(p.duration,1000000):!['immediate','boundary'].includes(p.scheduling))return;
    return (recording?` ${p.capacity} ${p.duration}`:` ${p.scheduling}`)+p.reads.map((r:any)=>` ${r.region} ${r.offset} ${r.length}`).join('');
  }
  if(method==='memory.query.start') {
    if(!keys(p,'after,before,candidates,endian,filter,range,type,value') || !n(p.before) || !p.before || !n(p.after) || !p.after || !n(p.range,63) || !n(p.candidates) || !['u8','u16','u32','f32'].includes(p.type) || !['little','big'].includes(p.endian) || !['changed','unchanged','increased','decreased','delta','equals'].includes(p.filter) || !Number.isFinite(p.value)) return;
    return ` ${p.before} ${p.after} ${p.range} ${p.type} ${p.endian} ${p.filter} ${p.value} ${p.candidates}`;
  }
  if(method==='memory.snapshot.changes') return keys(p,'after,before,cursor,granularity,limit,range') && n(p.before) && p.before>0 && n(p.after) && p.after>0 && n(p.range,63) && n(p.cursor) && n(p.limit,256) && p.limit>0 && ['run','line','page'].includes(p.granularity) ? ` ${p.before} ${p.after} ${p.range} ${p.cursor} ${p.limit} ${p.granularity}` : undefined;
  if(method==='memory.query.page') return keys(p,'cursor,handle,limit') && n(p.handle) && p.handle>0 && n(p.cursor) && n(p.limit,256) && p.limit>0 ? ` ${p.handle} ${p.cursor} ${p.limit}` : undefined;
  if(method==='memory.capture.freeze') return keys(p,'handle,sequence') && n(p.handle) && p.handle>0 && n(p.sequence) ? ` ${p.handle} ${p.sequence}` : undefined;
  if(method==='memory.capture.read')return keys(p,'handle,length,offset,range,sequence')&&n(p.handle)&&p.handle>0&&n(p.sequence)&&n(p.range,63)&&n(p.offset)&&n(p.length,16384)&&p.length>0?` ${p.handle} ${p.sequence} ${p.range} ${p.offset} ${p.length}`:undefined;
  if(method==='memory.snapshot.compare')return keys(p,'after,before,cursor,endian,filter,limit,range,type,value')&&n(p.before)&&p.before>0&&n(p.after)&&p.after>0&&n(p.range,63)&&['u8','u16','u32','f32'].includes(p.type)&&['little','big'].includes(p.endian)&&['changed','unchanged','increased','decreased','delta','equals'].includes(p.filter)&&n(p.cursor)&&n(p.limit,256)&&p.limit>0&&Number.isFinite(p.value)?` ${p.before} ${p.after} ${p.range} ${p.type} ${p.endian} ${p.filter} ${p.cursor} ${p.limit} ${p.value}`:undefined;
  return keys(p,'handle')&&n(p.handle)&&p.handle>0?` ${p.handle}`:undefined;
}
export function validMemoryTools(method:string,s:any):boolean {
  if(!s||typeof s!=='object')return false;
  if(method==='memory.capture.release'||method==='memory.query.release')return s.released===true;
  if(method==='memory.query.start'||method==='memory.query.advance') return n(s.handle) && s.handle>0 && n(s.epoch) && ['running','done','failed'].includes(s.state) && n(s.scanned) && n(s.total) && s.scanned<=s.total && n(s.candidate_count,262144) && s.candidate_limit===262144 && (s.state==='failed' ? typeof s.error==='string' : s.error===null) && (s.state!=='done'||s.scanned===s.total);
  if(method==='memory.snapshot.changes') return s.semantics==='contents' && ['run','line','page'].includes(s.granularity) && (s.next_cursor===null||n(s.next_cursor)) && Array.isArray(s.changes) && s.changes.length<=256 && s.changes.every((c:any)=>c&&n(c.offset)&&n(c.length,65536)&&c.length>0&&typeof c.continues_before==='boolean'&&typeof c.continues_after==='boolean');
  if(method==='memory.snapshot.compare'||method==='memory.query.page')return (method==='memory.query.page' ? n(s.handle)&&s.handle>0&&n(s.epoch) : s.semantics==='contents')&&(s.next_cursor===null||n(s.next_cursor))&&Array.isArray(s.matches)&&s.matches.length<=256&&s.matches.every((m:any)=>m&&n(m.offset)&&(m.old===null||Number.isFinite(m.old))&&(m.new===null||Number.isFinite(m.new))&&typeof m.old_hex==='string'&&/^(?:[0-9a-f]{2}){1,4}$/.test(m.old_hex)&&typeof m.new_hex==='string'&&m.new_hex.length===m.old_hex.length&&/^[0-9a-f]+$/.test(m.new_hex));
  if(method==='memory.capture.read')return n(s.handle)&&s.handle>0&&n(s.epoch)&&n(s.sequence)&&n(s.frame,Number.MAX_SAFE_INTEGER)&&n(s.range,63)&&n(s.offset)&&typeof s.hex==='string'&&/^(?:[0-9a-f]{2}){1,16384}$/.test(s.hex);
  if (!(s.first_frame===null||n(s.first_frame,Number.MAX_SAFE_INTEGER)) || !(s.last_frame===null||n(s.last_frame,Number.MAX_SAFE_INTEGER)) || !n(s.last_copy_ns,Number.MAX_SAFE_INTEGER) || !n(s.max_copy_ns,Number.MAX_SAFE_INTEGER) || s.last_copy_ns>s.max_copy_ns || !n(s.copied_bytes,Number.MAX_SAFE_INTEGER) || !n(s.transferred_bytes,Number.MAX_SAFE_INTEGER)) return false;
  return n(s.handle)&&s.handle>0&&n(s.epoch)&&typeof s.frame_kind==='string'&&typeof s.active==='boolean'&&typeof s.pending==='boolean'&&n(s.capacity,1024)&&s.capacity>0&&n(s.duration,1000000)&&n(s.next_sequence)&&n(s.first_sequence)&&s.first_sequence<=s.next_sequence&&n(s.dropped)&&Array.isArray(s.ranges)&&s.ranges.length>0&&s.ranges.length<=64&&s.ranges.every((r:any)=>r&&typeof r.region==='string'&&n(r.offset)&&n(r.length,16*1024*1024)&&r.length>0);
}
