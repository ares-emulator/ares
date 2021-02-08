//$1f80:1060 RAM_SIZE
//$fffe:0130 CACHE_CTRL

auto MemoryControl::readByte(u32 address) -> u32 {
  n32 data;
  if((address & ~3) == 0x1f80'1060) data = readWord(address & ~3) >> 8 * (address & 3);
  if(address == 0xfffe'0130) data = readWord(address);  //must be word-aligned
  return data;
}

auto MemoryControl::readHalf(u32 address) -> u32 {
  n32 data;
  if((address & ~3) == 0x1f80'1060) data = readWord(address & ~3) >> 8 * (address & 3);
  if(address == 0xfffe'0130) data = readWord(address);  //must be word-aligned
  return data;
}

auto MemoryControl::readWord(u32 address) -> u32 {
  n32 data;

  if(address == 0x1f80'1060) {
    data = ram.value;
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
  }

  return data;
}

auto MemoryControl::writeByte(u32 address, u32 data) -> void {
  if((address & ~3) == 0x1f80'1060) writeWord(address & ~3, data << 8 * (address & 3));
  if(address == 0xfffe'0130) writeWord(address, data);  //must be word-aligned
}

auto MemoryControl::writeHalf(u32 address, u32 data) -> void {
  if((address & ~3) == 0x1f80'1060) writeWord(address & ~3, data << 8 * (address & 3));
  if(address == 0xfffe'0130) writeWord(address, data);  //must be word-aligned
}

auto MemoryControl::writeWord(u32 address, u32 value) -> void {
  n32 data = value;

  if(address == 0x1f80'1060) {
    ram.value = data;

    ram.delay  = ram.value.bit(7);
    ram.window = ram.value.bit(9,11);
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
  }
}
