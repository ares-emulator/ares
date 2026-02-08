#include <n64/n64.hpp>
#include <cmath>

namespace ares::Nintendo64 {

AI ai;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto AI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("AI");

  stream = node->append<Node::Audio::Stream>("AI");
  stream->setChannels(2);
  stream->setFrequency(44100.0);

  debugger.load(node);
}

auto AI::unload() -> void {
  debugger = {};
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto AI::main() -> void {
  while(Thread::clock < 0) {
    sample();
    stream->frame(dac.left, dac.right);
    step(dac.period);
  }
}

auto AI::sample() -> void {
  if(io.dmaCount && io.dmaLength[0] && io.dmaEnable) {
    io.dmaAddress[0].bit(13,23) += io.dmaAddressCarry;
    auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");
    auto l = s16(data >> 16);
    auto r = s16(data >> 0);
    dac.left = l / 32768.0;
    dac.right = r / 32768.0;

    io.dmaAddress[0].bit(0,12) += 4;
    io.dmaAddressCarry = io.dmaAddress[0].bit(0,12) == 0;
    io.dmaLength[0] -= 4;
  } else {
    dac.left *= dac.decayFactor;
    dac.right *= dac.decayFactor;
    if(fabs(dac.left) < 1e-7) dac.left = 0.0;
    if(fabs(dac.right) < 1e-7) dac.right = 0.0;
  }

  if(io.dmaCount && !io.dmaLength[0]) {
    mi.raise(MI::IRQ::AI);
    if(--io.dmaCount) {
      io.dmaAddress[0] = io.dmaAddress[1];
      io.dmaLength[0] = io.dmaLength[1];
      io.dmaOriginPc[0] = io.dmaOriginPc[1];
    }
  }
}

auto AI::updateDecay() -> void {
  f64 time = 0.003;
  dac.decayFactor = exp(-1.0 / (dac.frequency * time));
}

auto AI::power(bool reset) -> void {
  Thread::reset();

  fifo[0] = {};
  fifo[1] = {};
  io = {};
  dac.frequency = 44100;
  dac.precision = 16;
  dac.period = system.frequency() / dac.frequency;
  dac.left = 0.0;
  dac.right = 0.0;
  updateDecay();
}

}