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
  //The previous step may leave DMA ahead of CPU at a thread entry point.
  //Reach this timestamp before accepting another request or consuming a wait.
  Thread::synchronize(cpu);

  if(counter > 0) {
    u32 clocks = counter;
    counter = 0;
    return step(clocks);
  }

  if(arbitrate()) return;

  step(bus.arbiter.owner == Bus::Idle && !bus.arbiter.cpuHandoff ? 128 : 1);
}

auto DMA::arbitrate() -> bool {
  for(u32 id : channelsByPriority) {
    if(channels[id].step()) return true;
  }
  return false;
}

auto DMA::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto DMA::active() -> bool {
  for(u32 id : channelsByPriority) {
    if(channels[id].state == Running) return true;
  }
  return false;
}

auto DMA::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&DMA::main, this));

  counter = 0;
  cpuControl = 0;
  irq.force = 0;
  irq.enable = 0;
  irq.flag = 0;
  irq.unknown = 0;
  for(u32 n : range(7)) {
    channels[n].masterEnable = 0;
    channels[n].priority = 1 + n;
    channels[n].baseAddress = 0;
    channels[n].baseLength = 0;
    channels[n].address = 0;
    channels[n].length = 0;
    channels[n].blocks = 0;
    channels[n].direction = 0;
    channels[n].decrement = n == OTC;
    channels[n].synchronization = 0;
    channels[n].chopping.enable = 0;
    channels[n].chopping.dmaWindow = 0;
    channels[n].chopping.cpuWindow = 0;
    channels[n].chopping.remaining = 0;
    channels[n].enable = 0;
    channels[n].trigger = 0;
    channels[n].forced = 0;
    channels[n].unknown = 0;
    channels[n].irq.enable = 0;
    channels[n].irq.flag = 0;
    channels[n].chain.address = 0;
    channels[n].chain.length = 0;
    channels[n].chain.transferred = 0;
    channels[n].state = 0;
    channels[n].blockOffset = 0;
  }
  for(auto& v : channelsByPriority) v = 0;
  sortChannelsByPriority();
}

}
