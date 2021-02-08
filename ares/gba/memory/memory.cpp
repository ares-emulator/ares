#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

Bus bus;

auto IO::readIO(u32 mode, n32 address) -> n32 {
  n32 word;

  if(mode & Word) {
    address &= ~3;
    word.byte(0) = readIO(address + 0);
    word.byte(1) = readIO(address + 1);
    word.byte(2) = readIO(address + 2);
    word.byte(3) = readIO(address + 3);
  } else if(mode & Half) {
    address &= ~1;
    word.byte(0) = readIO(address + 0);
    word.byte(1) = readIO(address + 1);
  } else if(mode & Byte) {
    word.byte(0) = readIO(address + 0);
  }

  return word;
}

auto IO::writeIO(u32 mode, n32 address, n32 word) -> void {
  if(mode & Word) {
    address &= ~3;
    writeIO(address + 0, word.byte(0));
    writeIO(address + 1, word.byte(1));
    writeIO(address + 2, word.byte(2));
    writeIO(address + 3, word.byte(3));
  } else if(mode & Half) {
    address &= ~1;
    writeIO(address + 0, word.byte(0));
    writeIO(address + 1, word.byte(1));
  } else if(mode & Byte) {
    writeIO(address + 0, word.byte(0));
  }
}

struct UnmappedIO : IO {
  auto readIO(n32 address) -> n8 override {
    return cpu.pipeline.fetch.instruction.byte(address & 1);
  }

  auto writeIO(n32 address, n8 byte) -> void override {
  }
};

static UnmappedIO unmappedIO;

auto Bus::mirror(n32 address, n32 size) -> n32 {
  n32 base = 0;
  if(size) {
    n32 mask = 1 << 27;  //28-bit bus
    while(address >= size) {
      while(!(address & mask)) mask >>= 1;
      address -= mask;
      if(size > mask) {
        size -= mask;
        base += mask;
      }
      mask >>= 1;
    }
    base += address;
  }
  return base;
}

auto Bus::power() -> void {
  for(u32 n : range(0x400)) io[n] = &unmappedIO;
}

}
