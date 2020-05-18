#include <n64/n64.hpp>

namespace ares::Nintendo64 {

AI ai;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto AI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("AI");

  stream = node->append<Node::Stream>("AI");
  stream->setChannels(2);
  stream->setFrequency(44100.0);

  debugger.load(node);
}

auto AI::unload() -> void {
  node = {};
  stream = {};
  debugger = {};
}

auto AI::main() -> void {
  sample();
  step(dac.period);
}

auto AI::sample() -> void {
  if(io.dmaCount == 0) return stream->sample(0.0, 0.0);

  auto data  = rdram.ram.readWord(io.dmaAddress[0]);
  auto left  = i16(data >> 16);
  auto right = i16(data >>  0);
  stream->sample(left / 32768.0, right / 32768.0);

  io.dmaAddress[0] += 4;
  io.dmaLength [0] -= 4;
  if(!io.dmaLength[0]) {
    mi.raise(MI::IRQ::AI);
    if(--io.dmaCount) {
      io.dmaAddress[0] = io.dmaAddress[1];
      io.dmaLength [0] = io.dmaLength [1];
    }
  }
}

auto AI::step(uint clocks) -> void {
  clock += clocks;
}

auto AI::power() -> void {
  Thread::reset();
  io = {};
  dac = {};
  stream->setFrequency(dac.frequency);
}

}
