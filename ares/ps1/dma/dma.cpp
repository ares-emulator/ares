#include <ps1/ps1.hpp>

namespace ares::PlayStation {

DMA dma;
#include "io.cpp"
#include "irq.cpp"
#include "channel.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto DMA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("DMA");
  debugger.load(node);
}

auto DMA::unload() -> void {
  debugger = {};
  node.reset();
}

auto DMA::main() -> void {
  for(u32 id : channelsByPriority) {
    if(channels[id].step(16)) break;
  }
  step(16);
}

auto DMA::step(u32 clocks) -> void {
  Thread::clock += clocks;
}

auto DMA::power(bool reset) -> void {
  Memory::Interface::setWaitStates(3, 3, 2);
  for(u32 n : range(7)) {
    channels[n].priority = 1 + n;
  }
  sortChannelsByPriority();
}

}
