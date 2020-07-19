#include <ps1/ps1.hpp>

namespace ares::PlayStation {

DMA dma;
#include "io.cpp"
#include "irq.cpp"
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

auto DMA::power(bool reset) -> void {
  for(uint n : range(7)) {
    channel[n].priority = 1 + n;
    channel[n].masterEnable = 0;
  }
}

}
