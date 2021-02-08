namespace Memory {
  #include "lsb-readable.hpp"
  #include "lsb-writable.hpp"
  #include "io.hpp"
}

struct Bus {
  //bus.hpp
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto readDual(u32 address) -> u64;
  auto writeByte(u32 address, u8  data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto writeDual(u32 address, u64 data) -> void;
};

extern Bus bus;
