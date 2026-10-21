//$1f80:1060 RAM_SIZE
//$fffe:0130 CACHE_CTRL

auto MemoryControl::readByte(u32 address) -> u32 {
  n32 data;
  if(address == 0xfffe'0130) return data = readWord(address);  //must be word-aligned
  debug(unhandled, "MemoryControl::readByte(", hex(address, 8L), ")");
  return data;
}

auto MemoryControl::readHalf(u32 address) -> u32 {
  n32 data;
  if(address == 0xfffe'0130) return data = readWord(address);  //must be word-aligned
  debug(unhandled, "MemoryControl::readHalf(", hex(address, 8L), ")");
  return data;
}

auto MemoryControl::readWord(u32 address) -> u32 {
  n32 data;

  if(address == 0x1f80'1000) return data = 0x1f00'0000 | exp1Base.value.bit(0,23);
  if(address == 0x1f80'1004) return data = 0x1f00'0000 | exp2Base.value.bit(0,23);

  if(address == 0x1f80'1060) {
    return data = ram.value;
  }

  if(address == 0xfffe'0130) {
    data.bit( 0) = cache.lock;
    data.bit( 1) = cache.invalidate;
    data.bit( 2) = cache.tagTest;
    data.bit( 3) = cache.scratchpadEnable;
    data.bit( 4) = cache.dataSize.bit(0);
    data.bit( 5) = cache.dataSize.bit(1);
    data.bit( 6) = 0;  //cache.dataDisable
    data.bit( 7) = cache.dataEnable;
    data.bit( 8) = cache.codeSize.bit(0);
    data.bit( 9) = cache.codeSize.bit(1);
    data.bit(10) = 0;  //cache.codeDisable
    data.bit(11) = cache.codeEnable;
    data.bit(12) = cache.interruptPolarity;
    data.bit(13) = cache.readPriority;
    data.bit(14) = cache.noWaitState;
    data.bit(15) = cache.busGrant;
    data.bit(16) = cache.loadScheduling;
    data.bit(17) = cache.noStreaming;
    data.bit(18,31) = cache.reserved;
    return data;
  }

  auto readPort = [&](const MemPort& port, bool exp1 = false) -> n32 {
    n32 value = 0;
    value.bit( 0, 3) = port.writeDelay;
    value.bit( 4, 7) = port.readDelay;
    value.bit( 8)     = port.recovery;
    value.bit( 9)     = port.hold;
    value.bit(10)     = port.floating;
    value.bit(11)     = port.preStrobe;
    value.bit(12)     = port.dataWidth;
    value.bit(13)     = port.autoIncrement;
    value.bit(14,15) = port.unknown14_15;
    value.bit(16,20) = port.addrBits;
    value.bit(24,27) = port.dmaTiming;
    value.bit(29)     = port.dmaSelect;
    if(exp1) value.bit(31) = port.wait;
    return value;
  };

  if(address == 0x1f80'1008) return data = readPort(exp1, true);
  if(address == 0x1f80'100c) return data = readPort(exp3);
  if(address == 0x1f80'1010) return data = readPort(bios);
  if(address == 0x1f80'1014) return data = readPort(spu);
  if(address == 0x1f80'1018) return data = readPort(cdrom);
  if(address == 0x1f80'101c) return data = readPort(exp2);

  if(address == 0x1f80'1020) {
    data.bit( 0, 3)  = common.com0;
    data.bit( 4, 7)  = common.com1;
    data.bit( 8,11)  = common.com2;
    data.bit(12,15)  = common.com3;
    data.bit(16,17)  = common.unused.bit(0,1);
    return data;
  }

  debug(unhandled, "MemoryControl::readWord(", hex(address, 8L), ")");
  return data;
}

auto MemoryControl::writeByte(u32 address, u32 data) -> void {
  if(address == 0xfffe'0130) return writeWord(address, data);
  debug(unhandled, "MemoryControl::writeByte(", hex(address, 8L), ", ", hex(data, 2L), ")");
}

auto MemoryControl::writeHalf(u32 address, u32 data) -> void {
  if(address == 0xfffe'0130) return writeWord(address, data);
  debug(unhandled, "MemoryControl::writeHalf(", hex(address, 8L), ", ", hex(data, 4L), ")");
}

auto MemoryControl::writeWord(u32 address, u32 value) -> void {
  n32 data = value;

  if(address == 0x1f80'1000) {
    exp1Base.value = 0x1f00'0000 | data.bit(0,23);
    updateConfiguration();
    return;
  }

  if(address == 0x1f80'1004) {
    exp2Base.value = 0x1f00'0000 | data.bit(0,23);
    updateConfiguration();
    return;
  }

  if(address == 0x1f80'1060) {
    ram.value = data;
    return;
  }

  if(address == 0xfffe'0130) {
    cache.lock              = data.bit( 0);
    cache.invalidate        = data.bit( 1);
    cache.tagTest           = data.bit( 2);
    cache.scratchpadEnable  = data.bit( 3);
    cache.dataSize.bit(0)   = data.bit( 4);
    cache.dataSize.bit(1)   = data.bit( 5);
  //cache.dataDisable       = data.bit( 6);  //forced to zero on the PS1
    cache.dataEnable        = data.bit( 7);
    cache.codeSize.bit(0)   = data.bit( 8);
    cache.codeSize.bit(1)   = data.bit( 9);
  //cache.codeDisable       = data.bit(10);  //forced to zero on the PS1
    cache.codeEnable        = data.bit(11);
    cache.interruptPolarity = data.bit(12);  //should be 0 for normal operation
    cache.readPriority      = data.bit(13);  //should be 1
    cache.noWaitState       = data.bit(14);  //should be 1
    cache.busGrant          = data.bit(15);  //should be 1
    cache.loadScheduling    = data.bit(16);  //should be 1
    cache.noStreaming       = data.bit(17);  //should be 0
    cache.reserved          = data.bit(18,31);

    cpu.icache.enable(cache.codeEnable);
    return;
  }

  auto writePort = [&](MemPort& port) {
    port.writeDelay     = data.bit(0,3);
    port.readDelay      = data.bit(4,7);
    port.recovery       = data.bit(8);
    port.hold           = data.bit(9);
    port.floating       = data.bit(10);
    port.preStrobe      = data.bit(11);
    port.dataWidth      = data.bit(12);
    port.autoIncrement  = data.bit(13);
    port.unknown14_15   = data.bit(14,15);
    port.addrBits       = data.bit(16,20);
    port.dmaTiming      = data.bit(24,27);
    port.dmaSelect      = data.bit(29);
    port.wait           = data.bit(31);
    updateConfiguration();
  };

  if(address == 0x1f80'1008) return writePort(exp1);
  if(address == 0x1f80'100c) return writePort(exp3);
  if(address == 0x1f80'1010) return writePort(bios);
  if(address == 0x1f80'1014) return writePort(spu);
  if(address == 0x1f80'1018) return writePort(cdrom);
  if(address == 0x1f80'101c) return writePort(exp2);

  if(address == 0x1f80'1020) {  // COM delays
    common.com0   = data.bit(0,3);
    common.com1   = data.bit(4,7);
    common.com2   = data.bit(8,11);
    common.com3   = data.bit(12,15);
    common.unused = data.bit(16,17);
    updateConfiguration();
    return;
  }

  debug(unhandled, "MemoryControl::writeWord(", hex(address, 8L), ", ", hex(data, 8L), ")");
}
