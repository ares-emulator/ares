auto PI::dmaRead() -> void {
  for(u32 address = 0; address < io.readLength; address += 2) {
    u16 data = bus.read<Half>(io.dramAddress + address);
    bus.write<Half>(io.pbusAddress + address, data);
  }
  io.dmaBusy = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::PI);
}

auto PI::dmaWrite() -> void {
  for(u32 address = 0; address < io.writeLength; address += 2) {
    u16 data = bus.read<Half>(io.pbusAddress + address);
    bus.write<Half>(io.dramAddress + address, data);
  }
  io.dmaBusy = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::PI);
}
