/* Abstract memory bus implementation.
 * The TLCS900/H can perform 8-bit (Byte), 16-bit (Word), and 32-bit (Long) reads.
 * The actual internal bus of the CPU is always 16-bit.
 * Each memory object on the bus may allow either 8-bit or 16-bit accesses.
 * For CS0-CS3 and CSX, the bus width may be changed dynamically.
 * Further, CS0-3 and CSX allow optional wait states to be added to accesses.
 * Finally, CS0-3 and CSX have registers to control where they appear on the bus.
 * CS2 has an additional fixed addressing mode setting.
 */

/* Default memory layout: (hardware remappable, but not usually done)
 * 0x000000 ... 0x0000ff: internal I/O
 * 0x000100 ... 0x003fff: reserved
 * 0x004000 ... 0x006fff: 12KB CPU RAM
 * 0x007000 ... 0x007fff:  4KB APU RAM
 * 0x008000 ... 0x0087ff: KxGE I/O registers
 * 0x008800 ... 0x008fff: object VRAM
 * 0x009000 ... 0x009fff: scroll VRAM
 * 0x00a000 ... 0x00bfff: character RAM
 * 0x00c000 ... 0x01ffff: reserved
 * 0x020000 ... 0x03ffff: program flash ROM (CS0)
 * 0x040000 ... 0x07ffff: reserved
 * 0x080000 ... 0x09ffff: program flash ROM (CS1)
 * 0x0a0000 ... 0xfeffff: reserved
 * 0xff0000 ... 0xffffff: BIOS ROM
 */

auto CPU::Bus::wait() -> void {
  if(unlikely(debugging)) return;

  if(width == Byte) {
    switch(timing) {
    case 0: return cpu.step(2 + 2);  //1 state
    case 1: return cpu.step(2 + 4);  //2 states
    case 2: return cpu.step(2 + 2);  //1 state + (not emulated) /WAIT
    case 3: return cpu.step(2 + 0);  //0 states
    }
  }

  if(width == Word) {
    switch(timing) {
    case 0: return cpu.step(2 + 2);  //1 state
    case 1: return cpu.step(2 + 4);  //2 states
    case 2: return cpu.step(2 + 2);  //1 state + (not emulated) /WAIT
    case 3: return cpu.step(2 + 0);  //0 states
    }
  }
}

auto CPU::Bus::speed(u32 size, n24 address) -> n32 {
  if(width == Byte) {
    static constexpr u32 waits[4] = {2 + 2, 2 + 4, 2 + 2, 2 + 0};
    if(size == Byte) return waits[timing] * 1;
    if(size == Word) return waits[timing] * 2;
    if(size == Long) return waits[timing] * 4;
  }

  if(width == Word) {
    static constexpr u32 waits[4] = {2 + 2, 2 + 4, 2 + 2, 2 + 0};
    if(size == Byte) return waits[timing] * 1;
    if(size == Word && address.bit(0) == 0) return waits[timing] * 1;
    if(size == Word && address.bit(0) == 1) return waits[timing] * 2;
    if(size == Long && address.bit(0) == 0) return waits[timing] * 2;
    if(size == Long && address.bit(0) == 1) return waits[timing] * 3;
  }

  unreachable;
}

auto CPU::Bus::read(u32 size, n24 address) -> n32 {
  n32 data;

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

auto CPU::Bus::write(u32 size, n24 address, n32 data) -> void {
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

auto CPU::IO::select(n24 compare) const -> bool {
  return compare <= 0x0000ff;
}

/* ROM: (BIOS)
 */

auto CPU::ROM::select(n24 compare) const -> bool {
  return compare >= 0xff0000;
}

/* CRAM: (CPU memory)
 */

auto CPU::CRAM::select(n24 compare) const -> bool {
  return compare >= 0x004000 && compare <= 0x006fff;
}

/* ARAM: (APU memory)
 */

auto CPU::ARAM::select(n24 compare) const -> bool {
  return compare >= 0x007000 && compare <= 0x007fff;
}

/* VRAM: (VPU memory)
 */

auto CPU::VRAM::select(n24 compare) const -> bool {
  return compare >= 0x008000 && compare <= 0x00bfff;
}

/* CS0: (chip select 0)
 * Connected to cartridge flash chip 0.
 */

auto CPU::CS0::select(n24 compare) const -> bool {
  if(!enable) return false;
  return !(n24)((compare ^ address) & ~mask);
}

/* CS1: (chip select 1)
 * Connected to cartridge flash chip 1.
 */

auto CPU::CS1::select(n24 compare) const -> bool {
  if(!enable) return false;
  return !(n24)((compare ^ address) & ~mask);
}

/* CS2: (chip select 2)
 * Not connected and not used.
 */

auto CPU::CS2::select(n24 compare) const -> bool {
  if(!enable) return false;
  //TMP95C061 range is 000080-ffffff
  //however, the Neo Geo Pocket maps I/O registers from 000000-0000ff
  //the exact range is unknown, so it is a guess that the range was expanded here
  if(!mode) return compare >= 0x000100;
  return !(n24)((compare ^ address) & ~mask);
}

/* CS3: (chip select 3)
 * Not connected and not used.
 */

auto CPU::CS3::select(n24 compare) const -> bool {
  if(!enable) return false;
  return !(n24)((compare ^ address) & ~mask);
}

/* CSX: (chip select external)
 * Not connected and not used.
 */

auto CPU::width(n24 address) -> u32 {
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

auto CPU::speed(u32 size, n24 address) -> n32 {
  if(  io.select(address)) return   io.speed(size, address);
  if( rom.select(address)) return  rom.speed(size, address);
  if(cram.select(address)) return cram.speed(size, address);
  if(aram.select(address)) return aram.speed(size, address);
  if(vram.select(address)) return vram.speed(size, address);
  if( cs0.select(address)) return  cs0.speed(size, address);
  if( cs1.select(address)) return  cs1.speed(size, address);
  if( cs2.select(address)) return  cs2.speed(size, address);
  if( cs3.select(address)) return  cs3.speed(size, address);
                           return  csx.speed(size, address);
}

auto CPU::read(u32 size, n24 address) -> n32 {
  MAR = address;
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

auto CPU::write(u32 size, n24 address, n32 data) -> void {
  MAR = address;
  MDR = data;
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

auto CPU::disassembleRead(n24 address) -> n8 {
  maybe<Bus&> bus;
  if(0);
  else if(  io.select(address)) bus = io;
  else if( rom.select(address)) bus = rom;
  else if(cram.select(address)) bus = cram;
  else if(aram.select(address)) bus = aram;
  else if(vram.select(address)) bus = vram;
  else if( cs0.select(address)) bus = cs0;
  else if( cs1.select(address)) bus = cs1;
  else if( cs2.select(address)) bus = cs2;
  else if( cs3.select(address)) bus = cs3;
  else                          bus = csx;

  //prevent bus read from consuming CPU time
  bus->debugging = 1;
  auto data = bus->read(Byte, address);
  bus->debugging = 0;
  return data;
}
