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
  bus.readLatch = 0;
  memory.ram = {};
  memory.cache = {};
  memory.exp1Base.value = 0x1f00'0000;
  memory.exp2Base.value = 0x1f80'2000;
  memory.bios = {};
  memory.cdrom = {};
  memory.common = {};
  memory.exp1 = {};
  memory.exp2 = {};
  memory.exp3 = {};
  memory.spu = {};
  updateConfiguration();
}

auto MemoryControl::updateConfiguration() -> void {
  exp1Mask = (1u << min<u32>(exp1.addrBits, 23)) - 1;
  exp1Address = (0x1f00'0000 | exp1Base.value.bit(0,23)) & ~exp1Mask;
  exp2Mask = (1u << min<u32>(exp2.addrBits, 13)) - 1;
  exp2Enabled = (0x1f00'0000 | exp2Base.value.bit(0,23)) == 0x1f80'2000;
  for(auto* port : {&exp1, &exp2, &exp3, &bios, &spu, &cdrom}) port->updateTiming();
}

auto MemoryControl::MemPort::updateTiming() -> void {
  for(u32 operation : range(4)) {
    bool isWrite = operation & 1;
    bool isDMA = operation & 2;
    u32 delay = isWrite ? writeDelay : readDelay;
    if(isDMA && dmaSelect) delay = dmaTiming;
    s32 first = 0;
    s32 sequential = 0;
    s32 minimum = 0;

    if(recovery) {
      first += (s32)memory.common.com0 - 1;
      sequential += (s32)memory.common.com0 - 1;
    }
    if(isWrite) {
      if(hold) {
        first += memory.common.com1;
        sequential += memory.common.com1;
      }
    } else {
      if(floating) {
        first += memory.common.com2;
        sequential += memory.common.com2;
      }
    }
    if(preStrobe) minimum = memory.common.com3;

    first += delay + 2;
    sequential += delay + 2;

    first = max(first, minimum + 6);
    sequential = max(sequential, minimum + 2);

    timing[operation] = {(u32)first, (u32)sequential, dataWidth ? 1u : 0u};
  }
}

template<bool isWrite, bool isDMA>
auto MemoryControl::MemPort::calcAccessTime(u32 bytesCount) -> u32 const {
  auto& value = timing[(isDMA ? 2 : 0) | (isWrite ? 1 : 0)];
  u32 accesses = bytesCount ? (bytesCount + (1u << value.widthShift) - 1) >> value.widthShift : 1;
  return value.first + value.sequential * (accesses - 1);
}

//explicit instantiations to prevent linker errors
template auto MemoryControl::MemPort::calcAccessTime<true,  false>(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<false, false>(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<true,  true >(u32) -> u32 const;
template auto MemoryControl::MemPort::calcAccessTime<false, true >(u32) -> u32 const;

}
