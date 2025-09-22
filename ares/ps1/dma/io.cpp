auto DMA::readByte(u32 address) -> u32 {
  n32 data;
  if((address & ~3) == 0x1f80'10f0) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & ~3) == 0x1f80'10f4) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & ~3) == 0x1f80'10f8) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & ~3) == 0x1f80'10fc) return data = readWord(address & ~3) >> 8 * (address & 3);
  auto& channel = channels[address >> 4 & 7];
  if((address & 0x1fff'ff8c) == 0x1f80'1080) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & 0x1fff'ff8f) == 0x1f80'1084) return data = channel.length.byte(0);
  if((address & 0x1fff'ff8f) == 0x1f80'1085) return data = channel.length.byte(1);
  if((address & 0x1fff'ff8f) == 0x1f80'1086) return data = channel.blocks.byte(0);
  if((address & 0x1fff'ff8f) == 0x1f80'1087) return data = channel.blocks.byte(1);
  if((address & 0x1fff'ff8c) == 0x1f80'1088) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & 0x1fff'ff8c) == 0x1f80'108c) return data = readWord(address & ~3) >> 8 * (address & 3);
  debug(unhandled, "DMA::readByte(", hex(address, 8L), ") -> ", hex(data, 2L));
  return data;
}

auto DMA::readHalf(u32 address) -> u32 {
  n32 data;
  if((address & ~3) == 0x1f80'10f0) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & ~3) == 0x1f80'10f4) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & ~3) == 0x1f80'10f8) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & ~3) == 0x1f80'10fc) return data = readWord(address & ~3) >> 8 * (address & 3);
  auto& channel = channels[address >> 4 & 7];
  if((address & 0x1fff'ff8c) == 0x1f80'1080) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & 0x1fff'ff8e) == 0x1f80'1084) return data = channel.length;
  if((address & 0x1fff'ff8e) == 0x1f80'1086) return data = channel.blocks;
  if((address & 0x1fff'ff8c) == 0x1f80'1088) return data = readWord(address & ~3) >> 8 * (address & 3);
  if((address & 0x1fff'ff8c) == 0x1f80'108c) return data = readWord(address & ~3) >> 8 * (address & 3);
  debug(unhandled, "DMA::readHalf(", hex(address, 8L), ") -> ", hex(data, 4L));
  return data;
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
    return data;
  }

  //DICR: DMA Interrupt
  if(address == 0x1f80'10f4) {
    data.bit( 0, 5) = irq.unknown;
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

  //unknown
  if(address == 0x1f80'10f8) {
    data = 0x7fe3'58d1;
    return data;
  }

  //unknown
  if(address == 0x1f80'10fc) {
    data = 0x00ff'fff7;
    return data;
  }

  auto& channel = channels[address >> 4 & 7];

  //DnMADR: DMA Base Address
  if((address & 0x1fff'ff8f) == 0x1f80'1080) {
    data.bit(0,23) = channel.address;
    return data;
  }

  //DnBCR: DMA Block Control
  if((address & 0x1fff'ff8f) == 0x1f80'1084) {
    data.bit( 0,15) = channel.length;
    data.bit(16,31) = channel.blocks;
    return data;
  }

  //DnCHCR: DMA Channel Control
  if((address & 0x1fff'ff8f) == 0x1f80'1088
  || (address & 0x1fff'ff8f) == 0x1f80'108c
  ) {
    data.bit( 0)    = channel.direction;
    data.bit( 1)    = channel.decrement;
    data.bit( 2)    = channel.chopping.enable;
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
  n32 data = value;
  if((address & ~3) == 0x1f80'10f0) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & ~3) == 0x1f80'10f4) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & ~3) == 0x1f80'10f8) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & ~3) == 0x1f80'10fc) return writeWord(address & ~3, data << 8 * (address & 3));
  auto& channel = channels[address >> 4 & 7];
  if((address & 0x1fff'ff8c) == 0x1f80'1080) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & 0x1fff'ff8f) == 0x1f80'1084) { channel.length.byte(0) = data; return; }
  if((address & 0x1fff'ff8f) == 0x1f80'1085) { channel.length.byte(1) = data; return; }
  if((address & 0x1fff'ff8f) == 0x1f80'1086) { channel.blocks.byte(0) = data; return; }
  if((address & 0x1fff'ff8f) == 0x1f80'1087) { channel.blocks.byte(1) = data; return; }
  if((address & 0x1fff'ff8c) == 0x1f80'1088) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & 0x1fff'ff8c) == 0x1f80'108c) return writeWord(address & ~3, data << 8 * (address & 3));

  debug(unhandled, "DMA::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto DMA::writeHalf(u32 address, u32 value) -> void {
  n32 data = value;
  if((address & ~3) == 0x1f80'10f0) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & ~3) == 0x1f80'10f4) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & ~3) == 0x1f80'10f8) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & ~3) == 0x1f80'10fc) return writeWord(address & ~3, data << 8 * (address & 3));
  auto& channel = channels[address >> 4 & 7];
  if((address & 0x1fff'ff8c) == 0x1f80'1080) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & 0x1fff'fffe) == 0x1f80'1084) { channel.length = data; return; }
  if((address & 0x1fff'fffe) == 0x1f80'1086) { channel.blocks = data; return; }
  if((address & 0x1fff'ff8c) == 0x1f80'1088) return writeWord(address & ~3, data << 8 * (address & 3));
  if((address & 0x1fff'ff8c) == 0x1f80'108c) return writeWord(address & ~3, data << 8 * (address & 3));

  debug(unhandled, "DMA::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
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
    sortChannelsByPriority();
    return;
  }

  //DICR: DMA Interrupt
  if(address == 0x1f80'10f4) {
                irq.unknown = data.bit( 0,5);
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
    channel.address = data.bit(0,23);
    return;
  }

  //DnBCR: DMA Block Control
  if((address & 0x1fff'ff8f) == 0x1f80'1084) {
    channel.length = data.bit( 0,15);
    channel.blocks = data.bit(16,31);
    return;
  }

  //DnCHCR: DMA Channel Control
  if((address & 0x1fff'ff8f) == 0x1f80'1088
  || (address & 0x1fff'ff8f) == 0x1f80'108c
  ) {
    channel.direction          = data.bit( 0);
    channel.decrement          = data.bit( 1);
    channel.chopping.enable    = data.bit( 2);
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

    channel.state = Idle;
    channel.chain.length = 0;

    for(u32 id : channelsByPriority) {
      if(channels[id].kick()) break;
    }

    return;
  }

  debug(unhandled, "DMA::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ")");
}
