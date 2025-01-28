auto Virage::commandFinished() -> void {
  u8 operation = io.command.bit(24,31);

  switch(operation) {
    case 2: { //Save sram to flash
      for (auto i : range(flash.size))
        flash.data[i] = sram.data[i];
      io.storeDone = 1;
    } break;

    case 3: { //Reload sram from flash
      for (auto i : range(flash.size))
        sram.data[i] = flash.data[i];
      io.loadDone = 1;
    } break;

    default: {
      debug(unimplemented, "[Virage::command] command=", hex(io.command, 8L));
    } break;
  }

  io.unk30 = 1;
  io.busy = 0;
}

auto Virage::command(u32 command) -> void {
  if (io.busy) {
    debug(unusual, "[Virage::command] Sent command ", hex(command, 8L), " while busy with last command ", hex(io.command, 8L));
    return;
  }
  io.unk30 = 0;
  io.busy = 1;
  io.command = command;
  queue.insert(queueID, 2000*3); //TODO: cycles
}

auto Virage::readWord(u32 address, Thread& thread) -> u32 {
  if (!mi.secure()) {
    debug(unusual, "[Virage::readWord] Attempted read from Virage address space outside of secure mode");
    return 0;
  }

  address &= 0xFFFF;
  n32 data;

  if(address < 0x8000) { //SRAM
    data = sram.read<Word>(address);
    debugger.ioSRAM(Read, address, data);
  } else if(address < 0xC000) {
    //Configuration registers?
    address = (address & 0x1F) >> 2;

    if(address == 6 || address == 7)
      debug(unimplemented, "[Virage::readWord] Uncaught read from Virage config register ", address);

    data = io.configReg[address];
    debugger.ioConfigReg(Read, address, data);
  } else if(address < 0xE000) {
    //control/status register
    data.bit(0) = io.busy;
    data.bit(23) = io.loadDone;
    data.bit(29) = io.storeDone;
    data.bit(30) = io.unk30;
    debugger.ioStatusReg(Read, data);
  } else {
    //Command register
    debug(unimplemented, "[Virage::readWord] Read from Virage command register");
  }
  return data;
}

auto Virage::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if (!mi.secure()) {
    debug(unusual, "[Virage::writeWord] Attempted write to Virage address space outside of secure mode");
    return;
  }

  address &= 0xFFFF;

  if(address < 0x8000) { //SRAM
    sram.write<Word>(address, data);
    debugger.ioSRAM(Write, address, data);
  } else if(address < 0xC000) {
    //Configuration registers?
    address = (address & 0x1F) >> 2;

    if(address == 6 || address == 7)
      debug(unimplemented, "[Virage::writeWord] Uncaught write to Virage config register ", address);

    io.configReg[address] = data;
    debugger.ioConfigReg(Write, address, data);
  } else if(address < 0xE000) { //Control/status register
    io.unk30 = 1;
    debugger.ioStatusReg(Write, data);
  } else {
    //Command register
    command(data);
    debugger.ioControlReg(Write, data);
  }
}
