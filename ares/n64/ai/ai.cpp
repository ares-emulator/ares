#include <n64/n64.hpp>

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
  bool active = false;

  if(io.dmaCount && io.dmaLength[0] && io.dmaEnable) {
    io.dmaAddress[0].bit(13,23) += io.dmaAddressCarry;
    auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");
    dac.left  = (s16)(data >> 16) / 32768.0;
    dac.right = (s16)(data >>  0) / 32768.0;

    io.dmaAddress[0].bit(0,12) += 4;
    io.dmaAddressCarry = io.dmaAddress[0].bit(0,12) == 0;
    io.dmaLength[0] -= 4;
    active = true;
  }

  if(io.dmaCount && io.dmaLength[0] == 0) {
    if(--io.dmaCount) {
      io.dmaAddress[0]  = io.dmaAddress[1];
      io.dmaLength[0]   = io.dmaLength[1];
      io.dmaOriginPc[0] = io.dmaOriginPc[1];
      mi.raise(MI::IRQ::AI);
    }
  }

  if(!active) {
    dac.left  *= dac.decayFactor;
    dac.right *= dac.decayFactor;
    if(fabs(dac.left)  < 1e-7) dac.left  = 0.0;
    if(fabs(dac.right) < 1e-7) dac.right = 0.0;
  }
}

auto AI::updateDecay() -> void {
  dac.decayFactor = exp(-1.0 / (dac.frequency * 0.003));
}

auto AI::power(bool reset) -> void {
  Thread::reset();
  io = {};
  dac.left  = 0.0;
  dac.right = 0.0;
  dac.frequency = 44100;
  dac.precision = 16;
  dac.period = system.frequency() / dac.frequency;
  updateDecay();
}

}
