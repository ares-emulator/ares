import type { TracePage } from './trace-contracts.js';
import { GPR_NAMES } from './n64-contracts.js';
export function traceEntries(result: TracePage) {
  return result.records.map(r => result.kind === 'pc' ? r : {
    seq: r.sequence, frame: r.frame, callsite: r.pc, target: r.target, kind: ['jal', 'jalr', 'branch_and_link'][r.kind!], link_register: r.link_register, link_value: r.link_value, sp: r.sp,
    ...(r.registers && Object.keys(r.registers).length ? { registers: Object.fromEntries(Object.entries(r.registers).map(([n, value]) => [GPR_NAMES[Number(n)], value])) } : {}),
    ...(r.f12 === undefined ? {} : { f12: { raw: r.f12, fr: r.fr } }),
  });
}
