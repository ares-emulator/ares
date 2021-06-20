#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

PSG psg;
#include "serialization.cpp"

auto PSG::writePitch(u32 pitch) -> void {
  apu.debugger.interrupt(string{"writePitch ", pitch & 15, " ", pitch >> 4 & 63});
}

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(2);
  stream->setFrequency(system.frequency() / 32.0);
  stream->addHighPassFilter(20.0, 1);
}

auto PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto PSG::main() -> void {
  f64 left  = 0.0;
  f64 right = 0.0;

  if(psg.enable) {
    auto channels = T6W28::clock();
    left  += volume[channels[0]];
    left  += volume[channels[1]];
    left  += volume[channels[2]];
    left  += volume[channels[3]];
    left  /= 4.0;
    right += volume[channels[4]];
    right += volume[channels[5]];
    right += volume[channels[6]];
    right += volume[channels[7]];
    right /= 4.0;
  } else {
    left  += dac.left  / 255.0;
    right += dac.right / 255.0;
  }

  stream->frame(left, right);
  step(1);
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
  synchronize(apu);
}

auto PSG::enablePSG() -> void {
  psg.enable = 1;
}

auto PSG::enableDAC() -> void {
  psg.enable = 0;
}

auto PSG::writeLeftDAC(n8 data) -> void {
  dac.left  = data;
}

auto PSG::writeRightDAC(n8 data) -> void {
  dac.right = data;
}

auto PSG::power() -> void {
  Thread::create(system.frequency() / 32.0, {&PSG::main, this});

  psg = {};
  dac = {};
  for(u32 level : range(15)) {
    volume[level] = pow(2, level * -2.0 / 6.0);
  }
  volume[15] = 0;
}

}
