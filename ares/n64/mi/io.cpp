static const vector<string> registerNames = {
  "MI_INIT_MODE",
  "MI_VERSION",
  "MI_INTR",
  "MI_INTR_MASK",
};

auto MI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //MI_INIT_MODE
  }

  if(address == 1) {
    //MI_VERSION
    data = 0x01010101;  //unverified
  }

  if(address == 2) {
    //MI_INTR
    data |= irq.sp.line << 0;
    data |= irq.si.line << 1;
    data |= irq.ai.line << 2;
    data |= irq.vi.line << 3;
    data |= irq.pi.line << 4;
    data |= irq.dp.line << 5;
  }

  if(address == 3) {
    //MI_INTR_MASK
    data |= irq.sp.mask << 0;
    data |= irq.si.mask << 1;
    data |= irq.ai.mask << 2;
    data |= irq.vi.mask << 3;
    data |= irq.pi.mask << 4;
    data |= irq.dp.mask << 5;
  }

//print("* ", registerNames(address, "MI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto MI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //MI_INIT_MODE
  }

  if(address == 1) {
    //MI_VERSION (read-only)
  }

  if(address == 2) {
    //MI_INTR (read-only)
  }

  if(address == 3) {
    //MI_INTR_MASK
    if(data >>  0 & 1) irq.sp.mask = 0;
    if(data >>  1 & 1) irq.sp.mask = 1;
    if(data >>  2 & 1) irq.si.mask = 0;
    if(data >>  3 & 1) irq.si.mask = 1;
    if(data >>  4 & 1) irq.ai.mask = 0;
    if(data >>  5 & 1) irq.ai.mask = 1;
    if(data >>  6 & 1) irq.vi.mask = 0;
    if(data >>  7 & 1) irq.vi.mask = 1;
    if(data >>  8 & 1) irq.pi.mask = 0;
    if(data >>  9 & 1) irq.pi.mask = 1;
    if(data >> 10 & 1) irq.dp.mask = 0;
    if(data >> 11 & 1) irq.dp.mask = 1;
    poll();
  }

//print("* ", registerNames(address, "MI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
