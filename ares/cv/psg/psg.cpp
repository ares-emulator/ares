#include <cv/cv.hpp>

namespace ares::ColecoVision {

PSG psg;
#include "serialization.cpp"

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(1);
  stream->setFrequency(system.colorburst() / 16.0);
  stream->addHighPassFilter(20.0, 1);
}

auto PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto PSG::main() -> void {
  auto channels = SN76489::clock();
  double output = 0.0;
  output += volume[channels[0]];
  output += volume[channels[1]];
  output += volume[channels[2]];
  output += volume[channels[3]];
  stream->frame(output / 4.0);
  step(1);
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PSG::power() -> void {
  SN76489::power();
  Thread::create(system.colorburst() / 16.0, {&PSG::main, this});

  for(u32 level : range(15)) {
    volume[level] = pow(2, level * -2.0 / 6.0);
  }
  volume[15] = 0;
}

}
