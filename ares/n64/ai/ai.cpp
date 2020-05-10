#include <n64/n64.hpp>

namespace ares::Nintendo64 {

AI ai;
#include "io.cpp"
#include "serialization.cpp"

auto AI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("AI");

  stream = node->append<Node::Stream>("AI");
  stream->setChannels(2);
  stream->setFrequency(44100.0);
}

auto AI::unload() -> void {
  node = {};
  stream = {};
}

auto AI::main() -> void {
  sample();
  step(dac.period);
}

auto AI::sample() -> void {
  if(io.fifoPending == 0) return stream->sample(0.0, 0.0);

  auto& fifo = this->fifo[io.fifoPlaying];
  auto data  = fifo.buffer[fifo.offset++];
  auto left  = i16(data >> 16);
  auto right = i16(data >>  0);
  stream->sample(left / 32768.0, right / 32768.0);

  fifo.length -= 4;
  if(fifo.length == 0) {
    io.fifoPending--;
    io.fifoPlaying ^= 1;
    mi.raise(MI::IRQ::AI);
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
