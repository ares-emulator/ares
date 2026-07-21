auto Cartridge::ISViewer::piAddress(u32 address, PIDeviceTiming) -> bool {
  if(!enabled()) return false;
  if(address < 0x13ff'0000 || address > 0x13ff'ffff) return false;
  piAddr = address & 0xffff;
  return true;
}

auto Cartridge::ISViewer::piReadHalf(PIDeviceTiming) -> maybe<u16> {
  u16 data = readHalf(piAddr);
  piAddr += 2;
  return data;
}

auto Cartridge::ISViewer::piWriteHalf(u16 data, PIDeviceTiming) -> void {
  pi.writeForceFinish();
  writeHalf(piAddr, data);
  piAddr += 2;
}

auto Cartridge::ISViewer::readHalf(u32 address) -> u16 {
  address = (address & 0xffff);
  return ram.read<Half>(address);
}

auto Cartridge::ISViewer::messageChar(char c) -> void {
  if(!tracer->enabled()) return;
  tracer->notify(c);
}

auto Cartridge::ISViewer::writeHalf(u32 address, u16 data) -> void {
  address = (address & 0xffff);

  if(address == 0x16) {
    // HACK: allow printf output to work for both libultra and libdragon
    // Libultra expects a real IS-Viewer device and treats this address as a
    // pointer to the end of the buffer, reading the current value, writing N
    // bytes, then updating the buffer pointer.
    // libdragon instead treats this as a "number of bytes" register, only
    // writing an "output byte count"
    // In order to satisfy both libraries, we assume it behaves as libdragon
    // expects, and by forcing the write pointer to always be zero, libultra
    // remains functional.
    // Also reset the read pointer back to zero if it is used as a starting
    // address, for compatibility with hardware even though neither library
    // ever writes to it.
    for(auto address : range(ram.read<Half>(0x4), data)) {
      char c = ram.read<Byte>(0x20 + address);
      messageChar(c);
    }
    ram.write<Word>(0x4, 0);
    return;
  }

  ram.write<Half>(address, data);
}
