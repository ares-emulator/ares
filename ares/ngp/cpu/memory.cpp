/* Abstract memory bus implementation.
 * The TLCS900/H can perform 8-bit (Byte), 16-bit (Word), and 32-bit (Long) reads.
 * The actual internal bus of the CPU is always 16-bit.
 * Each memory object on the bus may allow either 8-bit or 16-bit accesses.
 * For CS0-CS3 and CSX, the bus width may be changed dynamically.
 * Further, CS0-3 and CSX allow optional wait states to be added to accesses.
 * Finally, CS0-3 and CSX have registers to control where they appear on the bus.
 * CS2 has an additional fixed addressing mode setting.
 */

auto CPU::Bus::wait() -> void {
  switch(timing) {
  case 0: return cpu.step(2 + 1);  //1 state
  case 1: return cpu.step(2 + 2);  //2 states
  case 2: return cpu.step(2 + 1);  //1 state + (not emulated) /WAIT
  case 3: return cpu.step(2 + 0);  //0 states
  }
  unreachable;
}

auto CPU::Bus::read(uint size, uint24 address) -> uint32 {
  uint32 data;

  if(width == Byte) {
    if(size == Byte) {
      wait();
      data.byte(0) = reader(address + 0);
    }

    if(size == Word) {
      wait();
      data.byte(0) = reader(address + 0);
      wait();
      data.byte(1) = reader(address + 1);
    }

    if(size == Long) {
      wait();
      data.byte(0) = reader(address + 0);
      wait();
      data.byte(1) = reader(address + 1);
      wait();
      data.byte(2) = reader(address + 2);
      wait();
      data.byte(3) = reader(address + 3);
    }
  }

  if(width == Word) {
    if(size == Byte) {
      wait();
      data.byte(0) = reader(address + 0);
    }

    if(size == Word && address.bit(0) == 0) {
      wait();
      data.byte(0) = reader(address + 0);
      data.byte(1) = reader(address + 1);
    }

    if(size == Word && address.bit(0) == 1) {
      wait();
      data.byte(0) = reader(address + 0);
      wait();
      data.byte(1) = reader(address + 1);
    }

    if(size == Long && address.bit(0) == 0) {
      wait();
      data.byte(0) = reader(address + 0);
      data.byte(1) = reader(address + 1);
      wait();
      data.byte(2) = reader(address + 2);
      data.byte(3) = reader(address + 3);
    }

    if(size == Long && address.bit(0) == 1) {
      wait();
      data.byte(0) = reader(address + 0);
      wait();
      data.byte(1) = reader(address + 1);
      data.byte(2) = reader(address + 2);
      wait();
      data.byte(3) = reader(address + 3);
    }
  }

  return data;
}

auto CPU::Bus::write(uint size, uint24 address, uint32 data) -> void {
  if(width == Byte) {
    if(size == Byte) {
      wait();
      writer(address + 0, data.byte(0));
    }

    if(size == Word) {
      wait();
      writer(address + 0, data.byte(0));
      wait();
      writer(address + 1, data.byte(1));
    }

    if(size == Long) {
      wait();
      writer(address + 0, data.byte(0));
      wait();
      writer(address + 1, data.byte(1));
      wait();
      writer(address + 2, data.byte(2));
      wait();
      writer(address + 3, data.byte(3));
    }
  }

  if(width == Word) {
    if(size == Byte) {
      wait();
      writer(address + 0, data.byte(0));
    }

    if(size == Word && address.bit(0) == 0) {
      wait();
      writer(address + 0, data.byte(0));
      writer(address + 1, data.byte(1));
    }

    if(size == Word && address.bit(0) == 1) {
      wait();
      writer(address + 0, data.byte(0));
      wait();
      writer(address + 1, data.byte(1));
    }

    if(size == Long && address.bit(0) == 0) {
      wait();
      writer(address + 0, data.byte(0));
      writer(address + 1, data.byte(1));
      wait();
      writer(address + 2, data.byte(2));
      writer(address + 3, data.byte(3));
    }

    if(size == Long && address.bit(0) == 1) {
      wait();
      writer(address + 0, data.byte(0));
      wait();
      writer(address + 1, data.byte(1));
      writer(address + 2, data.byte(2));
      wait();
      writer(address + 3, data.byte(3));
    }
  }
}

