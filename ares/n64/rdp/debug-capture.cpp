auto RDPDebugCapture::onRspStart(u64 pc) -> void {
  if(!inInterval()) return;
  // Do not associate a later batch with an earlier task when storage fills.
  lastTask = -1;
  if(tasks.size() == MaxTasks) { ++droppedTasks; truncated = true; return; }
  Task task; task.id = tasks.size(); task.cpuPc = pc;
  u8 dmem[4096], imem[4096];
  for(u32 n : range(4096)) { dmem[n] = rsp.dmem.read<Byte>(n); imem[n] = rsp.imem.read<Byte>(n); }
  for(u32 n : range(64)) task.header[n] = dmem[0xfc0 + n];
  task.dmemHash = XXH64(dmem, sizeof(dmem), 0); task.imemHash = XXH64(imem, sizeof(imem), 0);
  if(keepDmem) task.dmem.assign(dmem, dmem + sizeof(dmem));
  lastTask = task.id; tasks.push_back(std::move(task));
}

auto RDPDebugCapture::onBatch(u32 start, u32 current, u32 end, bool source, u64 pc) -> void {
  if(!armed) return;
  if(!inInterval()) { ++batchesOutsideInterval; return; }
  // Match the reference's aligned submission range, including XBUS bounds.
  u32 address = current & ~7u, last = end & ~7u;
  if(source) { address &= 0xfff; last = std::min(last, 0x1000u); }
  u32 count = last > address ? (last - address) / 8 : 0;
  if(batches.size() == MaxBatches) {
    ++droppedBatches; droppedWords += count; truncated = true; return;
  }
  Batch batch; batch.id = batches.size(); batch.source = source;
  batch.dpcStart = start; batch.current = current; batch.end = end;
  batch.cpuPc = pc; batch.rspPc = rsp.ipu.pc; batch.rspRunning = !rsp.status.halted;
  batch.task = lastTask; batch.firstWord = words.size(); batch.words = count;
  batch.capturedWords = std::min<u32>(count, maxWords - words.size());
  // Once storage fills, count the rest without an unbounded read/iteration loop.
  droppedWords += count - batch.capturedWords;
  if(batch.capturedWords != count) truncated = true;
  for(u32 n = 0; n < batch.capturedWords; ++n, address += 8) {
    u64 value = source ? rsp.dmem.readUnaligned<Dual>(address)
                       : rdram.ram.read<Dual>(address, RBusDevice::ARES_DEBUGGER);
    words.push_back({value, address, batch.id});
  }
  batches.push_back(batch);
}
