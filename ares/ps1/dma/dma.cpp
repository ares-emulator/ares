#include <ps1/ps1.hpp>

namespace ares::PlayStation {

DMA dma;
#include "io.cpp"
#include "transfer.cpp"
#include "serialization.cpp"

auto DMA::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("DMA");
}

auto DMA::unload() -> void {
  node.reset();
}

auto DMA::step(uint clocks) -> void {
}

auto DMA::pollIRQ() -> void {
  irq.flag = irq.force;
  if(!irq.enable) return;
  for(uint n : range(7)) {
    if(channel[n].irq.flag & channel[n].irq.enable) irq.flag = 1;
  }
}

auto DMA::power(bool reset) -> void {
  for(uint n : range(7)) {
    channel[n].priority = 1 + n;
    channel[n].masterEnable = 0;
  }
}

}
