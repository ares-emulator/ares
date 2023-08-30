auto CPU::DMA::valid(n20 address) -> bool {
  return cpu.speed(address) == 1 && cpu.width(address) == Word;
}

auto CPU::DMA::transfer() -> void {
  //length of 0 or SRAM source address cause immediate termination
  if(length == 0 || !valid(source)) {
    enable = 0;
    return;
  }

  self.step(5);
  while(length) {
    self.step(2);
    u16 data = 0;
    //once DMA is started; SRAM reads still incur time penalty, but do not transfer
    if(valid(source)) {
      data |= self.read(source + 0) << 0;
      data |= self.read(source + 1) << 8;
      self.write(target + 0, data >> 0);
      self.write(target + 1, data >> 8);
    }
    source += direction ? -2 : +2;
    target += direction ? -2 : +2;
    length -= 2;
  };
  enable = 0;
}
