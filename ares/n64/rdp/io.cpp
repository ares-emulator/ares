auto RDP::readWord(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  n32 data;

  if(address == 0) {
    //DPC_START
    data.bit(0,23) = command.start;
  }

  if(address == 1) {
    //DPC_END
    data.bit(0,23) = command.end;
  }

  if(address == 2) {
    //DPC_CURRENT
    data.bit(0,23) = command.current;
  }

  if(address == 3) {
    //DPC_STATUS
    data.bit( 0) = command.source;
    data.bit( 1) = command.freeze;
    data.bit( 2) = command.flush;
    data.bit( 3) = 0;  //start gclk?
    data.bit( 4) = command.tmemBusy > 0;
    data.bit( 5) = command.pipeBusy > 0;
    data.bit( 6) = command.bufferBusy > 0;
    data.bit( 7) = command.ready;
    data.bit( 8) = 0;  //DMA busy
    data.bit( 9) = 0;  //end valid
    data.bit(10) = 0;  //start valid
  }

  if(address == 4) {
    //DPC_CLOCK
    data.bit(0,23) = command.clock;
  }

  if(address == 5) {
    //DPC_BUSY
    data.bit(0,23) = command.bufferBusy;
  }

  if(address == 6) {
    //DPC_PIPE_BUSY
    data.bit(0,23) = command.pipeBusy;
  }

  if(data == 7) {
    //DPC_TMEM_BUSY
    data.bit(0,23) = command.tmemBusy;
  }

  debugger.ioDPC(Read, address, data);
  return data;
}

auto RDP::writeWord(u32 address, u32 data_) -> void {
  address = (address & 0xfffff) >> 2;
  n32 data = data_;

  if(address == 0) {
    //DPC_START
    command.start = data.bit(0,23) & ~7;
    command.current = command.start;
  }

  if(address == 1) {
    //DPC_END
    command.end = data.bit(0,23) & ~7;
    if(command.end > command.current) {
      command.freeze = 0;
      render();
      command.ready = 1;
    }
  }

  if(address == 2) {
    //DPC_CURRENT (read-only)
  }

  if(address == 3) {
    //DPC_STATUS
    if(data.bit(0)) command.source = 0;
    if(data.bit(1)) command.source = 1;
    if(data.bit(2)) command.freeze = 0;
  //if(data.bit(3)) command.freeze = 1;
    if(data.bit(4)) command.flush = 0;
    if(data.bit(5)) command.flush = 1;
    if(data.bit(6)) command.tmemBusy = 0;
    if(data.bit(7)) command.pipeBusy = 0;
    if(data.bit(8)) command.bufferBusy = 0;
    if(data.bit(9)) command.clock = 0;
  }

  if(address == 4) {
    //DPC_CLOCK (read-only)
  }

  if(address == 5) {
    //DPC_BUSY (read-only)
  }

  if(address == 6) {
    //DPC_PIPE_BUSY (read-only)
  }

  if(address == 7) {
    //DPC_TMEM_BUSY (read-only)
  }

  debugger.ioDPC(Write, address, data);
}

auto RDP::IO::readWord(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  n32 data;

  if(address == 0) {
    //DPS_TBIST
    data.bit(0)    = bist.check;
    data.bit(1)    = bist.go;
    data.bit(2)    = bist.done;
    data.bit(3,10) = bist.fail;
  }

  if(address == 1) {
    //DPS_TEST_MODE
    data.bit(0) = test.enable;
  }

  if(address == 2) {
    //DPS_BUFTEST_ADDR
    data.bit(0,6) = test.address;
  }

  if(address == 3) {
    //DPS_BUFTEST_DATA
    data.bit(0,31) = test.data;
  }

  self.debugger.ioDPS(Read, address, data);
  return data;
}

auto RDP::IO::writeWord(u32 address, u32 data_) -> void {
  address = (address & 0xfffff) >> 2;
  n32 data = data_;

  if(address == 0) {
    //DPS_TBIST
    bist.check = data.bit(0);
    bist.go    = data.bit(1);
    if(data.bit(2)) bist.done = 0;
  }

  if(address == 1) {
    //DPS_TEST_MODE
    test.enable = data.bit(0);
  }

  if(address == 2) {
    //DPS_BUFTEST_ADDR
    test.address = data.bit(0,6);
  }

  if(address == 3) {
    //DPS_BUFTEST_DATA
    test.data = data.bit(0,31);
  }

  self.debugger.ioDPS(Write, address, data);
}
