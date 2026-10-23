const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {spawn} = require('node:child_process');
const {once} = require('node:events');
const {LocalServiceClient} = require('../dist/services/local-endpoint.js');
const {StdioPeer} = require('./stdio-peer.cjs');
const sleep = ms => new Promise(r=>setTimeout(r,ms));
module.exports = async function checkSignals(bundle) {
  const directory=fs.mkdtempSync('/tmp/ares-signals-'),serviceDirectory=path.join(directory,'ipc');
  const root=path.resolve(__dirname,'..');
  const electron=bundle?path.join(bundle,'Contents/MacOS/Electron'):require('electron');
  const entry=bundle?[]:[path.join(root,'dist/main.js')];
  const bridge=bundle?path.join(bundle,'Contents/Resources/app/dist/mcp/server.js'):path.join(root,'dist/mcp/server.js');
  const baseEnv={...process.env};
  for(const key of Object.keys(baseEnv))if(key.startsWith('ARES_')||key==='ELECTRON_RUN_AS_NODE')delete baseEnv[key];
  if(!bundle&&process.env.ARES_HOST)baseEnv.ARES_HOST=process.env.ARES_HOST;
  Object.assign(baseEnv,{ARES_BACKGROUND_TEST:'1',ARES_DEFAULT_PLUGINS:'0',ARES_SILENT:'1',ARES_ENABLE_SERVICES:'1',ARES_SERVICE_DIR:serviceDirectory});
  const children=[];
  const launch=profile=>{
    const child=spawn(electron,[...entry,`--user-data-dir=${path.join(directory,profile)}`,'--instance-name','kirby'],{env:baseEnv,stdio:['ignore','pipe','pipe']});
    child.stdout.resume();let log='';child.stderr.on('data',b=>log+=b);const closed=once(child,'close');const value={child,closed,log:()=>log};children.push(value);return value;
  };
  async function until(fn,label){for(let i=0;i<300;i++){try{const value=await fn();if(value)return value}catch{}await sleep(10)}throw new Error(label+'\n'+children.map(c=>c.log()).join('\n'));}
  async function stopped(value,signal){value.child.kill(signal);const timer=setTimeout(()=>value.child.kill('SIGKILL'),5000);try{const [code,ended]=await value.closed;if(signal!=='SIGKILL'){assert.equal(ended,null,value.log());assert.equal(code,0,value.log())}}finally{clearTimeout(timer)}}
  const client=LocalServiceClient.named(serviceDirectory,'kirby',2000);
  const peer=new StdioPeer(electron,[bridge,'--service-dir',serviceDirectory,'--instance-name','kirby'],{env:{...baseEnv,ELECTRON_RUN_AS_NODE:'1',NODE_OPTIONS:''}});
  const namedFile=path.join(serviceDirectory,'named-kirby.json');
  try {
    await peer.initialize();
    const a=launch('profile');
    const first=await until(async()=>{const r=await client.request('session.status');return r.ok?r:undefined},'first launch');
    const old=JSON.parse(fs.readFileSync(namedFile));
    const duplicate=launch('duplicate');await until(()=>duplicate.log().includes('instance_name_in_use'),'duplicate name rejected');await duplicate.closed;
    assert.equal(JSON.parse(fs.readFileSync(namedFile)).instance,old.instance);
    const mutation=peer.tool('run_frames',{n:1000000,max_seconds:60}).catch(e=>({error:String(e)}));
    await until(async()=>{const r=await client.request('session.status');return r.result?.operation_id},'active mutation');
    await stopped(a,'SIGTERM');const interrupted=await mutation;assert.ok(interrupted.error,JSON.stringify(interrupted));
    assert.equal(fs.existsSync(namedFile),false,'SIGTERM removes the owned named claim');
    const absent=await peer.tool('emu_status');assert.ok(['instance_not_running','app_disconnected','app_not_running'].includes(absent.error),JSON.stringify(absent));
    const b=launch('profile');
    const second=await until(async()=>{const r=await peer.tool('emu_status');return r.session&&r.session!==first.session?r:undefined},'same saved MCP configuration reconnects');
    const fresh=JSON.parse(fs.readFileSync(namedFile));assert.notEqual(fresh.instance,old.instance);assert.notEqual(fresh.token,old.token);assert.equal(second.operation_id,null,'interrupted mutation is not replayed');
    const forgedFile=path.join(serviceDirectory,'old-token.json');fs.writeFileSync(forgedFile,JSON.stringify({...fresh,token:old.token}),{mode:0o600});
    const forged=new LocalServiceClient(forgedFile,fresh.instance,1000);try{await assert.rejects(forged.request('session.status'),/unauthorized/)}finally{forged.close();fs.rmSync(forgedFile)}
    await stopped(b,'SIGINT');assert.equal(fs.existsSync(namedFile),false,'SIGINT removes the owned named claim');
    const c=launch('profile');await until(async()=>{const r=await peer.tool('emu_status');return r.session&&r.session!==second.session?r:undefined},'third launch');
    const stale=JSON.parse(fs.readFileSync(namedFile));await stopped(c,'SIGKILL');assert.equal(fs.existsSync(namedFile),true,'SIGKILL deliberately leaves stale claim');
    const d=launch('profile');await until(async()=>{if(!fs.existsSync(namedFile))return false;const info=JSON.parse(fs.readFileSync(namedFile));if(info.instance===stale.instance)return false;const r=await peer.tool('emu_status');return r.session&&r.session!==second.session},'recover killed launch');await stopped(d,'SIGTERM');
    return {signals:['SIGTERM','SIGINT','SIGKILL'],fresh_credentials:true,old_token_rejected:true,named_restart:true,mutation_replayed:false};
  } finally {
    client.close();await peer.close();
    for(const value of children)if(value.child.exitCode===null&&value.child.signalCode===null){value.child.kill('SIGKILL');await value.closed;}
    await sleep(30);fs.rmSync(directory,{recursive:true,force:true});
  }
};
