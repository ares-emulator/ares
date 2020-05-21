static const vector<string> registerNamesSCC = {
  "DPC_START",
  "DPC_END",
  "DPC_CURRENT",
  "DPC_STATUS",
  "DPC_CLOCK",
  "DPC_BUSY",
  "DPC_PIPE_BUSY",
  "DPC_TMEM_BUSY",
};

auto RDP::readSCC(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  uint32 data;

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

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNamesSCC(address, "DPC_UNKNOWN"), " => ", hex(data, 8L)});
  }
  return data;
}

auto RDP::writeSCC(u32 address, uint32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //DPC_START
    command.start = data.bit(0,23) & ~7;
    command.current = command.start;
  }

  if(address == 1) {
    //DPC_END
    command.end = data.bit(0,23) & ~7;
    command.freeze = 0;
    render();
    command.current = command.end;
    command.ready = 1;
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

  if(debugger.tracer.io->enabled()) {
    debugger.io({registerNamesSCC(address, "DPC_UNKNOWN"), " <= ", hex(data, 8L)});
  }
}