/* IO: (internal I/O registers)
 */

auto CPU::IO::select(uint24 compare) const -> bool {
  return compare <= 0x0000ff;
}

/* ROM: (BIOS)
 */

auto CPU::ROM::select(uint24 compare) const -> bool {
  return compare >= 0xff0000;
}

/* CRAM: (CPU memory)
 */

auto CPU::CRAM::select(uint24 compare) const -> bool {
  return compare >= 0x004000 && compare <= 0x006fff;
}

/* ARAM: (APU memory)
 */

auto CPU::ARAM::select(uint24 compare) const -> bool {
  return compare >= 0x007000 && compare <= 0x007fff;
}

/* VRAM: (VPU memory)
 */

auto CPU::VRAM::select(uint24 compare) const -> bool {
  return compare >= 0x008000 && compare <= 0x00bfff;
}

/* CS0: (chip select 0)
 * Connected to cartridge flash chip 0.
 */

auto CPU::CS0::select(uint24 compare) const -> bool {
  if(!enable) return false;
  return !(uint24)((compare ^ address) & ~mask);
}

/* CS1: (chip select 1)
 * Connected to cartridge flash chip 1.
 */

auto CPU::CS1::select(uint24 compare) const -> bool {
  if(!enable) return false;
  return !(uint24)((compare ^ address) & ~mask);
}

/* CS2: (chip select 2)
 * Not connected and not used.
 */

auto CPU::CS2::select(uint24 compare) const -> bool {
  if(!enable) return false;
  //TMP95C061 range is 000080-ffffff
  //however, the Neo Geo Pocket maps I/O registers from 000000-0000ff
  //the exact range is unknown, so it is a guess that the range was expanded here
  if(!mode) return compare >= 0x000100;
  return !(uint24)((compare ^ address) & ~mask);
}

/* CS3: (chip select 3)
 * Not connected and not used.
 */

auto CPU::CS3::select(uint24 compare) const -> bool {
  if(!enable) return false;
  return !(uint24)((compare ^ address) & ~mask);
}

/* CSX: (chip select external)
 * Not connected and not used.
 */

auto CPU::width(uint24 address) -> uint {
  if(  io.select(address)) return   io.width;
  if( rom.select(address)) return  rom.width;
  if(cram.select(address)) return cram.width;
  if(aram.select(address)) return aram.width;
  if(vram.select(address)) return vram.width;
  if( cs0.select(address)) return  cs0.width;
  if( cs1.select(address)) return  cs1.width;
  if( cs2.select(address)) return  cs2.width;
  if( cs3.select(address)) return  cs3.width;
                           return  csx.width;
}

auto CPU::read(uint size, uint24 address) -> uint32 {
  mar = address;
  if(  io.select(address)) return   io.read(size, address);
  if( rom.select(address)) return  rom.read(size, address);
  if(cram.select(address)) return cram.read(size, address);
  if(aram.select(address)) return aram.read(size, address);
  if(vram.select(address)) return vram.read(size, address);
  if( cs0.select(address)) return  cs0.read(size, address);
  if( cs1.select(address)) return  cs1.read(size, address);
  if( cs2.select(address)) return  cs2.read(size, address);
  if( cs3.select(address)) return  cs3.read(size, address);
                           return  csx.read(size, address);
}

auto CPU::write(uint size, uint24 address, uint32 data) -> void {
  mar = address;
  mdr = data;
  if(  io.select(address)) return   io.write(size, address, data);
  if( rom.select(address)) return  rom.write(size, address, data);
  if(cram.select(address)) return cram.write(size, address, data);
  if(aram.select(address)) return aram.write(size, address, data);
  if(vram.select(address)) return vram.write(size, address, data);
  if( cs0.select(address)) return  cs0.write(size, address, data);
  if( cs1.select(address)) return  cs1.write(size, address, data);
  if( cs2.select(address)) return  cs2.write(size, address, data);
  if( cs3.select(address)) return  cs3.write(size, address, data);
                           return  csx.write(size, address, data);
}
