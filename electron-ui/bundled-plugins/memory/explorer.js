// Large copies and scans stay native; only a visible comparison page crosses IPC.
export function activateExplorer(ctx) {
  ctx.ui.registerPanel('diff', ({element,signal}) => {
    element.classList.add('memory-diff')
    element.innerHTML = `<h2>RAM diff explorer</h2><label>Region <select data-field="region"></select></label><label>Offset <input data-field="offset" value="0x0"></label><label>Bytes <input data-field="length" type="number" value="65536" min="1" max="16777216"></label><button data-action="before">Capture before</button><button data-action="after">Capture after</button><label>Type <select data-field="type"><option>u8</option><option>u16</option><option>u32</option><option>f32</option></select></label><label>Endian <select data-field="endian"><option>big</option><option>little</option></select></label><label>Filter <select data-field="filter"><option>changed</option><option>unchanged</option><option>increased</option><option>decreased</option><option>delta</option><option>equals</option></select></label><label>Value <input data-field="value" type="number" value="0" step="any"></label><button data-action="compare">Compare</button><button data-action="next" disabled>Next page</button><p role="status" aria-live="polite"></p><table><thead><tr><th>Offset</th><th>Before</th><th>After</th><th>Label</th></tr></thead><tbody></tbody></table><label>Label offset <input data-field="labelOffset" value="0x0"></label><label>Label <input data-field="label" maxlength="160"></label><button data-action="label">Save label</button><button data-action="export">Export labels</button><label>Import labels <input data-field="import" type="file" accept="application/json"></label>`
    element.insertAdjacentHTML('beforeend', `<fieldset><legend>Narrow and inspect</legend><button data-action="narrow">Narrow retained candidates</button><label>Changes <select data-field="map"><option>run</option><option>line</option><option>page</option></select></label><button data-action="map">Show content changes</button><button data-action="clear">Clear samples</button></fieldset><fieldset><legend>Frame recording (does not resume a paused game)</legend><label>Capacity <input data-field="capacity" type="number" value="60" min="1" max="1024"></label><label>Duration (0 = until stopped) <input data-field="duration" type="number" value="60" min="0" max="1000000"></label><button data-action="record">Start recording</button><button data-action="stop">Stop recording</button><button data-action="recordStatus">Refresh recording</button><p data-recording>No recording.</p><label>Sequence <input data-field="sequence" type="number" min="0" value="0"></label><button data-action="frameBefore">Use frame as before</button><button data-action="frameAfter">Use frame as after</button></fieldset>`)
    const field=id=>element.querySelector(`[data-field="${id}"]`), button=id=>element.querySelector(`[data-action="${id}"]`), status=element.querySelector('[role="status"]')
    const request=async(method,p)=>{const r=await ctx.services.request(method,p);if(!r.ok)throw Object.assign(new Error(r.error), {code:r.error});return r.result}
    let before,after,candidates,working,recording,view='typed',cursor=0,next=null,busy=false,regions=[],labels={}
    const storageKey=(ctx.game?.sha256??('diff-labels-'+ctx.system).replace(/[^a-zA-Z0-9_-]/g,'_').slice(0,64))
    const provenance=()=>({rom:ctx.game?.sha256??null,system:ctx.system,regions:regions.map(({name,size,addressing})=>({name,size,addressing}))})
    const show=async(fn)=>{if(busy)return;busy=true;for(const b of element.querySelectorAll('button'))b.disabled=true;try{await fn()}catch(e){status.textContent=String(e);if(['stale_memory_handle','stale_query_handle','stale_epoch','stale_session'].includes(e.code)){await clear();status.textContent='Samples expired. Capture new before/after samples.'}if(e.code==='cursor_overwritten')await refreshRecording()}finally{busy=false;if(!signal.aborted){for(const b of element.querySelectorAll('button'))b.disabled=false;button('next').disabled=next===null;button('narrow').disabled=!candidates;for(const id of ['stop','recordStatus','frameBefore','frameAfter'])button(id).disabled=!recording}}}
    const release=async s=>{if(s)await request('memory.capture.release',{handle:s.handle}).catch(()=>{})}
    const releaseQuery=async q=>{if(q)await request('memory.query.release',{handle:q.handle}).catch(()=>{})}
    const clear=async()=>{await releaseQuery(working);working=undefined;await releaseQuery(candidates);candidates=undefined;await release(before);before=undefined;await release(after);after=undefined;await release(recording);recording=undefined;next=null;if(!signal.aborted){element.querySelector('tbody').replaceChildren();element.querySelector('[data-recording]').textContent='No recording.'}}
    const setSample=async(which,sample)=>{
      if(signal.aborted){await release(sample);return}
      if(which==='before'){await release(before);before=sample}else{await release(after);after=sample}
      next=null;cursor=0;element.querySelector('tbody').replaceChildren();status.textContent=`${which}: epoch ${sample.epoch}, frame ${sample.first_frame}, ${sample.frame_kind}, capture ${sample.handle}. ${candidates?'Retained candidates can be narrowed.':''}`
    }
    const capture=async which=>{
      const region=field('region').value,offset=Number(field('offset').value),length=Number(field('length').value)
      const sample=await request('memory.snapshot.capture',{reads:[{region,offset,length}],scheduling:'immediate'})
      await setSample(which,sample)
    }
    const compare=async()=>{
      if(!before||!after)throw new Error('Capture before and after first.')
      const page=view==='typed'?await request('memory.query.page',{handle:candidates.handle,cursor,limit:128}):await request('memory.snapshot.changes',{before:before.handle,after:after.handle,range:0,cursor,limit:128,granularity:view})
      if(signal.aborted)return
      const body=element.querySelector('tbody');body.replaceChildren()
      if(view!=='typed'){for(const m of page.changes){const row=document.createElement('tr');for(const text of [`0x${m.offset.toString(16)}`,`${m.length} bytes${m.continues_before?' (continued)':''}${m.continues_after?' (continues)':''}`,'Changed contents','']){const td=document.createElement('td');td.textContent=text;row.append(td)}body.append(row)}next=page.next_cursor;status.textContent=`${page.changes.length} changed ${view}s · captures ${before.handle} → ${after.handle} · ${next===null?'end':'more addresses'}`;return}
      for(const m of page.matches){const row=document.createElement('tr');for(const text of [`0x${m.offset.toString(16)}`,`${m.old??'nonfinite'} (${m.old_hex})`,`${m.new??'nonfinite'} (${m.new_hex})`,labels[`${field('region').selectedOptions[0].textContent}:${m.offset}`]??'']){const cell=document.createElement('td');cell.textContent=text;row.append(cell)}body.append(row)}
      next=page.next_cursor;status.textContent=`${page.matches.length} matches on this page · ${candidates.candidate_count} candidates total · frames ${before.first_frame} → ${after.first_frame} · captures ${before.handle} → ${after.handle} · epoch ${before.epoch}${next===null?' · end':' · more addresses available'}`
    }
    const search=async narrow=>{
      if(!before||!after)throw new Error('Capture before and after first.')
      const previous=candidates
      working=await request('memory.query.start',{before:before.handle,after:after.handle,range:0,type:field('type').value,endian:field('endian').value,filter:field('filter').value,value:Number(field('value').value),candidates:narrow?previous?.handle??0:0})
      try{
        while(working.state==='running'&&!signal.aborted){working=await request('memory.query.advance',{handle:working.handle});status.textContent=`Scanned ${working.scanned} / ${working.total} values · ${working.candidate_count} candidates`}
        if(signal.aborted)return
        if(working.state==='failed')throw new Error(`${working.error}. Select a smaller range or more selective filter.`)
        candidates=working;working=undefined;await releaseQuery(previous);cursor=0;next=null;view='typed';await compare()
      }finally{await releaseQuery(working);working=undefined}
    }
    const refreshRecording=async()=>{
      if(!recording)throw new Error('Start recording first.')
      recording=await request('memory.capture.status',{handle:recording.handle})
      const r=recording
      element.querySelector('[data-recording]').textContent=`${r.active?'Recording':'Stopped'} · sequences ${r.first_sequence}–${r.next_sequence?r.next_sequence-1:'none'} · frames ${r.first_frame??'none'}–${r.last_frame??'none'} (${r.frame_kind}) · ${r.dropped} overwritten · ${(r.max_copy_ns/1000).toFixed(1)} µs maximum native copy · ${r.transferred_bytes} bytes fetched`
      field('sequence').min=String(r.first_sequence);field('sequence').max=String(Math.max(r.first_sequence,r.next_sequence-1));if(Number(field('sequence').value)<r.first_sequence)field('sequence').value=String(r.first_sequence)
    }
    ctx.scope.listen(button('narrow'),'click',()=>show(()=>search(true)))
    ctx.scope.listen(button('map'),'click',()=>show(async()=>{view=field('map').value;cursor=0;next=null;await compare()}))
    ctx.scope.listen(button('clear'),'click',()=>show(async()=>{await clear();status.textContent='Capture new before/after samples.'}))
    ctx.scope.listen(button('record'),'click',()=>show(async()=>{
      const created=await request('memory.recorder.start',{reads:[{region:field('region').value,offset:Number(field('offset').value),length:Number(field('length').value)}],capacity:Number(field('capacity').value),duration:Number(field('duration').value)})
      await release(recording);recording=created;await refreshRecording()
    }))
    ctx.scope.listen(button('stop'),'click',()=>show(async()=>{recording=await request('memory.recorder.stop',{handle:recording.handle});await refreshRecording()}))
    ctx.scope.listen(button('recordStatus'),'click',()=>show(refreshRecording))
    for(const which of ['before','after'])ctx.scope.listen(button(which==='before'?'frameBefore':'frameAfter'),'click',()=>show(async()=>{await setSample(which,await request('memory.capture.freeze',{handle:recording.handle,sequence:Number(field('sequence').value)}))}))
    for(const which of ['before','after'])ctx.scope.listen(button(which),'click',()=>show(()=>capture(which)))
    ctx.scope.listen(button('compare'),'click',()=>show(()=>search(false)))
    ctx.scope.listen(button('next'),'click',()=>show(async()=>{cursor=next;await compare()}))
    ctx.scope.listen(button('label'),'click',()=>show(async()=>{const offset=Number(field('labelOffset').value);if(!Number.isSafeInteger(offset)||offset<0)throw new Error('Invalid label offset');const key=`${field('region').selectedOptions[0].textContent}:${offset}`;labels[key]=field('label').value;await ctx.storage.set(storageKey,{...provenance(),labels});status.textContent='Label saved.'}))
    ctx.scope.listen(button('export'),'click',()=>{const url=URL.createObjectURL(new Blob([JSON.stringify({...provenance(),labels},null,2)],{type:'application/json'}));const a=document.createElement('a');a.href=url;a.download='ares-memory-labels.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),0)})
    ctx.scope.listen(field('import'),'change',()=>show(async()=>{const file=field('import').files[0];if(!file||file.size>16384)throw new Error('Label import must be under 16 KiB');const data=JSON.parse(await file.text());if(JSON.stringify({...data,labels:undefined})!==JSON.stringify({...provenance(),labels:undefined})||!data.labels||Object.values(data.labels).some(v=>typeof v!=='string'||v.length>160))throw new Error('Labels have incompatible ROM or memory-map provenance');labels=data.labels;await ctx.storage.set(storageKey,data);status.textContent='Labels imported.'}))
    signal.addEventListener('abort',()=>{void clear()},{once:true})
    void show(async()=>{regions=(await request('memory.regions')).regions.filter(r=>r.readable);for(const r of regions){const option=document.createElement('option');option.value=r.id;option.textContent=r.name;field('region').append(option)}field('length').value=String(Math.min(65536,regions[0]?.size??1));const stored=await ctx.storage.get(storageKey);if(stored&&JSON.stringify(stored.regions)===JSON.stringify(provenance().regions))labels=stored.labels??{};status.textContent='Capture two samples to compare memory contents.'})
  })
  ctx.ui.registerCommand('diff',()=>ctx.ui.openPanel('diff'))
}
