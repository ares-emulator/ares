#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

APU apu;
#include "io.cpp"
#include "square.cpp"
#include "square1.cpp"
#include "square2.cpp"
#include "wave.cpp"
#include "noise.cpp"
#include "sequencer.cpp"
#include "fifo.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("APU");

  stream = node->append<Node::Stream>("PSG");
  stream->setChannels(2);
  stream->setFrequency(system.frequency() / 64.0);
  stream->addHighPassFilter(20.0, 1);
}

auto APU::unload() -> void {
  node = {};
  stream = {};
}

auto APU::main() -> void {
  //GBA clock runs at 16777216hz
  //GBA PSG channels run at 2097152hz
  sequence();
  step(8);

  //audio PWM output frequency and bit-rate are dependent upon amplitude setting:
  //0 = 9-bit @  32768hz
  //1 = 8-bit @  65536hz
  //2 = 7-bit @ 131072hz
  //3 = 6-bit @ 262144hz

  //dynamically changing the output frequency under emulation would be difficult;
  //always run the output frequency at the maximum 262144hz
  if(++clock & 7) return;

  int lsample = bias.level - 0x0200;
  int rsample = bias.level - 0x0200;

  //amplitude: 0 = 32768hz; 1 = 65536hz; 2 = 131072hz; 3 = 262144hz
  if((clock & (63 >> (3 - bias.amplitude))) == 0) {
    sequencer.sample();
    fifo[0].sample();
    fifo[1].sample();
  }

  lsample += sequencer.loutput;
  rsample += sequencer.routput;

  int fifo0 = fifo[0].output << fifo[0].volume;
  int fifo1 = fifo[1].output << fifo[1].volume;

  if(fifo[0].lenable) lsample += fifo0;
  if(fifo[1].lenable) lsample += fifo1;

  if(fifo[0].renable) rsample += fifo0;
  if(fifo[1].renable) rsample += fifo1;

  lsample = sclamp<11>(lsample);
  rsample = sclamp<11>(rsample);

  //clip 11-bit signed output to more limited output bit-rate
  //note: leaving 2-bits more on output to prevent quantization noise
  if(bias.amplitude == 0) lsample &= ~0, rsample &= ~0;  //9-bit
  if(bias.amplitude == 1) lsample &= ~1, rsample &= ~1;  //8-bit
  if(bias.amplitude == 2) lsample &= ~3, rsample &= ~3;  //7-bit
  if(bias.amplitude == 3) lsample &= ~7, rsample &= ~7;  //6-bit

  if(cpu.stopped()) lsample = 0, rsample = 0;
  stream->sample((lsample << 5) / 32768.0, (rsample << 5) / 32768.0);
}

auto APU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto APU::power() -> void {
  Thread::create(system.frequency(), {&APU::main, this});

  clock = 0;
  bias = {};
  square1.power();
  square2.power();
  wave.power();
  noise.power();
  sequencer.power();
  fifo[0].power();
  fifo[1].power();

  for(uint n = 0x060; n <= 0x0a7; n++) bus.io[n] = this;
}

}
