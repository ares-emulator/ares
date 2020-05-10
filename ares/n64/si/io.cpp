static const vector<string> registerNames = {
  "SI_DRAM_ADDRESS",
  "SI_PIF_ADDRESS_READ64B",
  "SI_INT_ADDRESS_WRITE64B",
  "SI_RESERVED",
  "SI_PIF_ADDRESS_WRITE64B",
  "SI_INT_ADDRESS_READ64B",
  "SI_STATUS",
};

auto SI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  uint32 data;

  if(address == 0) {
    //SI_DRAM_ADDRESS
    data.bit(0,23) = io.dramAddress;
  }

  if(address == 1) {
    //SI_PIF_ADDRESS_READ64B
    data.bit(0,31) = io.readAddress;
  }

  if(address == 2) {
    //SI_INT_ADDRESS_WRITE64B
  }

  if(address == 3) {
    //SI_RESERVED
  }

  if(address == 4) {
    //SI_PIF_ADDRESS_WRITE64B
    data.bit(0,31) = io.writeAddress;
  }

  if(address == 5) {
    //SI_INT_ADDRESS_READ64B
  }

  if(address == 6) {
    //SI_STATUS
    data.bit( 0)    = io.dmaBusy;
    data.bit( 1)    = io.ioBusy;
    data.bit( 2)    = io.readPending;
    data.bit( 3)    = io.dmaError;
    data.bit( 4, 7) = io.pchState;
    data.bit( 8,11) = io.dmaState;
    data.bit(12)    = io.interrupt;
  }

  print("* ", registerNames(address, "SI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto SI::writeIO(u32 address, uint32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //SI_DRAM_ADDRESS
    io.dramAddress = data.bit(0,23);
  }

  if(address == 1) {
    //SI_PIF_ADDRESS_READ64B
    if(auto& device = controllerPort1.device) pi.ram.writeWord(0x04, device->read());
    if(auto& device = controllerPort2.device) pi.ram.writeWord(0x0c, device->read());
    if(auto& device = controllerPort3.device) pi.ram.writeWord(0x14, device->read());
    if(auto& device = controllerPort4.device) pi.ram.writeWord(0x1c, device->read());

    io.readAddress = data.bit(0,31);
    for(u32 offset = 0; offset < 64; offset += 8) {
      u64 data = bus.readDouble(offset);
      rdram.ram.writeDouble(io.dramAddress + offset, data);
    }
    io.interrupt = 1;
    mi.raise(MI::IRQ::SI);
  }

  if(address == 2) {
    //SI_INT_ADDRESS_WRITE64B
  }

  if(address == 3) {
    //SI_RESERVED
  }

  if(address == 4) {
    //SI_PIF_ADDRESS_WRITE64B
    io.writeAddress = data.bit(0,31);
    for(u32 offset = 0; offset < 64; offset += 8) {
      u64 data = rdram.ram.readDouble(io.dramAddress + offset);
      bus.writeDouble(offset, data);
    }
    io.interrupt = 1;
    mi.raise(MI::IRQ::SI);
  }

  if(address == 5) {
    //SI_INT_ADDRESS_READ64B
  }

  if(address == 6) {
    //SI_STATUS
    io.interrupt = 0;
    mi.lower(MI::IRQ::SI);
  }

  print("* ", registerNames(address, "SI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
