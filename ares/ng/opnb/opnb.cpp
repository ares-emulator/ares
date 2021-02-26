#include <ng/ng.hpp>

namespace ares::NeoGeo {

OPNB opnb;
#include "serialization.cpp"

auto OPNB::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("YM2610");

  stream = node->append<Node::Audio::Stream>("YM2610");
  stream->setChannels(2);
  stream->setFrequency(44100.0);  //incorrect
}

auto OPNB::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto OPNB::main() -> void {
  stream->frame(0.0, 0.0);
  step(1);
}

auto OPNB::step(u32 clocks) -> void {
  Thread::step(clocks);
}

auto OPNB::power(bool reset) -> void {
  Thread::create(44'100, {&OPNB::main, this});
}

}
