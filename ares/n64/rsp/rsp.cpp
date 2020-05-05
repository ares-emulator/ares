#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RSP rsp;
#include "core/core.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto RSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("RSP");

  stream = node->append<Node::Stream>("RSP");
  stream->setChannels(2);
  stream->setFrequency(48000.0);

  dmem.allocate(4_KiB);
  imem.allocate(4_KiB);
}

auto RSP::unload() -> void {
  dmem.reset();
  imem.reset();
  node = {};
  stream = {};
}

auto RSP::main() -> void {
  if(io.halt) return step(93'750'000 / 48'000);
  instruction();
  step(5);
}

auto RSP::step(uint clocks) -> void {
  clock += clocks;

  static u32 counter = 0;
  counter += clocks;
  while(counter >= 93'750'000 / 48'000) {
    counter -= 93'750'000 / 48'000;
    stream->sample(0.0, 0.0);
  }
}

auto RSP::power() -> void {
  Thread::reset();
  powerCore();
  io.halt = true;
  io.interruptOnBreak = false;
}

}
