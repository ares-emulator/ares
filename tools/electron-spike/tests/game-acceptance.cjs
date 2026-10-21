'use strict';
// Migrated live_acceptance/live_paper_cuts scenarios against the NEW host + MCP.
// Reads the supplied ROM/save; all emulator writes and artifacts use scratch.
const assert=require('node:assert/strict');
const fs=require('node:fs');const path=require('node:path');const crypto=require('node:crypto');
const {spawn}=require('node:child_process');const {once}=require('node:events');
const {NativeSessionClient}=require('../dist/services/native-client.js');
const {SessionRouter}=require('../dist/services/session-router.js');
const {LocalServiceHost}=require('../dist/services/local-endpoint.js');
const {StdioPeer}=require('./stdio-peer.cjs');
const hash=file=>crypto.createHash('sha256').update(fs.readFileSync(file)).digest('hex');
async function main() {
  const rom=path.resolve(process.argv[2]),dir=path.resolve(process.argv[3]);
  fs.mkdirSync(dir,{recursive:true});const originalSave=rom.replace(/\.[^.]+$/,'.eeprom');
  const before={rom:hash(rom),save:fs.existsSync(originalSave)?hash(originalSave):null};
  const saves=path.join(dir,'saves');fs.mkdirSync(path.join(saves,'Nintendo 64'),{recursive:true});
  if(before.save) fs.copyFileSync(originalSave,path.join(saves,'Nintendo 64',path.basename(originalSave)));
  const settings=path.join(dir,'settings.bml');fs.writeFileSync(settings,'');
  const identity={session:'task9-game-acceptance',epoch:1};
  const host=path.resolve(__dirname,'../../../build-electron-make/bin/ares-electron-host');
  const child=spawn(host,['--rom',rom,'--system','Nintendo 64','--settings',settings,'--save-dir',saves,'--silent','--paused','--control-session',identity.session],{stdio:['pipe','pipe','pipe','pipe']});
  const closed=once(child,'close');let log='';child.stdout.resume();child.stderr.on('data',b=>{log=(log+b).slice(-50000);});
  const native=new NativeSessionClient(child.stdin,child.stdio[3],identity,()=>{}),router=new SessionRouter();router.attach(native);
  const endpoint=new LocalServiceHost(router),ipc=fs.mkdtempSync('/tmp/ares-game-ipc-');let peer;const evidence=[];
  const check=(name,detail)=>{evidence.push({name,pass:true,detail});console.log('PASS '+name);};
  try {
    const status=await native.request('session.status');assert.equal(status.ok,true,JSON.stringify(status));
    await endpoint.listen(ipc);peer=new StdioPeer(process.execPath,[path.resolve(__dirname,'../dist/mcp/server.js'),'--connection',endpoint.metadataFile,'--instance',endpoint.instance]);await peer.initialize();
    async function tool(name,args={},allowError=false) {
      const r=await peer.request('tools/call',{name,arguments:args},650000);assert.equal(r.error,undefined,JSON.stringify(r));
      const value=JSON.parse(r.result.content[0].text);if(!allowError) assert.ok(!r.result.isError,`${name}: ${JSON.stringify(value)}`);return value;
    }
    const loadedStatus=await tool('emu_status',{verbose:true});assert.equal(loadedStatus.loaded,true);assert.equal(loadedStatus.save.type,'EEPROM');assert.equal(loadedStatus.save.size,2048);assert.equal(loadedStatus.save.loaded_from_file,true);assert.ok(loadedStatus.save.resolved_path.startsWith(dir));check('real game EEPROM uses isolated copied save',loadedStatus.save);
    const list=(await peer.request('tools/list',{})).result.tools;assert.equal(list.length,68);check('actual 68-tool registry');
    const boot=await tool('run_frames',{n:1800,max_seconds:60});assert.equal(boot.stop_reason,'frames_done');check('real game boot: 1800 VI frames',{frame:boot.frame,executor:boot.executor});
    await tool('controller_tap',{button:'start',frames:4});await tool('run_frames',{n:150,max_seconds:60});
    for(let i=0;i<3;++i){await tool('controller_tap',{button:'a',frames:4});await tool('run_frames',{n:150,max_seconds:60});}
    await tool('run_frames',{n:500,max_seconds:60});
    const shot=await tool('screenshot',{path:path.join(dir,'game.png'),overwrite:true});assert.ok(shot.width>0);check('game framebuffer screenshot',shot);
    const point=await tool('emu_status');const compact=await tool('cpu_registers'),verbose=await tool('cpu_registers',{verbose:true});
    assert.equal(Object.keys(compact.gpr_named).length,32);assert.equal(compact.gpr,undefined);assert.equal(verbose.fpr.length,32);assert.equal(Object.keys(verbose.cop0).length,32);assert.equal((await tool('emu_status')).frame,point.frame);check('compact/verbose registers are nonadvancing');
    const invalid=await tool('cpu_set_registers',{gpr:{a0:123},pc:'0x80000001'},true);assert.ok(invalid.error);assert.deepEqual(await tool('cpu_registers',{verbose:true}),verbose);check('late invalid register mutation is atomic');
    const baseline=path.join(dir,'baseline.bs');await tool('state_save',{path:baseline,overwrite:true});
    const dumps=[];for(let i=0;i<2;++i){await tool('state_load',{path:baseline});await tool('controller_set',{});const run=await tool('run_frames',{n:30,max_seconds:60,fpr:true,cop0:true});assert.equal(run.stop_reason,'frames_done');assert.equal(run.completion_registers.fpr.length,32);assert.equal(run.completion_registers.cop0.Random,null);dumps.push(await tool('dump_rdram',{path:path.join(dir,`det-${i}.bin`),overwrite:true,expect_size:8388608}));}
    assert.equal(dumps[0].sha256,dumps[1].sha256);check('load/run/dump deterministic in real game',{sha256:dumps[0].sha256});
    const offlinePoint=await tool('emu_status');await tool('state_peek',{path:baseline,addr:'0x2000',length:32});assert.equal((await tool('emu_status')).frame,offlinePoint.frame);check('offline state inspection leaves game unchanged');
    const dis=await tool('disassemble',{addr:'0x80000400',count:8});check('game disassembly',dis);
    const stack=await tool('stack');assert.ok(stack);check('game heuristic stack',stack);
    await tool('capture_control',{rsp:true,rdp:true});await tool('run_frames',{n:5,max_seconds:60});
    const rsp=await tool('rsp_capture'),rdp=await tool('rdp_capture',{next_frame:false});assert.ok(rdp.count>0);check('game legacy command buffers',{rsp_count:rsp.count,rdp_count:rdp.count,rsp_configured:rsp.configured});
    const exact=await tool('rdp_capture',{next_frame:true,max_seconds:10,max_commands:4096,limit:16});assert.equal(exact.stop_reason,'frames_done');assert.equal(exact.complete,true);assert.ok(exact.word_count>0);check('game exact RDP interval',exact);
    await tool('capture_control',{rsp:false,rdp:false});
    const badState=path.join(dir,'invalid.bs');fs.writeFileSync(badState,'invalid');assert.ok((await tool('state_load',{path:badState},true)).error);assert.equal((await tool('emu_status')).paused,true);check('bad state load leaves isolated game paused');
    // Original acceptance's input handoff and wall-expiry regressions.
    await tool('controller_set',{port:3,a:true,timeout_seconds:0.03});await new Promise(r=>setTimeout(r,80));
    const input=await tool('controller_state');assert.equal(input.ports[2].pending.overridden,false);assert.equal(input.ports[2].latched.overridden,false);check('paused wall lease expiry',input);
    await tool('state_load',{path:baseline});
    await tool('write_memory_va',{addr:'0x7c0000',hex_bytes:'0085102d03e0000800000000',view:'rdram'});
    await tool('cpu_set_registers',{pc:'0x80000400',status:'0x34000000'});
    const call=await tool('call_function',{addr:'0x7c0000',args:[9,7],max_instructions:3});assert.equal(call.stop_reason,'function_return');assert.equal(call.v0,'0x10');assert.equal(call.caller_context_restored,true);check('bounded original function call on real-game machine');
    await tool('write_memory_va',{addr:'0x7c0100',hex_bytes:'1000ffff00000000',view:'rdram'});
    const interrupted=await tool('call_function',{addr:'0x7c0100',max_instructions:2});assert.equal(interrupted.stop_reason,'instruction_limit');assert.equal(interrupted.recovery_available,true);assert.equal((await tool('call_recover')).restored,true);check('interrupted real-game call explicitly recovers');
    await tool('state_load',{path:baseline});check('baseline restored after acceptance');
    await peer.close();peer=undefined;await endpoint.close();router.detach();native.close();child.stdin.end('quit\n');assert.equal((await closed)[0],0,log);
  } finally {
    if(peer) await peer.close();await endpoint.close();router.detach();native.close();if(child.exitCode===null){child.kill('SIGKILL');await closed;}
    assert.equal(hash(rom),before.rom);if(before.save)assert.equal(hash(originalSave),before.save);
    fs.rmSync(ipc,{recursive:true,force:true});
    fs.writeFileSync(path.join(dir,'host.log'),log);fs.writeFileSync(path.join(dir,'acceptance-results.json'),JSON.stringify({binary:hash(host),original_assets_unchanged:true,evidence},null,2));
  }
}
main().catch(e=>{console.error(e);process.exitCode=1;});
