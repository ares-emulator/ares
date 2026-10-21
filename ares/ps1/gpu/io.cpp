inline auto GPU::writeFifoEmpty() const -> bool {
  if(io.mode == Mode::CopyFromVRAM) return true;
  if(drawBusy()) return false;
  bool collectingPrimitive = !queue.gp0.empty() && queue.gp0.command >= 0x20 && queue.gp0.command <= 0x5f;
  bool latchOccupied = latch.ready || (latch.length && (latch.data[0] >> 24) >= 0x20 && (latch.data[0] >> 24) <= 0x5f);
  return fifo.empty() && !latchOccupied && !collectingPrimitive;
}

inline auto GPU::dmaRequest() const -> bool {
  switch(io.dmaDirection) {
  case 0: return false;
  case 1: return fifo.size() + (latch.ready ? 1 : 0) <= Fifo::Depth / 2;
  case 2: return writeFifoEmpty();
  case 3: return io.mode == Mode::CopyFromVRAM;
  }
  return false;
}

auto GPU::canReadDMA() -> bool {
  return dmaRequest();
}

auto GPU::canWriteDMA() -> bool {
  return dmaRequest();
}

auto GPU::readDMA() -> u32 {
  return readWord(0x1f80'1810);
}

auto GPU::writeDMA(u32 data) -> void {
  return writeGP0(data, false, true);
}

auto GPU::readByte(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "GPU::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto GPU::readHalf(u32 address) -> u32 {
  n32 data;
  debug(unhandled, "GPU::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
}

auto GPU::readWord(u32 address) -> u32 {
  n32 data;
  if(address == 0x1f80'1810) return data = readGP0();
  if(address == 0x1f80'1814) return data = readGP1();
  debug(unhandled, "GPU::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
  return data;
}

auto GPU::writeByte(u32 address, u32 data) -> void {
  debug(unhandled, "GPU::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto GPU::writeHalf(u32 address, u32 data) -> void {
  debug(unhandled, "GPU::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto GPU::writeWord(u32 address, u32 data) -> void {
  if(address == 0x1f80'1810) return writeGP0(data);
  if(address == 0x1f80'1814) return writeGP1(data);
  debug(unhandled, "GPU::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ")");
}
