'use strict';
// Original test microcode and ELF image; no game/library binaries or saves.
const queueText = () => {
  const b=Buffer.alloc(0x240);
  for(const [at,w] of [[0,0x241c0000],[4,0x240e0800],[8,0x0800000f],[12,0],
    [0x1c,0x24110001],[0x24,0x24110002],[0x3c,0x08000020],
    [0x80,0x08000080],[0xc4,0x24110003],
    [0x200,0x24100001],[0x204,0xac1000a4],[0x208,0x0000000d]]) b.writeUInt32BE(w,at);
  return b;
};
function elfImage(text=queueText()) {
  const b=Buffer.alloc(0x1200); b.set([0x7f,69,76,70,1,2,1]);
  b.writeUInt16BE(2,16);b.writeUInt16BE(8,18);b.writeUInt32BE(1,20);b.writeUInt32BE(0x100,32);b.writeUInt16BE(52,40);b.writeUInt16BE(40,46);b.writeUInt16BE(8,48);b.writeUInt16BE(1,50);
  const names=Buffer.from('\0.shstrtab\0.strtab\0.symtab\0.text\0.data\0.debug_info\0.debug_abbrev\0');names.copy(b,0x300);
  const strings=Buffer.from('\0rsp_queue_text_start\0rsp_fixture\0');strings.copy(b,0x380);
  function section(i,name,type,addr,offset,size,link=0,entsize=0) {
    const at=0x100+i*40;b.writeUInt32BE(names.indexOf(name),at);b.writeUInt32BE(type,at+4);b.writeUInt32BE(addr,at+12);b.writeUInt32BE(offset,at+16);b.writeUInt32BE(size,at+20);b.writeUInt32BE(link,at+24);b.writeUInt32BE(entsize,at+36);
  }
  section(1,'.shstrtab',3,0,0x300,names.length);section(2,'.strtab',3,0,0x380,strings.length);section(3,'.symtab',2,0,0x400,48,2,16);section(4,'.text',1,0x80001000,0x600,text.length);section(5,'.data',1,0x80002000,0xa00,0x100);
  function symbol(at,name,value,size,section) {b.writeUInt32BE(strings.indexOf(name),at);b.writeUInt32BE(value,at+4);b.writeUInt32BE(size,at+8);b[at+12]=0x11;b.writeUInt16BE(section,at+14);}
  symbol(0x410,'rsp_queue_text_start',0x80001000,text.length,4);symbol(0x420,'rsp_fixture',0x80002000,32,5);
  b.writeUInt32BE(0x80003000,0xa08);b.writeUInt32BE(0x80003100,0xa0c);b.writeUInt32BE(0x80002040,0xa18);b.write('rsp_fixture\0',0xa40);
  const abbrev=Buffer.from([1,0x11,1,0,0, 2,0x13,1,3,8,0x0b,0x0b,0,0, 3,0x0d,0,3,8,0x49,0x13,0x38,0x0b,0,0, 4,0x24,0,3,8,0x0b,0x0b,0,0,0]);
  const info=Buffer.alloc(128);info.writeUInt16BE(4,4);info[10]=4;let p=11;info[p++]=1;info[p++]=2;p+=info.write('rsp_queue_s\0',p);info[p++]=64;info[p++]=3;p+=info.write('counter\0',p);const ref=p;p+=4;info[p++]=32;info[p++]=0;info.writeUInt32BE(p,ref);info[p++]=4;p+=info.write('u32\0',p);info[p++]=4;info[p++]=0;info.writeUInt32BE(p-4,0);
  info.copy(b,0xc00,0,p);abbrev.copy(b,0xd00);section(6,'.debug_info',1,0,0xc00,p);section(7,'.debug_abbrev',1,0,0xd00,abbrev.length);
  text.copy(b,0x600);return b;
}
function queueConfig() {
  return {name:'original test queue',imem:{rspq_execute_command:'0x80',RSPQ_Loop:'0x3c',rspq_load_overlay:'0xc4',rspq_fetch_buffer:'0x1c',rspq_fetch_buffer_with_ptr:'0x24'},dmem:{cmds_offset:'0xa0',cmds_size:'0x100',ovl_table_offset:'0x8',ovl_idmap_offset:'0x48'},overlays:[{id:0,name:'RSPQ',commands:{}},{name:'fixture',commands:{'0x01':{name:'Fixture Signed',fmt:'n={n} addr={addr:x}',args:[{name:'n',word:0,bits:[15,0],signed:true,mul:3,div:2,add:1},{name:'addr',word:1}]},'0x11':{name:'Second Slot',args:[{name:'word',word:1}]}}}]};
}
module.exports={queueText,elfImage,queueConfig};
