#include <md/md.hpp>

namespace ares::MegaDrive {

OPN2 opn2;
#include "serialization.cpp"

auto OPN2::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("YM2612");

  stream = node->append<Node::Audio::Stream>("YM2612");
  stream->setChannels(2);
  stream->setFrequency(system.frequency() / 7.0 / 144.0);
  stream->addHighPassFilter(  20.0, 1);
  stream->addLowPassFilter (2840.0, 1);
}

auto OPN2::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto OPN2::main() -> void {
  step(144);
  auto samples = YM2612::clock();
  stream->frame(samples[0] / 32768.0, samples[1] / 32768.0);
}

auto OPN2::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto OPN2::power(bool reset) -> void {
  YM2612::power();
  Thread::create(system.frequency() / 7.0, {&OPN2::main, this});
}

}
