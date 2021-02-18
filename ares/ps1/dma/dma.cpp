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

  irq.force = 0;
  irq.enable = 0;
  irq.flag = 0;
  irq.unknown = 0;
  for(u32 n : range(7)) {
    channels[n].masterEnable = 0;
    channels[n].priority = 1 + n;
    channels[n].address = 0;
    channels[n].length = 0;
    channels[n].blocks = 0;
    channels[n].direction = 0;
    channels[n].decrement = 0;
    channels[n].synchronization = 0;
    channels[n].chopping.enable = 0;
    channels[n].chopping.dmaWindow = 0;
    channels[n].chopping.cpuWindow = 0;
    channels[n].enable = 0;
    channels[n].trigger = 0;
    channels[n].unknown = 0;
    channels[n].irq.enable = 0;
    channels[n].irq.flag = 0;
    channels[n].chain.address = 0;
    channels[n].chain.length = 0;
    channels[n].state = 0;
    channels[n].counter = 0;
  }
  for(auto& v : channelsByPriority) v = 0;
  sortChannelsByPriority();
}

}
