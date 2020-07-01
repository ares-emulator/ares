#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

DSP dsp;
#include "memory.cpp"
#include "gaussian.cpp"
#include "counter.cpp"
#include "envelope.cpp"
#include "brr.cpp"
#include "misc.cpp"
#include "voice.cpp"
#include "echo.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto DSP::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("DSP");

  stream = node->append<Node::Stream>("DSP");
  stream->setChannels(2);
  stream->setFrequency(system.apuFrequency() / 768.0);

  debugger.load(node);
}

auto DSP::unload() -> void {
  node = {};
  stream = {};
  debugger = {};
}

auto DSP::main() -> void {
  voice5(voice[0]);
  voice2(voice[1]);
  tick();

  voice6(voice[0]);
  voice3(voice[1]);
  tick();

  voice7(voice[0]);
  voice4(voice[1]);
  voice1(voice[3]);
  tick();

  voice8(voice[0]);
  voice5(voice[1]);
  voice2(voice[2]);
  tick();

  voice9(voice[0]);
  voice6(voice[1]);
  voice3(voice[2]);
  tick();

  voice7(voice[1]);
  voice4(voice[2]);
  voice1(voice[4]);
  tick();

  voice8(voice[1]);
  voice5(voice[2]);
  voice2(voice[3]);
  tick();

  voice9(voice[1]);
  voice6(voice[2]);
  voice3(voice[3]);
  tick();

  voice7(voice[2]);
  voice4(voice[3]);
  voice1(voice[5]);
  tick();

  voice8(voice[2]);
  voice5(voice[3]);
  voice2(voice[4]);
  tick();

  voice9(voice[2]);
  voice6(voice[3]);
  voice3(voice[4]);
  tick();

  voice7(voice[3]);
  voice4(voice[4]);
  voice1(voice[6]);
  tick();

  voice8(voice[3]);
  voice5(voice[4]);
  voice2(voice[5]);
  tick();

  voice9(voice[3]);
  voice6(voice[4]);
  voice3(voice[5]);
  tick();

  voice7(voice[4]);
  voice4(voice[5]);
  voice1(voice[7]);
  tick();

  voice8(voice[4]);
  voice5(voice[5]);
  voice2(voice[6]);
  tick();

  voice9(voice[4]);
  voice6(voice[5]);
  voice3(voice[6]);
  tick();

  voice1(voice[0]);
  voice7(voice[5]);
  voice4(voice[6]);
  tick();

  voice8(voice[5]);
  voice5(voice[6]);
  voice2(voice[7]);
  tick();

  voice9(voice[5]);
  voice6(voice[6]);
  voice3(voice[7]);
  tick();

  voice1(voice[1]);
  voice7(voice[6]);
  voice4(voice[7]);
  tick();

  voice8(voice[6]);
  voice5(voice[7]);
  voice2(voice[0]);
  tick();

  voice3a(voice[0]);
  voice9(voice[6]);
  voice6(voice[7]);
  echo22();
  tick();

  voice7(voice[7]);
  echo23();
  tick();

  voice8(voice[7]);
  echo24();
  tick();

  voice3b(voice[0]);
  voice9(voice[7]);
  echo25();
  tick();

  echo26();
  tick();

  misc27();
  echo27();
  tick();

  misc28();
  echo28();
  tick();

  misc29();
  echo29();
  tick();

  misc30();
  voice3c(voice[0]);
  echo30();
  tick();

  voice4(voice[0]);
  voice1(voice[2]);
  tick();
}

auto DSP::tick() -> void {
  Thread::step(3 * 8);
  Thread::synchronize(smp);
}

auto DSP::sample(int16 left, int16 right) -> void {
  stream->sample(left / 32768.0, right / 32768.0);
}

auto DSP::power(bool reset) -> void {
  Thread::create(system.apuFrequency(), {&DSP::main, this});

  if(!reset) {
    random.array(apuram, sizeof(apuram));
    random.array(registers, sizeof(registers));
  }

  master = {};
  echo = {};
  noise = {};
  brr = {};
  latch = {};
  for(uint n : range(8)) {
    voice[n] = {};
    voice[n].index = n << 4;
  }

  gaussianConstructTable();
}

}
