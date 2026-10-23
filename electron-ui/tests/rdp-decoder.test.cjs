const {test} = require('node:test');
const assert = require('node:assert/strict');
const {decodeCommands, COMMAND_LENGTHS, COMMAND_NAMES} = require('../dist/services/rdp-decoder.js');
test('reference exact-capture opcode lengths, names, cross-batch words and incomplete commands', () => {
  assert.equal(COMMAND_LENGTHS.length,64); assert.equal(COMMAND_NAMES.length,64);
  for(let opcode=0;opcode<64;opcode++) {
    const length=COMMAND_LENGTHS[opcode];
    const words=Array.from({length},(_,i)=>({value:i?'0x1234':'0x'+(BigInt(opcode)<<56n).toString(16),address:'0x'+(i*8).toString(16),batch:i?1:0}));
    const c=decodeCommands(words,[{source:'DMEM'},{source:'RDRAM'}],42);
    assert.equal(c.length,1);assert.equal(c[0].words.length,length);assert.equal(c[0].frame,42);assert.equal(c[0].name,COMMAND_NAMES[opcode]);
    if(length>1) { assert.deepEqual(c[0].batches,[0,1]);const partial=decodeCommands(words.slice(0,-1),[{source:'DMEM'},{source:'RDRAM'}],42)[0];assert.equal(partial.incomplete,true);assert.equal(partial.expected_words,length); }
  }
});
test('reference OSTask layout and batch associations preserve raw metadata',()=>{
  const {exportTask,exportBatch}=require('../dist/services/rdp-decoder.js');
  const bytes=Buffer.alloc(64);bytes.writeUInt32BE(1,0);bytes.writeUInt32BE(0x80001000,16);bytes.writeUInt32BE(0x80002000,48);bytes.writeUInt32BE(0x100,52);
  const task={id:0,header:bytes.toString('hex'),cpu_pc:'0x80000000',dmem_hash_xxh64:'0x1',imem_hash_xxh64:'0x2',dmem:null};
  const parsed=exportTask(task,8*1024*1024);assert.equal(parsed.os_task.type_name,'M_GFXTASK');assert.equal(parsed.display_list,'0x80002000');assert.equal(parsed.dmem_task_header,task.header);assert.equal(parsed.header,task.header);
  assert.equal(exportTask({...task,header:'ff'.repeat(64)},8*1024*1024).os_task,null);
  const batch={task:0,current:'0x100',end:'0x200',source:'DMEM'};assert.equal(exportBatch(batch,[task]).association,'rsp_xbus_output_of_gfx_task');assert.equal(exportBatch({...batch,task:-1},[task]).task,null);
  const later={...task,id:1,header:'00000002'+task.header.slice(8)};assert.equal(exportBatch({...batch,task:1,source:'RDRAM'},[task,later]).task,0);assert.match(exportBatch({...batch,task:1,source:'RDRAM'},[task,later]).association,/non-gfx/);
});
