auto RDRAM::readWord(u32 address, Thread& thread) -> u32 {
  n1 broadcast = address >> 19 & 1;
  u32 select = address >> 10 & 0x1ff;
  u32 offset = address & 0x3ff;

  if(broadcast || !ri.active()) {
    debugger.io(Read, select, 0, 0);
    return 0;
  }

  if(offset >= 0x200) {
    auto chip = selectChip(select, false);
    u32 data = chip ? (u32)chip->row : 0;
    debugger.io(Read, chip ? chipIndex(chip) : select, 10, data);
    return data;
  }

  u32 index = (offset >> 2) & 15;
  auto chip = selectChip(select, false);
  if(!chip) {
    debugger.io(Read, select, index, 0);
    return 0;
  }

  u32 data = readRegister(*chip, index);
  debugger.io(Read, chipIndex(chip), index, data);
  return data;
}

auto RDRAM::writeWord(u32 address, u32 data, Thread& thread, u8 repeatLength) -> void {
  n1 broadcast = address >> 19 & 1;
  u32 select = address >> 10 & 0x1ff;
  u32 offset = address & 0x3ff;

  if(!ri.active()) return;

  if(offset >= 0x200) {
    if(broadcast) {
      for(auto& chip : chips) {
        if(chip.present) chip.row = data;
      }
    } else if(auto chip = selectChip(select, false)) {
      chip->row = data;
    }
    debugger.io(Write, select, 10, data);
    return;
  }

  u32 index = (offset >> 2) & 15;

  if(broadcast) {
    for(auto& chip : chips) {
      if(!chip.present) continue;
      writeRegister(chip, index, data, repeatLength);
    }
    debugger.io(Write, 0xff, index, data);
    return;
  }

  auto chip = selectChip(select, false);
  if(!chip) return;

  writeRegister(*chip, index, data, repeatLength);
  debugger.io(Write, chipIndex(chip), index, data);
}
