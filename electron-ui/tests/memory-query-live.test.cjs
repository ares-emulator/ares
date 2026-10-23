const {test} = require('node:test');
const assert = require('node:assert/strict');
const {spawn} = require('node:child_process');
const {once} = require('node:events');
const fs = require('node:fs');
const path = require('node:path');
const {NativeSessionClient} = require('../dist/services/native-client.js');
async function fixture(args, check) {
  const directory = fs.mkdtempSync('/tmp/ares-query-');
  const gba = args.includes('Game Boy Advance');
  const gb = args.includes('Game Boy');
  const rom = path.join(directory, gba ? 'fixture.gba' : gb ? 'fixture.gb' : 'fixture.z64');
  const image = Buffer.alloc(0x1000);image.writeUInt32BE(0x80371240);image.write('QUERY FIXTURE',0x20);image[0x3e]=0x45;
  if(gba) {const cart=Buffer.alloc(0x8000);cart.writeUInt32LE(0xeafffffe);cart[0xb2]=0x96;fs.writeFileSync(rom,cart);const bios=Buffer.alloc(0x4000);bios.writeUInt32LE(0xeafffffe);const biosPath=path.join(directory,'bios.bin');fs.writeFileSync(biosPath,bios);fs.writeFileSync(path.join(directory,'settings.bml'),`GameBoyAdvance\n  Firmware\n    BIOS.World: ${biosPath}\n`);}
  else {fs.writeFileSync(rom,gb?require('../fixture.cjs').makeFixture():image);fs.writeFileSync(path.join(directory,'settings.bml'),'');}
  const child=spawn(path.resolve(__dirname,'../../build-electron-make/bin/ares-electron-host'),[...args.map(a=>a==='ROM'?rom:a),'--silent','--paused','--settings',path.join(directory,'settings.bml'),'--save-dir',directory,'--control-session','queries'],{stdio:['pipe','pipe','pipe','pipe']});
  child.stdout.resume();child.stderr.resume();const closed=once(child,'close');
  const client=new NativeSessionClient(child.stdin,child.stdio[3],{session:'queries',epoch:1},()=>{});
  const request=async(method,p,owner='query-owner')=>{const r=await client.request(method,undefined,p,owner);assert.equal(r.ok,true,`${method}: ${r.error}`);return r.result};
  try {await check({client,request});}finally{client.close();child.stdin.end('quit\n');await closed;fs.rmSync(directory,{recursive:true,force:true});}
}
const defaults={range:0,type:'u8',endian:'big',filter:'changed',value:0,candidates:0};
async function complete(request,q) {while(q.state==='running')q=await request('memory.query.advance',{handle:q.handle});return q;}
test('native candidate narrowing, immutable pages, aligned typed values and content groups', async()=>{
 await fixture(['--synthetic'],async({client,request})=>{
  const region=(await request('memory.regions')).regions.find(r=>r.writable);
  const reads=[{region:region.id,offset:0,length:128}];
  const snapshot=()=>request('memory.snapshot.capture',{reads,scheduling:'immediate'});
  const before=await snapshot();await request('memory.write',{region:region.id,offset:0,hex:'aabbccdd'});const after=await snapshot();
  let q=await request('memory.query.start',{...defaults,before:before.handle,after:after.handle});
  assert.equal((await client.request('memory.query.page',undefined,{handle:q.handle,cursor:0,limit:2},'query-owner')).error,'candidate_set_not_complete');
  q=await complete(request,q);assert.equal(q.state,'done');assert.equal(q.candidate_count,4);
  const page=await request('memory.query.page',{handle:q.handle,cursor:0,limit:2});assert.deepEqual(page.matches.map(m=>m.offset),[0,1]);assert.equal(page.next_cursor,2);
  assert.equal((await client.request('memory.query.page',undefined,{handle:q.handle,cursor:0,limit:2},'other')).error,'stale_query_handle');
  assert.deepEqual((await request('memory.snapshot.changes',{before:before.handle,after:after.handle,range:0,cursor:0,limit:256,granularity:'run'})).changes,[{offset:0,length:4,continues_before:false,continues_after:false}]);
  assert.equal((await request('memory.snapshot.changes',{before:before.handle,after:after.handle,range:0,cursor:0,limit:256,granularity:'line'})).changes[0].length,16);
  await request('memory.write',{region:region.id,offset:2,hex:'0102'});const third=await snapshot();
  let narrowed=await request('memory.query.start',{...defaults,before:after.handle,after:third.handle,candidates:q.handle});
  assert.equal((await client.request('memory.query.release',undefined,{handle:q.handle},'query-owner')).error,'query_in_use');
  narrowed=await complete(request,narrowed);assert.equal(narrowed.candidate_count,2);
  await request('memory.query.release',{handle:q.handle});
  assert.deepEqual((await request('memory.query.page',{handle:narrowed.handle,cursor:0,limit:256})).matches.map(m=>m.offset),[2,3]);
  // Completed candidates retain provenance and offsets after the old bytes go away.
  await request('memory.capture.release',{handle:before.handle});await request('memory.capture.release',{handle:after.handle});
  assert.equal((await client.request('memory.query.page',undefined,{handle:narrowed.handle,cursor:0,limit:1},'query-owner')).error,'stale_memory_handle');
  const fourth=await snapshot();const empty=await complete(request,await request('memory.query.start',{...defaults,before:third.handle,after:fourth.handle,candidates:narrowed.handle}));assert.equal(empty.candidate_count,0);assert.equal((await request('memory.query.page',{handle:empty.handle,cursor:0,limit:1})).next_cursor,null);
  // Float NaN equality uses the new value; changed compares raw payload bits.
  await request('memory.write',{region:region.id,offset:0,hex:'7fc0000100000000ffffffff00000001'});const floatsBefore=await snapshot();
  await request('memory.write',{region:region.id,offset:0,hex:'3fc00000000000800000000000000000'});const floatsAfter=await snapshot();
  const floats=await complete(request,await request('memory.query.start',{...defaults,before:floatsBefore.handle,after:floatsAfter.handle,type:'f32',filter:'equals',value:1.5}));assert.equal(floats.candidate_count,1);assert.equal((await request('memory.query.page',{handle:floats.handle,cursor:0,limit:1})).matches[0].new,1.5);
  const delta=await complete(request,await request('memory.query.start',{...defaults,before:floatsBefore.handle,after:floatsAfter.handle,type:'u32',filter:'delta',value:-4294967295}));assert.equal(delta.candidate_count,1);
  const little=await complete(request,await request('memory.query.start',{...defaults,before:floatsBefore.handle,after:floatsAfter.handle,type:'u32',endian:'little',filter:'equals',value:2147483648}));assert.equal((await request('memory.query.page',{handle:little.handle,cursor:0,limit:1})).matches[0].offset,4);
  const unaligned=await request('memory.snapshot.capture',{reads:[{region:region.id,offset:1,length:8}],scheduling:'immediate'});assert.equal((await client.request('memory.query.start',undefined,{...defaults,before:unaligned.handle,after:unaligned.handle,type:'u32'},'query-owner')).error,'unaligned_range');
  await request('execution.release');assert.equal((await client.request('memory.query.advance',undefined,{handle:empty.handle},'query-owner')).error,'stale_query_handle');
 });
});
test('large native scans bound work, report candidate overflow, and enforce byte/query budgets',async()=>{
 await fixture(['--rom','ROM','--system','Nintendo 64'],async({client,request})=>{
  const region=(await request('memory.regions')).regions.find(r=>r.name==='RDRAM');assert.ok(region.size>=4*1024*1024);
  const snapshot=length=>request('memory.snapshot.capture',{reads:[{region:region.id,offset:0,length}],scheduling:'immediate'});
  const a=await snapshot(1024*1024);let q=await request('memory.query.start',{...defaults,before:a.handle,after:a.handle,filter:'unchanged'});
  q=await request('memory.query.advance',{handle:q.handle});assert.equal(q.scanned,65536);assert.equal(q.state,'running');
  q=await complete(request,q);assert.equal(q.state,'failed');assert.equal(q.error,'candidate_budget_exhausted');assert.equal(q.candidate_count,262144);
  assert.equal((await client.request('memory.query.start',undefined,{...defaults,before:a.handle,after:a.handle,candidates:q.handle},'query-owner')).error,'candidate_set_not_complete');
  for(let i=0;i<7;i++)await request('memory.query.start',{...defaults,before:a.handle,after:a.handle});
  assert.equal((await client.request('memory.query.start',undefined,{...defaults,before:a.handle,after:a.handle},'query-owner')).error,'query_budget_exhausted');
  await request('execution.release');
  const captures=[];for(let i=0;i<16;i++)captures.push(await snapshot(4*1024*1024));
  assert.equal((await client.request('memory.snapshot.capture',undefined,{reads:[{region:region.id,offset:0,length:1}],scheduling:'immediate'},'query-owner')).error,'memory_budget_exhausted');
  await request('memory.capture.release',{handle:captures.pop().handle});assert.ok((await snapshot(1)).handle);
  await request('session.reset');assert.equal((await client.request('memory.query.advance',undefined,{handle:q.handle},'query-owner')).error,'stale_query_handle');
 });
});

