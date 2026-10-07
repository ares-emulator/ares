auto CPU::retireMemoryFrontend() -> void {
  while(true) {
    scheduleWriteBuffer();
    bool refillDue = frontend.refill.active && frontend.refill.completion <= execution.clock;
    bool writeDue = frontend.writeBuffer.count && frontend.writeBuffer.completion
      && frontend.writeBuffer.completion <= execution.clock;
    if(!refillDue && !writeDue) return;

    if(writeDue && (!refillDue || frontend.writeBuffer.completion <= frontend.refill.completion)) {
      retireWriteBuffer();
    } else {
      icache.retireRefill();
    }
  }
}

auto CPU::scheduleWriteBuffer() -> bool {
  auto& buffer = frontend.writeBuffer;
  if(!buffer.count || buffer.completion) return false;
  if(!bus.acquire(Bus::WriteBuffer)) {
    buffer.ready = execution.clock;
    return false;
  }
  buffer.completion = buffer.ready + buffer.delay;
  return true;
}

auto CPU::enterBusWait(u8 reason) -> void {
  frontend.busWait = reason;
  while(frontend.busWait != MemoryFrontend::BusWaitNone) step(forceSyncInterval);
}

auto CPU::writeBufferByteEnable(u32 address, u32 size) const -> u8 {
  if(size == Byte) return 1 << (address & 3);
  if(size == Half) return 3 << (address & 2);
  return 15;
}

auto CPU::enqueueWriteBuffer(u32 address, u32 offset, u32 data, u8 byteEnable) -> void {
  auto& buffer = frontend.writeBuffer;
  while(buffer.count == 4) {
    if(buffer.completion > execution.clock) step(buffer.completion - execution.clock);
    else step(1);
    if(active()) synchronize();
  }

  bool wasEmpty = buffer.count == 0;
  u8 tail = (buffer.head + buffer.count) & 3;
  u32 shift = (address & 3) * 8;
  buffer.entries[tail] = {address & ~3, offset & ~3, data << shift, byteEnable, buffer.nextSequence++};
  buffer.count++;
  if(wasEmpty) {
    buffer.ready = execution.clock;
    buffer.delay = 4;
    buffer.completion = 0;
    scheduleWriteBuffer();
  }
  step(1);
}

auto CPU::retireWriteBuffer() -> void {
  auto& buffer = frontend.writeBuffer;
  if(!buffer.count || buffer.completion > execution.clock) return;

  u64 completion = buffer.completion;
  auto entry = buffer.entries[buffer.head];
  for(u32 byte : range(4)) {
    if(entry.byteEnable & 1 << byte) ram.write<Byte>(entry.offset + byte, entry.data >> byte * 8);
  }

  buffer.entries[buffer.head] = {};
  bus.release(Bus::WriteBuffer);
  buffer.head = (buffer.head + 1) & 3;
  buffer.count--;
  if(buffer.count == 0) {
    buffer.head = 0;
    buffer.completion = 0;
    buffer.ready = 0;
    buffer.delay = 4;
    return;
  }

  auto& next = buffer.entries[buffer.head];
  bool samePage = (entry.address >> 10) == (next.address >> 10);
  buffer.completion = 0;
  buffer.ready = completion;
  buffer.delay = samePage ? 2 : 4;
  scheduleWriteBuffer();
}

auto CPU::waitWriteBuffer(u32 offset, u8 byteEnable) -> void {
  auto& buffer = frontend.writeBuffer;
  while(true) {
    bool conflict = false;
    for(u32 index : range(buffer.count)) {
      auto& entry = buffer.entries[(buffer.head + index) & 3];
      if(entry.offset == (offset & ~3) && (entry.byteEnable & byteEnable)) conflict = true;
    }
    if(!conflict) return;
    if(buffer.completion > execution.clock) step(buffer.completion - execution.clock);
    else step(1);
    if(active()) synchronize();
  }
}
