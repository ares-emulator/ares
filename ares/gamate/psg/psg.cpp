#include <gamate/gamate.hpp>

namespace ares::Gamate {

PSG psg;
#include "serialization.cpp"

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(2);
  stream->setFrequency(PsgClock);
}

auto PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto PSG::main() -> void {
  auto channels = AY38910::clock();

  f64 left  = volume[channels[0]] + volume[channels[2]] * 0.5;
  f64 right = volume[channels[1]] + volume[channels[2]] * 0.5;
  stream->frame(left / 1.5, right / 1.5);

  step(1);
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

//there is no separate address latch: the register is chosen by the low four
//bits of the address the CPU touches, then the data port is read or written
auto PSG::read(n4 address) -> n8 {
  AY38910::select(address);
  return AY38910::read();
}

auto PSG::write(n4 address, n8 data) -> void {
  AY38910::select(address);
  AY38910::write(data);
}

auto PSG::power() -> void {
  AY38910::power();
  Thread::create(PsgClock, std::bind_front(&PSG::main, this));

  //the AY's volume steps are 3dB apart
  for(u32 level : range(16)) {
    volume[level] = level ? 1.0 / pow(2, (15 - level) / 2.0) : 0.0;
  }
}

}
