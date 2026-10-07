auto DMA::readByte(u32 address) -> u32 {
  return readWord(address & ~3) >> 8 * (address & 3) & 0xff;
}

auto DMA::readHalf(u32 address) -> u32 {
  return readWord(address & ~3) >> 8 * (address & 2) & 0xffff;
}

auto DMA::readWord(u32 address) -> u32 {
  n32 data;

  //DPCR: DMA Control
  if(address == 0x1f80'10f0) {
    data.bit( 0, 2) = channels[0].priority;
    data.bit( 3)    = channels[0].masterEnable;
    data.bit( 4, 6) = channels[1].priority;
    data.bit( 7)    = channels[1].masterEnable;
    data.bit( 8,10) = channels[2].priority;
    data.bit(11)    = channels[2].masterEnable;
    data.bit(12,14) = channels[3].priority;
    data.bit(15)    = channels[3].masterEnable;
    data.bit(16,18) = channels[4].priority;
    data.bit(19)    = channels[4].masterEnable;
    data.bit(20,22) = channels[5].priority;
    data.bit(23)    = channels[5].masterEnable;
    data.bit(24,26) = channels[6].priority;
    data.bit(27)    = channels[6].masterEnable;
    data.bit(28,31) = cpuControl;
    return data;
  }

  //DICR: DMA Interrupt
  if(address == 0x1f80'10f4) {
    data.bit( 0, 6) = irq.unknown;
    data.bit(15)    = irq.force;
    data.bit(16)    = channels[0].irq.enable;
    data.bit(17)    = channels[1].irq.enable;
    data.bit(18)    = channels[2].irq.enable;
    data.bit(19)    = channels[3].irq.enable;
    data.bit(20)    = channels[4].irq.enable;
    data.bit(21)    = channels[5].irq.enable;
    data.bit(22)    = channels[6].irq.enable;
    data.bit(23)    =             irq.enable;
    data.bit(24)    = channels[0].irq.flag;
    data.bit(25)    = channels[1].irq.flag;
    data.bit(26)    = channels[2].irq.flag;
    data.bit(27)    = channels[3].irq.flag;
    data.bit(28)    = channels[4].irq.flag;
    data.bit(29)    = channels[5].irq.flag;
    data.bit(30)    = channels[6].irq.flag;
    data.bit(31)    = irq.flag;
    return data;
  }

  //Reference fallback for undocumented registers; no dynamic hardware model is established.
  if(address >= 0x1f80'10f8 || (address & 15) == 12) return 0xffff'ffff;

  auto& channel = channels[address >> 4 & 7];

  //DnMADR: DMA Base Address
  if((address & 0x1fff'ff8f) == 0x1f80'1080) {
    data.bit(0,23) = channel.baseAddress;
    return data;
  }

  //DnBCR: DMA Block Control
  if((address & 0x1fff'ff8f) == 0x1f80'1084) {
    data.bit( 0,15) = channel.baseLength;
    data.bit(16,31) = channel.blocks;
    return data;
  }

  //DnCHCR: DMA Channel Control
  if((address & 0x1fff'ff8f) == 0x1f80'1088) {
    data.bit( 0)    = channel.direction;
    data.bit( 1)    = channel.decrement;
    data.bit( 8)    = channel.chopping.enable;
    data.bit( 9,10) = channel.synchronization;
    data.bit(16,18) = channel.chopping.dmaWindow;
    data.bit(20,22) = channel.chopping.cpuWindow;
    data.bit(24)    = channel.enable;
    data.bit(28)    = channel.trigger;
    data.bit(29,30) = channel.unknown;
    return data;
  }

  debug(unhandled, "DMA::readWord(", hex(address, 8L), ") -> ", hex(data, 8L));
  return data;
}

auto DMA::writeByte(u32 address, u32 value) -> void {
  // Reference-compatible lane writes replace the full register, including BCR.
  writeWord(address & ~3, (value & 0xff) << 8 * (address & 3));
}

auto DMA::writeHalf(u32 address, u32 value) -> void {
  writeWord(address & ~3, (value & 0xffff) << 8 * (address & 2));
}

auto DMA::writeWord(u32 address, u32 value) -> void {
  n32 data = value;

  //DPCR: DMA Control
  if(address == 0x1f80'10f0) {
    channels[0].priority     = data.bit( 0, 2);
    channels[0].masterEnable = data.bit( 3);
    channels[1].priority     = data.bit( 4, 6);
    channels[1].masterEnable = data.bit( 7);
    channels[2].priority     = data.bit( 8,10);
    channels[2].masterEnable = data.bit(11);
    channels[3].priority     = data.bit(12,14);
    channels[3].masterEnable = data.bit(15);
    channels[4].priority     = data.bit(16,18);
    channels[4].masterEnable = data.bit(19);
    channels[5].priority     = data.bit(20,22);
    channels[5].masterEnable = data.bit(23);
    channels[6].priority     = data.bit(24,26);
    channels[6].masterEnable = data.bit(27);
    cpuControl = data.bit(28,31);
    sortChannelsByPriority();
    return;
  }

  //DICR: DMA Interrupt
  if(address == 0x1f80'10f4) {
                irq.unknown = data.bit( 0,6);
                irq.force   = data.bit(15);
    channels[0].irq.enable  = data.bit(16);
    channels[1].irq.enable  = data.bit(17);
    channels[2].irq.enable  = data.bit(18);
    channels[3].irq.enable  = data.bit(19);
    channels[4].irq.enable  = data.bit(20);
    channels[5].irq.enable  = data.bit(21);
    channels[6].irq.enable  = data.bit(22);
                irq.enable  = data.bit(23);
    if(data.bit(24)) channels[0].irq.flag = 0;
    if(data.bit(25)) channels[1].irq.flag = 0;
    if(data.bit(26)) channels[2].irq.flag = 0;
    if(data.bit(27)) channels[3].irq.flag = 0;
    if(data.bit(28)) channels[4].irq.flag = 0;
    if(data.bit(29)) channels[5].irq.flag = 0;
    if(data.bit(30)) channels[6].irq.flag = 0;
    irq.poll();
    return;
  }

  //unused
  if(address == 0x1f80'10f8) {
    debug(unusual, "DMA::writeWord(): write to unused register 0x1f8010f8");
    return;
  }

  //unused
  if(address == 0x1f80'10fc) {
    debug(unusual, "DMA::writeWord(): write to unused register 0x1f8010c0");
    return;
  }

  auto& channel = channels[address >> 4 & 7];

  //DnMADR: DMA Base Address
  if((address & 0x1fff'ff8f) == 0x1f80'1080) {
    channel.address = channel.baseAddress = data.bit(0,23);
    return;
  }

  //DnBCR: DMA Block Control
  if((address & 0x1fff'ff8f) == 0x1f80'1084) {
    channel.length = channel.baseLength = data.bit( 0,15);
    channel.blocks = data.bit(16,31);
    return;
  }

  //DnCHCR: DMA Channel Control
  if((address & 0x1fff'ff8f) == 0x1f80'1088) {
    bool wasEnabled = channel.enable;
    u32 previousMode = channel.synchronization;
    u32 previousDirection = channel.direction;
    u32 previousDecrement = channel.decrement;
    channel.direction          = data.bit( 0);
    channel.decrement          = data.bit( 1);
    channel.chopping.enable    = data.bit( 8);
    channel.synchronization    = data.bit( 9,10);
    channel.chopping.dmaWindow = data.bit(16,18);
    channel.chopping.cpuWindow = data.bit(20,22);
    channel.enable             = data.bit(24);
    channel.trigger            = data.bit(28);
    channel.unknown            = data.bit(29,30);

    if(channel.id == OTC) {
      //OTC DMA hard-codes certain fields:
      channel.direction = 0;
      channel.decrement = 1;
      channel.chopping.enable = 0;
      channel.synchronization = 0;
      channel.chopping.dmaWindow = 0;
      channel.chopping.cpuWindow = 0;
      channel.unknown.bit(0) = 0;
    }

    bool restart = !wasEnabled || !channel.enable || previousMode != channel.synchronization
      || previousDirection != channel.direction || previousDecrement != channel.decrement;
    if(restart) {
      bus.release(Bus::dmaOwner(channel.id));
      channel.address = channel.baseAddress;
      channel.length = channel.baseLength;
      channel.state = Idle;
      channel.blockOffset = 0;
      channel.chopping.remaining = 0;
      channel.chain = {};
      channel.forced = 0;
    }

    for(u32 id : channelsByPriority) {
      if(channels[id].kick()) break;
    }

    return;
  }

  debug(unhandled, "DMA::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ")");
}