test('non-N64 content maps preserve cross-page runs and native frame rings remain live with overlapping owners', async()=>{
 await fixture(['--rom','ROM','--system','Game Boy Advance'],async({client,request})=>{
  const region=(await request('memory.regions')).regions.find(r=>r.name==='CPU EWRAM');assert.ok(region);
  const reads=[{region:region.id,offset:0,length:131072}];
  const before=await request('memory.snapshot.capture',{reads,scheduling:'immediate'});
  const old=(await request('memory.read',{region:region.id,offset:0,length:1})).hex;
  const changed=old==='ff'?'ee':'ff';
  for(let offset=0;offset<70000;offset+=1000)await request('memory.write',{region:region.id,offset,hex:changed.repeat(1000)});
  const after=await request('memory.snapshot.capture',{reads,scheduling:'immediate'});
  const base={before:before.handle,after:after.handle,range:0,limit:256,granularity:'run'};
  const first=await request('memory.snapshot.changes',{...base,cursor:0});assert.equal(first.changes[0].length,65536);assert.equal(first.changes[0].continues_after,true);assert.equal(first.next_cursor,65536);
  const second=await request('memory.snapshot.changes',{...base,cursor:first.next_cursor});assert.equal(second.changes[0].offset,65536);assert.equal(second.changes[0].length,4464);assert.equal(second.changes[0].continues_before,true);
  const map=await request('memory.snapshot.changes',{...base,cursor:0,granularity:'page'});assert.equal(map.changes.length,16);assert.equal(map.next_cursor,65536);
  const ranges=[{region:region.id,offset:0,length:0x6400}];
  const a=await request('memory.recorder.start',{reads:ranges,capacity:3,duration:6});
  const b=await request('memory.recorder.start',{reads:ranges,capacity:2,duration:4},'second-owner');
  assert.equal(a.frame_kind,'primary-screen-frame');
  await request('session.resume');
  let status;for(let i=0;i<200;i++){status=await request('memory.capture.status',{handle:a.handle});if(!status.active)break;await new Promise(r=>setTimeout(r,5));}
  assert.equal(status.active,false);assert.equal(status.next_sequence,6);assert.equal(status.copied_bytes,6*0x6400);assert.equal(status.transferred_bytes,0);assert.ok(status.max_copy_ns>0);
  const frozen=await request('memory.capture.freeze',{handle:a.handle,sequence:status.first_sequence});assert.equal(frozen.first_frame,status.first_frame);
  await request('memory.capture.read',{handle:frozen.handle,sequence:0,range:0,offset:0,length:4});
  assert.equal((await request('memory.capture.status',{handle:frozen.handle})).transferred_bytes,4);
  const other=await request('memory.capture.status',{handle:b.handle},'second-owner');assert.equal(other.next_sequence,4);
  await request('execution.release');assert.equal((await request('memory.capture.status',{handle:b.handle},'second-owner')).next_sequence,4);
  assert.equal((await request('session.status')).paused,false,'recorders do not pause live play');
  console.log('MEMORY_COPY_COST_GBA',JSON.stringify({sample_bytes:0x6400,samples:status.next_sequence,max_copy_ns:status.max_copy_ns,last_copy_ns:status.last_copy_ns,transferred_bytes:status.transferred_bytes}));
 });
});
