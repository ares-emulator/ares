#include <ng/ng.hpp>

namespace ares::NeoGeo {

OPNB opnb;
#include "serialization.cpp"

auto OPNB::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("YM2610");

  streamFM = node->append<Node::Audio::Stream>("FM");
  streamFM->setChannels(2);
  streamFM->setFrequency(8'000'000 / 144.0);
  streamFM->addHighPassFilter(  20.0, 1);
  streamFM->addLowPassFilter (2840.0, 1);

  streamSSG = node->append<Node::Audio::Stream>("SSG");
  streamSSG->setChannels(1);
  streamSSG->setFrequency(8'000'000 / 144.0);
}

auto OPNB::unload() -> void {
  node->remove(streamFM);
  node->remove(streamSSG);
  streamFM.reset();
  streamSSG.reset();
  node.reset();
}

auto OPNB::main() -> void {
  step(144);

  auto samples = fm.clock();
  streamFM->frame(samples[0] / 32768.0, samples[1] / 32768.0);

  auto channels = ssg.clock();
  f64 output = 0.0;
  output += volume[channels[0]];
  output += volume[channels[1]];
  output += volume[channels[2]];
  streamSSG->frame(output / 3.0);

  apu.irq.pending = fm.readStatus();
}

auto OPNB::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto OPNB::power(bool reset) -> void {
  YM2610::power();
  Thread::create(8'000'000, {&OPNB::main, this});

  for(u32 level : range(16)) {
    volume[level] = 1.0 / pow(2, 1.0 / 2 * (15 - level));
  }
}

}
