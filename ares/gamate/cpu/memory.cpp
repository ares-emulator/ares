auto CPU::read(n16 address) -> n8 {
  if(auto result = platform->cheat(address)) return step(1), *result;
  auto data = readBus(address);
  step(1);
  return data;
}

auto CPU::write(n16 address, n8 data) -> void {
  writeBus(address, data);
  step(1);
}

//decoding is sparse: the 1KB of RAM answers eight times over below 0x2000, and
//the sound, joypad and video registers each repeat through their 1KB page
auto CPU::readBus(n16 address) -> n8 {
  if(address <= 0x1fff) return ram.read(address & 0x03ff);
  if(address >= 0x4000 && address <= 0x43ff) return psg.read(address & 0x0f);
  if(address >= 0x4400 && address <= 0x47ff) return system.controls.read();
  if(address >= 0x4800 && address <= 0x4bff) return 0x00;  //link port, not emulated
  if(address >= 0x5000 && address <= 0x53ff) return vdp.read(address & 0x07);
  if(address == 0x5800) return cartridge.cardAvailableSet();
  if(address == 0x5a00) return cartridge.cardAvailableCheck();
  if(address >= 0x6000 && address <= 0xdfff) return cartridge.read(address - 0x6000);
  if(address >= 0xe000) return system.bios.read(address & 0x0fff);
  return 0xff;
}

auto CPU::writeBus(n16 address, n8 data) -> void {
  if(address <= 0x1fff) return ram.write(address & 0x03ff, data);
  if(address >= 0x4000 && address <= 0x43ff) return psg.write(address & 0x0f, data);
  if(address >= 0x5000 && address <= 0x53ff) return vdp.write(address & 0x07, data);
  if(address == 0x5900) return cartridge.cardReset();
  if(address >= 0x6000 && address <= 0xdfff) return cartridge.write(address - 0x6000, data);
}

//the debugger must not touch anything that latches: reading the video port
//advances the VRAM pointer, and reading 0x5800 unlocks the cartridge
auto CPU::readDebugger(n16 address) -> n8 {
  if(address <= 0x1fff) return ram.read(address & 0x03ff);
  if(address >= 0x6000 && address <= 0xdfff) return cartridge.read(address - 0x6000);
  if(address >= 0xe000) return system.bios.read(address & 0x0fff);
  return 0xff;
}

auto CPU::lastCycle() -> void {
  state.interruptPending = irqPending();
}

auto CPU::irqPending() -> bool {
  return state.irqLine && !P.i;
}
