#include <krokha/krokha.hpp>

namespace ares::Krokha {

PSG psg;
#include "serialization.cpp"

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(1);
  stream->setFrequency(system.frequency() / 8);
}

auto PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto PSG::main() -> void {
  stream->frame(level ? +0.5 : -0.5);
  step(1);
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PSG::write(n8 data) -> void {
  level = data.bit(1);
}

auto PSG::power() -> void {
  Thread::create(system.frequency() / 8, std::bind_front(&PSG::main, this));

  level = 0;
}

}
