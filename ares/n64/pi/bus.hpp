inline auto PI::readWord(u32 address, Thread& thread) -> u32 {
  if(address <= 0x046f'ffff) return ioRead(address);

  if (unlikely(io.ioBusy)) {
    debug(unusual, "[PI::readWord] PI read to 0x", hex(address, 8L), " will not behave as expected because PI writing is in progress");
    thread.step(writeForceFinish() * 2);
    return io.busLatch;
  }
  thread.step(250 * 2);

  // TODO: this will not have correct open-bus behaviour
  // can we just ignore the bottom two bits when calculating the unmapped read value?
  // how does open bus work with DMAs?
  const n16 hi = busRead<Half>(address    );
  const n16 lo = busRead<Half>(address + 2);

  io.busLatch.bit(16,31) = hi;
  io.busLatch.bit( 0,15) = lo;
  return io.busLatch;
}

template <u32 Size>
inline auto PI::busRead(u32 address) -> u16 {
  static_assert(Size == Half);  //PI bus will do 16-bit only
  if(address >= 0x0500'0000 && address <= 0x0500'03ff) {
    if(_DD()) return dd.c2s.read<Size>(address);
  }
  if(address >= 0x0500'0400 && address <= 0x0500'04ff) {
    if(_DD()) return dd.ds.read<Size>(address);
  }
  if(address >= 0x0500'0500 && address <= 0x0500'057f) {
    if(_DD()) return dd.read<Size>(address);
  }
  if(address >= 0x0500'0580 && address <= 0x0500'05bf) {
    if(_DD()) return dd.ms.read<Size>(address);
  }
  if(address >= 0x0600'0000 && address <= 0x063f'ffff) {
    if(_DD()) return dd.iplrom.read<Size>(address);
  }
  return cartridge.readHalf(address);
}

inline auto PI::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(address <= 0x046f'ffff) return ioWrite(address, data);

  if(io.ioBusy) return;
  io.ioBusy = 1;
  io.busLatch = data;
  queue.insert(Queue::PI_BUS_Write, 400);

  const n16 hi = data >> 16;
  const n16 lo = data & 0xFFFF;

  busWrite<Half>(address,     hi);
  busWrite<Half>(address + 2, lo);

  return;
}

template <u32 Size>
inline auto PI::busWrite(u32 address, u16 data) -> void {
  static_assert(Size == Half);  //PI bus will do 16-bit only
  if(address >= 0x0500'0000 && address <= 0x0500'03ff) {
    if(_DD()) return dd.c2s.write<Size>(address, data);
  }
  if(address >= 0x0500'0400 && address <= 0x0500'04ff) {
    if(_DD()) return dd.ds.write<Size>(address, data);
  }
  if(address >= 0x0500'0500 && address <= 0x0500'057f) {
    if(_DD()) return dd.write<Size>(address, data);
  }
  if(address >= 0x0500'0580 && address <= 0x0500'05bf) {
    if(_DD()) return dd.ms.write<Size>(address, data);
  }
  if(address >= 0x0600'0000 && address <= 0x063f'ffff) {
    if(_DD()) return dd.iplrom.write<Size>(address, data);
  }
  return cartridge.writeHalf(address, data);
}

inline auto PI::writeFinished() -> void {
  io.ioBusy = 0;
}

inline auto PI::writeForceFinish() -> u32 {
  io.ioBusy = 0;
  return queue.remove(Queue::PI_BUS_Write);
}
