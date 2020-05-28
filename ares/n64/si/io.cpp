static const vector<string> registerNames = {
  "SI_DRAM_ADDRESS",
  "SI_PIF_ADDRESS_READ64B",
  "SI_INT_ADDRESS_WRITE64B",
  "SI_RESERVED",
  "SI_PIF_ADDRESS_WRITE64B",
  "SI_INT_ADDRESS_READ64B",
  "SI_STATUS",
};

auto SI::readWord(u32 address) -> u32 {
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

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNames(address, "SI_UNKNOWN"), " => ", hex(data, 8L)});
  }
  return data;
}

auto SI::writeWord(u32 address, u32 data_) -> void {
  address = (address & 0xfffff) >> 2;
  uint32 data = data_;

  if(address == 0) {
    //SI_DRAM_ADDRESS
    io.dramAddress = data.bit(0,23) & ~7;
  }

  if(address == 1) {
    //SI_PIF_ADDRESS_READ64B
    main();
    io.readAddress = data.bit(0,31) & ~1;
    for(u32 offset = 0; offset < 64; offset += 2) {
      u16 data = bus.readHalf(io.readAddress + offset);
      bus.writeHalf(io.dramAddress + offset, data);
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
    io.writeAddress = data.bit(0,31) & ~1;
    for(u32 offset = 0; offset < 64; offset += 2) {
      u16 data = bus.readHalf(io.dramAddress + offset);
      bus.writeHalf(io.writeAddress + offset, data);
    }
    io.interrupt = 1;
    mi.raise(MI::IRQ::SI);
    main();
  }

  if(address == 5) {
    //SI_INT_ADDRESS_READ64B
  }

  if(address == 6) {
    //SI_STATUS
    io.interrupt = 0;
    mi.lower(MI::IRQ::SI);
  }

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNames(address, "SI_UNKNOWN"), " <= ", hex(data, 8L)});
  }
}
