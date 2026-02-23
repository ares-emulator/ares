#include <ps1/ps1.hpp>

namespace ares::PlayStation {

Bus bus;
MemoryControl memory;
MemoryExpansion expansion1;
MemoryExpansion expansion2;
MemoryExpansion expansion3;
Memory::Readable bios;
Memory::Unmapped unmapped;
#include "io.cpp"
#include "serialization.cpp"

auto MemoryControl::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Memory");
}

auto MemoryControl::unload() -> void {
  node.reset();
}

auto MemoryControl::power(bool reset) -> void {
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

template<bool isWrite, bool isDMA>
auto MemoryControl::MemPort::calcAccessTime(u32 bytesCount) -> u32 const {
  u32 delay = isWrite ? writeDelay : readDelay;
  if constexpr(isDMA) {
    if(dmaSelect) delay = dmaTiming;
  }

  u32 busBytes = (isDMA && wideDMA) ? 4 : (dataWidth ? 2 : 1);
  u32 accesses = (bytesCount > 0) ? ((bytesCount + busBytes - 1) / busBytes) : 1;

  u32 first = delay + 2;
  if(preStrobe) first += memory.common.com3;

  if constexpr(isWrite) {
    if(hold) first += memory.common.com1;
  } else {
    if(floating) first += memory.common.com2;
  }

  if(recovery) first += memory.common.com0;

  u32 seq = delay + 2;
  if constexpr(isWrite) {
    if(hold) seq += memory.common.com1;
  } else {
    if(floating) seq += memory.common.com2;
  }
  if(recovery) seq += memory.common.com0;

  return first + seq * (accesses - 1);
}


//explicit instantiations to prevent linker errors
template auto MemoryControl::MemPort::calcAccessTime<true,  false>(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<false, false>(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<true,  true >(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<false, true >(u32) -> u32 const;

}
