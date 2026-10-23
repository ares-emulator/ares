const {test}=require('node:test');const assert=require('node:assert/strict');const {spawn}=require('node:child_process');const {once}=require('node:events');const fs=require('node:fs');const path=require('node:path');
const {NativeSessionClient}=require('../dist/services/native-client.js');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
test('native snapshot immutability, compare paging, ring wrap and owner/epoch cleanup',async()=>{
 const saves=fs.mkdtempSync('/tmp/ares-memory-tools-'),identity={session:'tools',epoch:1};
 const child=spawn(path.resolve(__dirname,'../../build-electron-make/bin/ares-electron-host'),['--synthetic','--paused','--silent','--save-dir',saves,'--control-session','tools'],{stdio:['pipe','pipe','pipe','pipe']});child.stdout.resume();child.stderr.resume();const closed=once(child,'close');const client=new NativeSessionClient(child.stdin,child.stdio[3],identity,()=>{});
 async function request(method,p,owner='owner'){const r=await client.request(method,undefined,p,owner);assert.equal(r.ok,true,`${method}: ${r.error}`);return r.result}
 try {
  const region=(await request('memory.regions')).regions[0];const reads=[{region:region.id,offset:0,length:128}];
  const snap=await request('memory.snapshot.capture',{reads,scheduling:'immediate'});
  const original=await request('memory.capture.read',{handle:snap.handle,sequence:0,range:0,offset:0,length:8});
  await request('memory.write',{region:region.id,offset:0,hex:'abcdef01'});
  assert.equal((await request('memory.capture.read',{handle:snap.handle,sequence:0,range:0,offset:0,length:8})).hex,original.hex);
  const after=await request('memory.snapshot.capture',{reads,scheduling:'immediate'});
  const query={before:snap.handle,after:after.handle,range:0,type:'u8',endian:'big',filter:'changed',cursor:0,limit:2,value:0};
  const first=await request('memory.snapshot.compare',query);assert.equal(first.matches.length,2);assert.equal(first.next_cursor,2);
  const next=await request('memory.snapshot.compare',{...query,cursor:2});assert.equal(next.matches.length,2);
  assert.equal((await client.request('memory.capture.status',undefined,{handle:snap.handle},'foreign')).error,'stale_memory_handle');
  assert.equal((await client.request('memory.snapshot.capture',undefined,{reads,scheduling:'boundary'},'owner')).error,'paused_boundary_unavailable');
  const recording=await request('memory.recorder.start',{reads,capacity:2,duration:4});
  await request('session.resume');
  let status;for(let i=0;i<100;i++){status=await request('memory.capture.status',{handle:recording.handle});if(!status.active)break;await sleep(10)}
  assert.equal(status.active,false);assert.equal(status.next_sequence,4);assert.equal(status.dropped,2);assert.equal(status.first_sequence,2);
  const a=await request('memory.capture.read',{handle:recording.handle,sequence:2,range:0,offset:0,length:1}),b=await request('memory.capture.read',{handle:recording.handle,sequence:3,range:0,offset:0,length:1});assert.equal(b.frame,a.frame+1);
  assert.equal((await client.request('memory.capture.read',undefined,{handle:recording.handle,sequence:0,range:0,offset:0,length:1},'owner')).error,'cursor_overwritten');
  await request('execution.release');assert.equal((await client.request('memory.capture.status',undefined,{handle:snap.handle},'owner')).error,'stale_memory_handle');
  const reset=await request('memory.snapshot.capture',{reads,scheduling:'immediate'});await request('session.reset');assert.equal((await client.request('memory.capture.status',undefined,{handle:reset.handle},'owner')).error,'stale_memory_handle');
 }finally{client.close();child.stdin.end('quit\n');await closed;fs.rmSync(saves,{recursive:true,force:true})}
});
