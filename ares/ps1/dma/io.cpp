auto DMA::readByte(u32 address) -> u32 {
  print("* rb", hex(address, 8L), "\n");
  return 0;
}

auto DMA::readHalf(u32 address) -> u32 {
  print("* rh", hex(address, 8L), "\n");
  return 0;
}

auto DMA::readWord(u32 address) -> u32 {
  uint32 data;
  uint32 c = address >> 4 & 7;

  //DnMADR: DMA Base Address
  if((address & 0xffff'ff8f) == 0x1f80'1080 && c < 7) {
    data.bit(0,23) = channel[c].address;
  }

  //DnBCR: DMA Block Control
  if((address & 0xffff'ff8f) == 0x1f80'1084 && c < 7) {
    data.bit( 0,15) = channel[c].length;
    data.bit(16,31) = channel[c].blocks;
  }

  //DnCHCR: DMA Channel Control
  if((address & 0xffff'ff8f) == 0x1f80'1088 && c < 7) {
    data.bit( 0)    = channel[c].direction;
    data.bit( 1)    = channel[c].step;
    data.bit( 2)    = channel[c].chopping.enable;
    data.bit( 9,10) = channel[c].synchronization;
    data.bit(16,18) = channel[c].chopping.dmaWindow;
    data.bit(20,22) = channel[c].chopping.cpuWindow;
    data.bit(24)    = channel[c].enable;
    data.bit(28)    = channel[c].trigger;
    data.bit(29,30) = channel[c].unknown;
  }

  //DPCR: DMA Control
  if(address == 0x1f80'10f0) {
    data.bit( 0, 2) = channel[0].priority;
    data.bit( 3)    = channel[0].masterEnable;
    data.bit( 4, 6) = channel[1].priority;
    data.bit( 7)    = channel[1].masterEnable;
    data.bit( 8,10) = channel[2].priority;
    data.bit(11)    = channel[2].masterEnable;
    data.bit(12,14) = channel[3].priority;
    data.bit(15)    = channel[3].masterEnable;
    data.bit(16,18) = channel[4].priority;
    data.bit(19)    = channel[4].masterEnable;
    data.bit(20,22) = channel[5].priority;
    data.bit(23)    = channel[5].masterEnable;
    data.bit(24,26) = channel[6].priority;
    data.bit(27)    = channel[6].masterEnable;
  }

  //DICR: DMA Interrupt
  if(address == 0x1f80'10f4) {
    data.bit( 0, 5) = irq.unknown;
    data.bit(16)    = channel[0].irq.enable;
    data.bit(17)    = channel[1].irq.enable;
    data.bit(18)    = channel[2].irq.enable;
    data.bit(19)    = channel[3].irq.enable;
    data.bit(20)    = channel[4].irq.enable;
    data.bit(21)    = channel[5].irq.enable;
    data.bit(22)    = channel[6].irq.enable;
    data.bit(23)    =            irq.enable;
    data.bit(24)    = channel[0].irq.flag;
    data.bit(25)    = channel[1].irq.flag;
    data.bit(26)    = channel[2].irq.flag;
    data.bit(27)    = channel[3].irq.flag;
    data.bit(28)    = channel[4].irq.flag;
    data.bit(29)    = channel[5].irq.flag;
    data.bit(30)    = channel[6].irq.flag;
    data.bit(31)    = irq.flag;
  }

  return data;
}

auto DMA::writeByte(u32 address, u32 value) -> void {
  print("* wb", hex(address, 8L), " = ", hex(value, 2L), "\n");
}

auto DMA::writeHalf(u32 address, u32 value) -> void {
  print("* wh", hex(address, 8L), " = ", hex(value, 4L), "\n");
}

auto DMA::writeWord(u32 address, u32 value) -> void {
  uint32 data = value;
  uint32 c = address >> 4 & 7;

  //DnMADR: DMA Base Address
  if((address & 0xffff'ff8f) == 0x1f80'1080 && c < 7) {
    channel[c].address = data.bit(0,23);
  }

  //DnBCR: DMA Block Control
  if((address & 0xffff'ff8f) == 0x1f80'1084 && c < 7) {
    channel[c].length = data.bit( 0,15);
    channel[c].blocks = data.bit(16,31);
  }

  //DnCHCR: DMA Channel Control
  if((address & 0xffff'ff8f) == 0x1f80'1088 && c < 7) {
    channel[c].direction          = data.bit( 0);
    channel[c].step               = data.bit( 1);
    channel[c].chopping.enable    = data.bit( 2);
    channel[c].synchronization    = data.bit( 9,10);
    channel[c].chopping.dmaWindow = data.bit(16,18);
    channel[c].chopping.cpuWindow = data.bit(20,22);
    channel[c].enable             = data.bit(24);
    channel[c].trigger            = data.bit(28);
    channel[c].unknown            = data.bit(29,30);

    if(channel[c].active()) {
    //u32 base = address & 0xffff'fff0;
    //print("DMA", c, " ", hex(readWord(base + 8), 8L), " ", hex(readWord(base + 0), 8L), " ", hex(readWord(base + 4), 8L), "\n");

      if(channel[c].synchronization == 0) transferLinear(c);
      if(channel[c].synchronization == 1) transferLinear(c);
      if(channel[c].synchronization == 2) transferLinked(c);
      interrupt.pulse(Interrupt::DMA);
    }
  }

  //DPCR: DMA Control
  if(address == 0x1f80'10f0) {
    channel[0].priority     = data.bit( 0, 2);
    channel[0].masterEnable = data.bit( 3);
    channel[1].priority     = data.bit( 4, 6);
    channel[1].masterEnable = data.bit( 7);
    channel[2].priority     = data.bit( 8,10);
    channel[2].masterEnable = data.bit(11);
    channel[3].priority     = data.bit(12,14);
    channel[3].masterEnable = data.bit(15);
    channel[4].priority     = data.bit(16,18);
    channel[4].masterEnable = data.bit(19);
    channel[5].priority     = data.bit(20,22);
    channel[5].masterEnable = data.bit(23);
    channel[6].priority     = data.bit(24,26);
    channel[6].masterEnable = data.bit(27);
  }

  //DICR: DMA Interrupt
  if(address == 0x1f80'10f4) {
               irq.unknown = data.bit( 0,5);
               irq.force   = data.bit(15);
    channel[0].irq.enable  = data.bit(16);
    channel[1].irq.enable  = data.bit(17);
    channel[2].irq.enable  = data.bit(18);
    channel[3].irq.enable  = data.bit(19);
    channel[4].irq.enable  = data.bit(20);
    channel[5].irq.enable  = data.bit(21);
    channel[6].irq.enable  = data.bit(22);
               irq.enable  = data.bit(23);
    if(data.bit(24)) channel[0].irq.flag = 0;
    if(data.bit(25)) channel[1].irq.flag = 0;
    if(data.bit(26)) channel[2].irq.flag = 0;
    if(data.bit(27)) channel[3].irq.flag = 0;
    if(data.bit(28)) channel[4].irq.flag = 0;
    if(data.bit(29)) channel[5].irq.flag = 0;
    if(data.bit(30)) channel[6].irq.flag = 0;
    irq.poll();
  }
}
