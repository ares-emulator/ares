auto Cartridge::ISViewer::readWord(u32 address) -> u32 {
  address = (address & 0xffff);
  u32 data = ram.read<Word>(address);
  return data;
}

auto Cartridge::ISViewer::writeWord(u32 address, u32 data) -> void {
  address = (address & 0xffff);

  if(address == 0x14) {
    // HACK: allow printf output to work for both libultra and libdragon
    // Libultra expects a real IS-Viewer device and treats this address as a
    // pointer to the end of the buffer, reading the current value, writing N
    // bytes, then updating the buffer pointer.
    // libdragon instead treats this as a "number of bytes" register, only
    // writing an "output byte count"
    // In order to satisfy both libraries, we assume it behaves as libdragon
    // expects, and by forcing the write to never hit ram, libultra remains
    // functional.
    for(auto address : range(u16(data))) {
      char c = ram.read<Byte>(0x20 + address);
      fputc(c, stdout);
    }
    return;
  }

  ram.write<Word>(address, data);
}
