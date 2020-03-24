/* the APU can write to CPU RAM, but it cannot read from CPU RAM:
 * the exact returned value varies per system, but it always fails.
 * it is unknown which other regions of the bus are inaccessible to the APU.
 * it would certainly go very badly if the APU could reference itself at $a0xxxx.
 * for now, assume that only the cartridge and expansion buses are also accessible.
 */

auto APU::read(uint16 address) -> uint8 {
  //$2000-3fff mirrors $0000-1fff
  if(address >= 0x0000 && address <= 0x3fff) return ram.read(address);
  if(address >= 0x4000 && address <= 0x4003) return ym2612.readStatus();
  if(address >= 0x8000 && address <= 0xffff) {
    while(vdp.dma.active) {
      Thread::step(1);
      Thread::synchronize(vdp);
    }

    //bus arbiter delay rough approximation
    cpu.idle(11);
    step(3);

    uint24 location = io.bank << 15 | (uint15)address & ~1;
    if(location >= 0xa00000 && location <= 0xffffff) {
      //todo: apparently *some* I/O addresses can be read or written from the Z80.
      //it is not currently known which addresses are accepted.
      if(location != 0xa10000) return 0xff;  //version register can be read
    }
    if(address & 1) {
      return cpu.read(0, 1, location & ~1, 0x00).byte(0);
    } else {
      return cpu.read(1, 0, location & ~1, 0x00).byte(1);
    }
  }
  return 0x00;
}

auto APU::write(uint16 address, uint8 data) -> void {
  //$2000-3fff mirrors $0000-1fff
  if(address >= 0x0000 && address <= 0x3fff) return ram.write(address, data);
  if(address == 0x4000) return ym2612.writeAddress(0 << 8 | data);
  if(address == 0x4001) return ym2612.writeData(data);
  if(address == 0x4002) return ym2612.writeAddress(1 << 8 | data);
  if(address == 0x4003) return ym2612.writeData(data);
  if(address == 0x6000) return (void)(io.bank = data.bit(0) << 8 | io.bank >> 1);
  if(address == 0x7f11) return psg.write(data);
  if(address == 0x7f13) return psg.write(data);
  if(address == 0x7f15) return psg.write(data);
  if(address == 0x7f17) return psg.write(data);
  if(address >= 0x8000 && address <= 0xffff) {
    while(vdp.dma.active) {
      Thread::step(1);
      Thread::synchronize(vdp);
    }

    //bus arbiter delay rough approximation
    cpu.idle(11);
    step(3);

    uint24 location = io.bank << 15 | (uint15)address;
    if(location >= 0xa00000 && location <= 0xdfffff) return;
    if(address & 1) {
      return cpu.write(0, 1, location & ~1, data << 8 | data << 0);
    } else {
      return cpu.write(1, 0, location & ~1, data << 8 | data << 0);
    }
  }
}

//unused on Mega Drive
auto APU::in(uint16 address) -> uint8 {
  return 0x00;
}

//unused on Mega Drive
auto APU::out(uint16 address, uint8 data) -> void {
}
