namespace Board {

#include "sara-ram.cpp"
#include "linear.cpp"
#include "activision.cpp"
#include "atari8k.cpp"
#include "atari16k.cpp"
#include "atari32k.cpp"
#include "commavid.cpp"
#include "parker-bros.cpp"
#include "parker-bros-03e0.cpp"
#include "jvp.cpp"
#include "tigervision.cpp"
#include "ua.cpp"
#include "cbs-ram-plus.cpp"
#include "m-network.cpp"
#include "amiga-fc.cpp"
#include "wickstead.cpp"
#include "jane.cpp"
#include "dpc.cpp"
#include "mega-boy.cpp"
#include "atari-32-in-1.cpp"

//Homebrew
#include "dpc-plus.cpp"
#include "cdf.cpp"
#include "bus.cpp"
#include "chetiry.cpp"
#include "fa2.cpp"
#include "elf.cpp"
#include "movie-cart.cpp"
#include "cpuwiz-4ksc.cpp"
#include "three-e.cpp"
#include "three-ex.cpp"
#include "three-e-plus.cpp"
#include "enhanced-3f.cpp"
#include "four-a50.cpp"
#include "ef.cpp"
#include "mdm.cpp"
#include "x07.cpp"
#include "econo-banking.cpp"
#include "superbanking.cpp"

auto Interface::load(Memory::Readable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size());
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size());
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->write(name)) {
    memory.save(fp);
    return true;
  }
  return false;
}

auto Interface::readARMMemory(Memory::Readable<n8>& rom, Memory::Writable<n8>& ram,
  u32 mode, n32 address, u32 romBase, u32 ramLimit, n32& data) -> Harmony::Access {
  u32 size = mode & ARM7TDMI::Byte ? 1 : mode & ARM7TDMI::Half ? 2 : 4;
  u32 location = address;
  auto read = [&](auto& memory, u32 offset) -> Harmony::Access {
    if(offset > memory.size() || size > memory.size() - offset) return Harmony::Access::Fault;
    data = 0;
    for(u32 byte : range(size)) data |= memory.read(offset + byte) << byte * 8;
    return Harmony::Access::Granted;
  };

  if(location >= romBase && location < rom.size()) return read(rom, location);
  if(location >= 0x40000000 && location - 0x40000000 < ramLimit) {
    return read(ram, location - 0x40000000);
  }
  return Harmony::Access::Unmapped;
}

auto Interface::writeARMMemory(Memory::Writable<n8>& ram, u32 mode, n32 address,
  u32 ramLimit, n32 data) -> Harmony::Access {
  u32 size = mode & ARM7TDMI::Byte ? 1 : mode & ARM7TDMI::Half ? 2 : 4;
  u32 location = address;
  if(location < 0x40000000 || location - 0x40000000 >= ramLimit) return Harmony::Access::Unmapped;
  auto offset = location - 0x40000000;
  if(offset > ram.size() || size > ram.size() - offset || size > ramLimit - offset) {
    return Harmony::Access::Fault;
  }
  for(u32 byte : range(size)) ram.write(offset + byte, data >> byte * 8);
  return Harmony::Access::Granted;
}

}
