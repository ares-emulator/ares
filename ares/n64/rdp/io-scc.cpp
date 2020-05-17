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
    data.bit(0) = command.source;
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

//print("* ", registerNamesSCC(address, "DPC_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RDP::writeSCC(u32 address, uint32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //DPC_START
    command.start = data.bit(0,23);
  }

  if(address == 1) {
    //DPC_END
    command.end = data.bit(0,23);
  //hack:
    command.current = command.end;
    auto& memory = !command.source ? rdram.ram : rsp.dmem;
    for(u32 address = command.start; address < command.end; address += 8) {
      u64 command = memory.readDouble(address);
    //print("* RDP ", hex(command, 16L), "\n");
      command = command >> 56 & 0x3f;
      if(command == 0x27) mi.raise(MI::IRQ::DP);
      if(command == 0x28) mi.raise(MI::IRQ::DP);
      if(command == 0x29) mi.raise(MI::IRQ::DP);
      if(command == 0x31) mi.raise(MI::IRQ::DP);
    }
  }

  if(address == 2) {
    //DPC_CURRENT (read-only)
  }

  if(address == 3) {
    //DPC_STATUS
    if(data.bit(0)) command.source = 0;
    if(data.bit(1)) command.source = 1;
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

//print("* ", registerNamesSCC(address, "DPC_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
