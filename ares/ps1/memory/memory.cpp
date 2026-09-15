#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Bus bus;
MemoryControl memory;
MemoryExpansion expansion1;
MemoryExpansion expansion2;
MemoryExpansion expansion3;
Memory::Readable bios;
Memory::Unmapped unmapped;
#include "bus.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto MemoryControl::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Memory");
}

auto MemoryControl::unload() -> void {
  node.reset();
}

auto MemoryControl::power(bool reset) -> void {
  bus.power();
  memory.ram = {};
  memory.cache = {};
  memory.bios = {};
  memory.cdrom = {};
  memory.common = {};
  memory.exp1 = {};
  memory.exp2 = {};
  memory.exp3 = {};
  memory.spu = {};
}

auto MemoryControl::decodeRAM(u32 address) const -> RAMAccess {
  address &= 0x1fff'ffff;
  if(address >= 16_MiB) return {RAMAccess::NotRAM, 0};

  static constexpr u32 bankSize[8][2] = {
    {1_MiB, 0}, {4_MiB, 0}, {1_MiB, 1_MiB}, {4_MiB, 4_MiB},
    {2_MiB, 0}, {8_MiB, 0}, {2_MiB, 2_MiB}, {8_MiB, 8_MiB},
  };

  u32 mode = ram.window;
  u32 ras0 = bankSize[mode][0];
  u32 ras1 = bankSize[mode][1];
  if(address < ras0) {
    if(!ram.bankSize[0]) return {RAMAccess::HighZ, 0};
    return {RAMAccess::Mapped, address % ram.bankSize[0]};
  }
  if(address < ras0 + ras1) {
    if(!ram.bankSize[1]) return {RAMAccess::HighZ, 0};
    return {RAMAccess::Mapped, ram.bankSize[0] + (address - ras0) % ram.bankSize[1]};
  }
  return {RAMAccess::Unmapped, 0};
}

auto MemoryControl::MemPort::value() const -> u32 {
  n32 data = 0;
  data.bit( 0, 3) = writeDelay;
  data.bit( 4, 7) = readDelay;
  data.bit( 8) = recovery;
  data.bit( 9) = hold;
  data.bit(10) = floating;
  data.bit(11) = preStrobe;
  data.bit(12) = dataWidth;
  data.bit(13) = autoIncrement;
  data.bit(14,15) = unknown14_15;
  data.bit(16,20) = addrBits;
  data.bit(21,23) = reserved21_23;
  data.bit(24,27) = dmaTiming;
  data.bit(28) = addrError;
  data.bit(29) = dmaSelect;
  data.bit(30) = wideDMA;
  data.bit(31) = wait;
  return data;
}

auto MemoryControl::MemPort::write(u32 value) -> void {
  n32 data = value;
  if(!activation) activeValue = this->value();
  activation = 2;
  configured = true;

  writeDelay = data.bit(0,3);
  readDelay = data.bit(4,7);
  recovery = data.bit(8);
  hold = data.bit(9);
  floating = data.bit(10);
  preStrobe = data.bit(11);
  dataWidth = data.bit(12);
  autoIncrement = data.bit(13);
  unknown14_15 = data.bit(14,15);
  addrBits = data.bit(16,20);
  reserved21_23 = data.bit(21,23);
  dmaTiming = data.bit(24,27);
  addrError = data.bit(28);
  dmaSelect = data.bit(29);
  wideDMA = data.bit(30);
  wait = data.bit(31);
}

template<bool isWrite, bool isDMA>
auto MemoryControl::MemPort::calcAccessTime(u32 bytesCount) -> u32 const {
  n32 configuration = activation ? activeValue : value();
  u32 delay = isWrite ? configuration.bit(0,3) : configuration.bit(4,7);
  if constexpr(isDMA) {
    if(configuration.bit(29)) delay = configuration.bit(24,27);
  }

  u32 busBytes = (isDMA && configuration.bit(30)) ? 4 : (configuration.bit(12) ? 2 : 1);
  u32 accesses = (bytesCount > 0) ? ((bytesCount + busBytes - 1) / busBytes) : 1;

  s32 first = 0;
  s32 sequential = 0;
  s32 minimum = configuration.bit(11) ? (u32)memory.common.com3 : 0;

  if(configuration.bit(8)) {
    first += (s32)memory.common.com0 - 1;
    sequential += (s32)memory.common.com0 - 1;
  }
  if(configuration.bit(10)) {
    first += memory.common.com2;
    sequential += memory.common.com2;
  }
  if constexpr(isWrite) {
    if(configuration.bit(9)) {
      first += memory.common.com1;
      sequential += memory.common.com1;
    }
  }

  if(first < 6) first++;
  first += delay + 2;
  sequential += delay + 2;

  first = max(first, minimum + 6);
  sequential = max(sequential, minimum + 2);

  if(activation && --activation == 0) activeValue = value();
  return first + sequential * (accesses - 1);
}


//explicit instantiations to prevent linker errors
template auto MemoryControl::MemPort::calcAccessTime<true,  false>(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<false, false>(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<true,  true >(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<false, true >(u32) -> u32 const;

}
