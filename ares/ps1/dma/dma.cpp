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
  u8 active = enabledMask | runningMask;
  if(!active) {
    return step(counter > 0 ? (u32)counter : 128u);
  }

  blocked = false;
  for(u32 id : channelsByPriority) {
    if(!(active & (1 << id))) continue;
    if(channels[id].step()) {
      if(channels[id].synchronization == 0 && channels[id].chopping.enable) return;
      return step(busReleaseCycles);
    }
  }

  step(blocked ? 1u : counter > 0 ? (u32)counter : 128u);
}


auto DMA::canSkipWait() const -> bool {
  if(!blocked || runningMask) return false;
  if(cpu.busAccess) return true;
  if(enabledMask == (1 << 2)) return !channels[2].trigger && !gpu.canWriteDMA();
  if(!enabledMask) return false;
  for(u32 id : range(7)) {
    if((enabledMask & (1 << id)) && channels[id].dmaRequest()) return false;
  }
  return true;
}

auto DMA::skipBusWait(u64 time) -> void {
  skipBusWait(time, canSkipWait());
}

auto DMA::skipBusWait(u64 time, bool allowed) -> void {
  if(!allowed || clock() >= time) return;
  u64 delta = time - clock();
  u32 clocks = delta / scalar() + (delta % scalar() != 0);
  counter -= clocks;
  Thread::step(clocks);
}

auto DMA::step(u32 clocks) -> void {
  counter-= clocks;
  Thread::step(clocks);
  Thread::synchronize();
}

auto DMA::cpuReleaseWindow() const -> u32 {
  u32 clocks = busReleaseCycles;
  for(auto& channel : channels) {
    if(channel.state != Running) continue;
    if(channel.synchronization == 0 && channel.chopping.enable) {
      clocks = min<u32>(clocks, 1 << channel.chopping.cpuWindow);
    }
  }
  return clocks;
}

auto DMA::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&DMA::main, this));

  irq.force = 0;
  irq.enable = 0;
  irq.flag = 0;
  irq.unknown = 0;
  controlUnknown = 0;
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
  }
  for(auto& v : channelsByPriority) v = 0;
  counter = 0;
  blocked = false;
  runningMask = 0;
  enabledMask = 0;
  sortChannelsByPriority();
}

}
