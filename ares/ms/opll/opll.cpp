#include <ms/ms.hpp>

namespace ares::MasterSystem {

OPLL opll;
#include "serialization.cpp"

auto OPLL::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("OPLL");

  stream = node->append<Node::Stream>("YM2413");
  stream->setChannels(1);
  stream->setFrequency(system.colorburst() / 72.0);
  stream->addHighPassFilter(20.0, 1);
}

auto OPLL::unload() -> void {
  node = {};
  stream = {};
}

auto OPLL::main() -> void {
  auto output = YM2413::clock();
  stream->sample(output);
  step(1);
}

auto OPLL::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto OPLL::power() -> void {
  YM2413::power();
  Thread::create(system.colorburst() / 72.0, {&OPLL::main, this});
}

}
