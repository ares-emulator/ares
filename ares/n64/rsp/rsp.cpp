#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RSP rsp;
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
  stream->sample(0.0, 0.0);
  step(1);
}

auto RSP::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto RSP::power() -> void {
  Thread::create(48'000, {&RSP::main, this});
}

}
