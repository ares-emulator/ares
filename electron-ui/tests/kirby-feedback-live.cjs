// Opt-in private-media check. All writable game data lives in a disposable directory.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {spawn}=require('node:child_process'),{once}=require('node:events'),{randomUUID,createHash}=require('node:crypto');
const {NativeSessionClient}=require('../dist/services/native-client.js');
const {SessionRouter}=require('../dist/services/session-router.js');
const {LocalServiceHost}=require('../dist/services/local-endpoint.js');
const {StdioPeer}=require('./stdio-peer.cjs');
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
(async()=>{
 const source=process.env.ARES_PLUGIN_TEST_ROM,state=process.env.ARES_PLUGIN_TEST_STATE;
 assert.ok(source&&state,'Set ARES_PLUGIN_TEST_ROM and ARES_PLUGIN_TEST_STATE');
 const digest=p=>createHash('sha256').update(fs.readFileSync(p)).digest('hex');
 const hashes=[digest(source),digest(state)],dir=fs.mkdtempSync('/tmp/ares-kirby-feedback-');
 assert.equal(hashes[0],'2f579751d7ad2824dfd8a6141570306bfaeda1cff40139ba231c30b8591d681c');
 const output=fs.mkdtempSync(path.resolve(__dirname,'../artifacts/kirby-feedback-'));
 const rom=path.join(dir,'game.z64'),save=path.join(dir,'game.bs'),settings=path.join(dir,'settings.bml');
 fs.copyFileSync(source,rom);fs.copyFileSync(state,save);fs.writeFileSync(settings,'');
 const identity={session:randomUUID(),epoch:1};
 const child=spawn(process.env.ARES_HOST||path.resolve(__dirname,'../../build-electron-make/bin/ares-electron-host'),['--rom',rom,'--system','Nintendo 64','--settings',settings,'--silent','--paused','--save-dir',path.join(dir,'saves'),'--control-session',identity.session],{stdio:['pipe','pipe','pipe','pipe']});
 child.stdout.resume();let stderr='';child.stderr.on('data',b=>stderr+=b);const closed=once(child,'close');
 const native=new NativeSessionClient(child.stdin,child.stdio[3],identity,()=>{}),router=new SessionRouter();router.attach(native);
 const endpoint=new LocalServiceHost(router);let peer;
 const summary={rom_sha256:hashes[0],checks:{}};
 try {
  assert.equal((await native.request('session.status')).ok,true);await endpoint.listen(path.join(dir,'ipc'));
  peer=new StdioPeer(process.execPath,[path.resolve(__dirname,'../dist/mcp/server.js'),'--connection',endpoint.metadataFile,'--instance',endpoint.instance]);await peer.initialize();
  const call=async(name,args={})=>{const r=await peer.tool(name,args);assert.ok(!r.isError&&!r.error,`${name}: ${JSON.stringify(r)}`);return r};
  await call('state_load',{path:save});await call('run_frames',{n:1,max_seconds:5});
  const reads=[{address:'0x800d69c8',view:'cpu',length:0x48},{address:'0x802dd000',view:'cpu',length:0x6000},{address:'0x800e25d0',view:'cpu',length:4}];
  const a=await call('read_memory_many_va',{reads}),b=await call('read_memory_many_va',{reads});
  assert.equal(a.frame,b.frame);assert.notEqual(a.sample_id,b.sample_id);assert.ok(a.reads?.length===3,JSON.stringify(a));assert.deepEqual(a.reads,b.reads);summary.checks.atomic_bytes=24652;
  const regions=await call('memory_regions'),region=regions.regions.find(r=>r.name==='RDRAM');assert.ok(region,JSON.stringify(regions));
  const ranges=reads.map(r=>({region:region.id,offset:parseInt(r.address,16)&0x1fffffff,length:r.length}));
  const before=await call('memory_snapshot',{reads:ranges});
  await call('controller_set',{right:true,b:true,timeout_seconds:30});
  const ring=await call('memory_record_start',{reads:ranges,capacity:8,duration:60});
  await call('resume',{keep_overrides:true});
  let status;for(let i=0;i<500;i++){status=await call('memory_capture_status',{handle:ring.handle});if(!status.active)break;await sleep(20)}
  assert.equal(status.active,false);assert.equal(status.next_sequence,60);assert.equal(status.dropped,52);assert.equal(status.transferred_bytes,0);
  summary.checks.recorder=status;
  assert.equal((await native.request('session.status')).result.paused,false);
  const page=await call('memory_capture_read',{handle:ring.handle,sequence:52,range:2,length:4});
  const next=await call('memory_capture_read',{handle:ring.handle,sequence:53,range:2,length:4});
  assert.equal(next.frame,page.frame+1);
  assert.equal((await peer.tool('memory_capture_read',{handle:ring.handle,sequence:0,length:1})).error,'cursor_overwritten');
  await call('pause');await call('controller_release');
  const after=await call('memory_snapshot',{reads:ranges});
  const changes=await call('memory_changes',{before:before.handle,after:after.handle,range:1,granularity:'run'});
  assert.ok(changes.changes.length>0,JSON.stringify(changes));summary.checks.particle_changed_runs=changes.changes.length;
  let query=await call('memory_query_start',{before:before.handle,after:after.handle,range:1,type:'u32',filter:'changed'});
  while(query.state==='running')query=await call('memory_query_advance',{handle:query.handle});
  assert.equal(query.state,'done');assert.ok(query.candidate_count>0);summary.checks.query_candidates=query.candidate_count;
  const values=await call('memory_query_page',{handle:query.handle,limit:1});assert.equal(values.matches.length,1);
  let narrowed=await call('memory_query_start',{before:before.handle,after:after.handle,range:1,type:'u32',filter:'equals',value:values.matches[0].new,candidates:query.handle});
  while(narrowed.state==='running')narrowed=await call('memory_query_advance',{handle:narrowed.handle});
  assert.equal(narrowed.state,'done');assert.ok(narrowed.candidate_count>0&&narrowed.candidate_count<=query.candidate_count);
  summary.checks.narrowed_candidates=narrowed.candidate_count;
  await call('run_frames',{n:1,max_seconds:5});
  summary.checks.rdp=[];
  for(let i=0;i<4;i++){
   const capture=path.join(output,`rdp-${i}.json`);await call('rdp_capture',{path:capture,next_frame:true,max_commands:65536,max_seconds:5,limit:0});
   const file=JSON.parse(fs.readFileSync(capture));const triangles=file.commands.filter(c=>c.name.startsWith('Triangle')&&c.name.includes('Tex')).length,rects=file.commands.filter(c=>/^Tex-Rect/.test(c.name)).length;
   assert.equal(file.rom.sha256,hashes[0]);summary.checks.rdp.push({capture,triangles,rects});
   if(triangles&&rects){await call('dump_rdram',{path:path.join(output,'rdram.bin'),expect_size:8388608});break}
  }
  assert.ok(summary.checks.rdp.some(r=>r.triangles&&r.rects),JSON.stringify(summary.checks.rdp));
  assert.deepEqual([digest(source),digest(state)],hashes);summary.result='passed';fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));console.log('KIRBY_FEEDBACK_PASS',output);
 }finally{
  if(peer)await peer.close();await endpoint.close();router.detach();native.close();if(child.exitCode===null){child.stdin.end('quit\n');await Promise.race([closed,sleep(2000)]);if(child.exitCode===null){child.kill('SIGKILL');await closed}}
  fs.writeFileSync(path.join(output,'host-stderr.log'),stderr);fs.rmSync(dir,{recursive:true,force:true});
 }
})().catch(e=>{console.error(e);process.exitCode=1});
