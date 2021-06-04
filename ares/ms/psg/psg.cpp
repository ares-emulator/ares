#include <ms/ms.hpp>

namespace ares::MasterSystem {

PSG psg;
#include "serialization.cpp"

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(Device::MasterSystem() ? 1 : 2);
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

  if(Device::MasterSystem()) {
    f64 output = 0.0;
    output += volume[channels[0]];
    output += volume[channels[1]];
    output += volume[channels[2]];
    output += volume[channels[3]];
    if(io.mute) output = 0.0;

    stream->frame(output / 4.0);
  }

  if(Device::GameGear()) {
    f64 left = 0.0;
    if(io.enable.bit(4)) left  += volume[channels[0]];
    if(io.enable.bit(5)) left  += volume[channels[1]];
    if(io.enable.bit(6)) left  += volume[channels[2]];
    if(io.enable.bit(7)) left  += volume[channels[3]];

    f64 right = 0.0;
    if(io.enable.bit(0)) right += volume[channels[0]];
    if(io.enable.bit(1)) right += volume[channels[1]];
    if(io.enable.bit(2)) right += volume[channels[2]];
    if(io.enable.bit(3)) right += volume[channels[3]];

    stream->frame(left / 4.0, right / 4.0);
  }

  step(1);
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PSG::balance(n8 data) -> void {
  if(Device::GameGear()) {
    io.enable = data;
  }
}

auto PSG::power() -> void {
  SN76489::power();
  Thread::create(system.colorburst() / 16.0, {&PSG::main, this});

  io = {};
  for(u32 level : range(15)) {
    volume[level] = pow(2, level * -2.0 / 6.0);
  }
  volume[15] = 0;
}

}
