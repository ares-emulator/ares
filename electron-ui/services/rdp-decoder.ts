import type { CaptureWord, CaptureBatch } from './rdp-capture-contracts.js';
// Ported from ares-64 debug-session.cpp rdpCaptureFinish and render.cpp names,
// commit 1c9b9637476a83f7b80d3db1115a0f71529820d9 (ares ISC license).
// Exact capture lengths intentionally differ from the legacy render viewer table.
export const COMMAND_LENGTHS = [
  1,1,1,1,1,1,1,1,4,6,12,14,12,14,20,22,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
] as const;
export const COMMAND_NAMES = ["NOP", "Invalid (01)", "Invalid (02)", "Invalid (03)", "Invalid (04)", "Invalid (05)", "Invalid (06)", "Invalid (07)", "Triangle (Fill)", "Triangle (Z)", "Triangle (Tex)", "Triangle (Tex Z)", "Triangle (Shade)", "Triangle (Shade Z)", "Triangle (Shade Tex)", "Triangle (Shade Tex Z)", "Invalid (10)", "Invalid (11)", "Invalid (12)", "Invalid (13)", "Invalid (14)", "Invalid (15)", "Invalid (16)", "Invalid (17)", "Invalid (18)", "Invalid (19)", "Invalid (1a)", "Invalid (1b)", "Invalid (1c)", "Invalid (1d)", "Invalid (1e)", "Invalid (1f)", "Invalid (20)", "Invalid (21)", "Invalid (22)", "Invalid (23)", "Tex-Rect", "Tex-Rect (Flip)", "Sync Load", "Sync Pipe", "Sync Tile", "Sync Full", "Key GB", "Key R", "Convert", "Scissor", "Prim Depth", "Other Modes", "Load Tex LUT", "Invalid (31)", "Tile Size", "Load Block", "Load Tile", "Set Tile", "Fill-Rect", "Fill Color", "Fog Color", "Blend Color", "Prim Color", "Env Color", "Color Combiner", "Tex Image", "Depth Image", "Color Image"] as const;
const hex = (v: string | number) => '0x' + BigInt(v).toString(16);
export function decodeCommands(words: CaptureWord[], batches: CaptureBatch[], frame: number) {
  const commands = [];
  for (let i = 0; i < words.length;) {
    const first = words[i]!, opcode = Number(BigInt(first.value) >> 56n & 63n), length = COMMAND_LENGTHS[opcode]!;
    const taken = words.slice(i, i + length), ids: number[] = [];
    for (const w of taken) if (ids.at(-1) !== w.batch) ids.push(w.batch);
    commands.push({ seq: commands.length, opcode: hex(opcode), name: COMMAND_NAMES[opcode]!, address: hex(first.address), source: batches[first.batch]!.source,
      words: taken.map(w => hex(w.value)), batches: ids, frame, ...(taken.length < length ? { incomplete: true, expected_words: length } : {}) });
    i += taken.length;
  }
  return commands;
}

/** Reference OSTask decoding, preserving the native raw header/hash fields. */
export function exportTask(task: import('./rdp-capture-contracts.js').CaptureTask, rdramSize: number) {
  const words = Array.from({length:16}, (_,i)=>parseInt(task.header.slice(i*8,i*8+8),16));
  const ptr = (v:number) => v === 0 || (v & 0x1fffffff) < rdramSize;
  const recognized = [1,2,4,7].includes(words[0]!) && ptr(words[4]!) && ptr(words[12]!) && words[13]! < 0x800000;
  const fields = ['type','flags','ucode_boot','ucode_boot_size','ucode','ucode_size','ucode_data','ucode_data_size','dram_stack','dram_stack_size','output_buff','output_buff_size','data_ptr','data_size','yield_data_ptr','yield_data_size'];
  const os_task = recognized ? {...Object.fromEntries(fields.map((field,i)=>[field,hex(words[i]!)])),type_name:words[0]===1?'M_GFXTASK':words[0]===2?'M_AUDTASK':words[0]===4?'M_NJPEGTASK':'M_HVQMTASK'} : null;
  return {...task,index:task.id,dmem_task_header:task.header,os_task,display_list:recognized&&words[0]===1?hex(words[12]!):null,...(!recognized?{note:'task descriptor not in the standard OSTask layout; raw DMEM header and hashes provided'}:{})};
}
export function exportBatch(batch: CaptureBatch, tasks: import('./rdp-capture-contracts.js').CaptureTask[]) {
  let gfx = -1;
  for(let t=batch.task;t>=0;t--) if(parseInt(tasks[t]!.header.slice(0,8),16)===1) {gfx=t;break;}
  const association = gfx>=0 ? batch.source==='DMEM'?'rsp_xbus_output_of_gfx_task':gfx===batch.task?'temporal_latest_task_is_gfx':'temporal_latest_gfx_task (a non-gfx task started since)' : batch.task>=0?'ambiguous: no graphics task started in this interval before the batch':'unknown: the producing task started before the capture interval';
  return {...batch,dpc_current:batch.current,dpc_end:batch.end,task_index:batch.task,task:gfx>=0?gfx:batch.task>=0?batch.task:null,association};
}
