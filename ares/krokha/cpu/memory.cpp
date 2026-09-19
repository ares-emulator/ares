//the address decoding below mirrors MAME's krokha_mem; the real board almost
//certainly decodes fewer lines than this, but no test case exists to prove it

auto CPU::read(n16 address) -> n8 {
  if(auto result = platform->cheat(address)) return *result;

  if(address <= 0x1fff) return cartridge.read(address);
  if(address >= 0xe000 && address <= 0xefff) return ram.read(address & 0x7ff);
  if(address == 0xf7ff) return system.controls.read();

  return 0xff;  //open bus pulls high
}

auto CPU::write(n16 address, n8 data) -> void {
  if(address >= 0xe000 && address <= 0xefff) return ram.write(address & 0x7ff, data);
  if(address == 0xf7ff) return psg.write(data);
}

auto CPU::in(n16 address) -> n8 {
  return 0xff;  //no I/O space: everything is memory mapped
}

auto CPU::out(n16 address, n8 data) -> void {
}
