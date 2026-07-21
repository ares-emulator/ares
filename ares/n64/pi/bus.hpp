inline auto PI::attach(PIDevice& device, u32 priority) -> void {
  auto it = devices.begin();
  while(it != devices.end() && it->priority <= priority) ++it;
  u32 index = it - devices.begin();
  if(busDevice >= (s32)index) busDevice++;
  devices.insert(it, {priority, &device});
}

inline auto PI::detach(PIDevice& device) -> void {
  for(u32 i = 0; i < devices.size(); i++) {
    if(devices[i].device != &device) continue;
    devices.erase(devices.begin() + i);
    if(busDevice == (s32)i) busDevice = -1;
    else if(busDevice > (s32)i) busDevice--;
    return;
  }
}

inline auto PI::bsdForAddress(u32 address) -> BSD& {
  switch(address >> 24) {
  case 0x05:               return bsd2;
  case range8(0x08, 0x0F): return bsd2;
  default:                 return bsd1;
  }
}

inline auto PI::busAddress(u32 address) -> void {
  address &= ~1;
  io.busLatch = u16(address) << 16 | u16(address);
  auto& bsd = bsdForAddress(address);
  busTiming = {bsd.latency, bsd.pulseWidth, bsd.releaseDuration};
  busDevice = -1;
  for(u32 i = 0; i < devices.size(); i++) {
    if(devices[i].device->piAddress(address, busTiming)) {
      busDevice = i;
      break;
    }
  }
}

inline auto PI::busReadHalf() -> u16 {
  if(busDevice >= 0) {
    if(auto data = devices[busDevice].device->piReadHalf(busTiming)) {
      io.busLatch = u32(*data) << 16 | *data;
    }
  }
  return io.busLatch;
}

inline auto PI::busWriteHalf(u16 data) -> void {
  if(!io.ioBusy) io.busLatch = u32(data) << 16 | data;
  if(busDevice >= 0) devices[busDevice].device->piWriteHalf(data, busTiming);
}

inline auto PI::readWord(u32 address, Thread& thread) -> u32 {
  if(address <= 0x046f'ffff) return ioRead(address);

  if(unlikely(io.ioBusy)) {
    debug(unusual, "[PI::readWord] PI read to 0x", hex(address, 8L), " will not behave as expected because PI writing is in progress");
    thread.step(writeForceFinish() * 2);
    return io.busLatch;
  }
  thread.step(250 * 2);
  busAddress(address);
  u32 data = busReadHalf() << 16;
  io.busLatch = data | busReadHalf();
  io.pbusAddress = (address + 4) & ~1;
  return io.busLatch;
}

inline auto PI::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(address <= 0x046f'ffff) return ioWrite(address, data);

  if(io.ioBusy) return;
  io.ioBusy = 1;
  io.pbusAddress = (address + 4) & ~1;
  cpu.queueInsert(Queue::PI_BUS_Write, 400);
  busAddress(address);
  io.busLatch = data;
  busWriteHalf(data >> 16);
  busWriteHalf(data >>  0);
}

inline auto PI::writeFinished() -> void {
  io.ioBusy = 0;
}

inline auto PI::writeForceFinish() -> u32 {
  io.ioBusy = 0;
  return queue.remove(Queue::PI_BUS_Write);
}
