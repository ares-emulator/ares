#include <ps1/ps1.hpp>

namespace ares::PlayStation {

SPU spu;
#include "serialization.cpp"

auto SPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("SPU");

  stream = node->append<Node::Stream>("SPU");
  stream->setChannels(2);
  stream->setFrequency(44100.0);
}

auto SPU::unload() -> void {
  stream.reset();
  node.reset();
}

auto SPU::main() -> void {
  sample();
  step(33'868'800 / 44'100);
}

auto SPU::sample() -> void {
  stream->sample(0.0, 0.0);
}

auto SPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto SPU::power(bool reset) -> void {
  Thread::reset();
}

}
