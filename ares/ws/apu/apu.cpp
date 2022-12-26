#include <ws/ws.hpp>

namespace ares::WonderSwan {

APU apu;
#include "io.cpp"
#include "dma.cpp"
#include "channel1.cpp"
#include "channel2.cpp"
#include "channel3.cpp"
#include "channel4.cpp"
#include "channel5.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("APU");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(2);
  stream->setFrequency(24'000);
  stream->addHighPassFilter(15.915, 1);
}

auto APU::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto APU::main() -> void {
  if(++state.dmaClock == 0) dma.run();
  channel1.run();
  channel2.run();
  channel3.run();
  channel4.run();
  if(++state.dacClock == 0) dacRun();
  if(++state.sweepClock == 0) channel3.sweep();
  step(1);
}

auto APU::sample(u32 channel, n5 index) -> n4 {
  n8 data = iram.read((io.waveBase << 6) + (--channel << 4) + (index >> 1));
  if(index.bit(0) == 0) return data.bit(0,3);
  if(index.bit(0) == 1) return data.bit(4,7);
  unreachable;
}

auto APU::dacRun() -> void {
  bool outputEnable = io.headphonesConnected ? io.headphonesEnable : io.speakerEnable;

  if(channel1.io.enable) channel1.runOutput();
  if(channel2.io.enable) channel2.runOutput();
  if(channel3.io.enable) channel3.runOutput();
  if(channel4.io.enable) channel4.runOutput();
  if(channel5.io.enable) channel5.runOutput();

  if(!outputEnable) {
    stream->frame(0, 0);
    return;
  }

  s32 left = 0;
  if(channel1.io.enable) left += channel1.output.left;
  if(channel2.io.enable) left += channel2.output.left;
  if(channel3.io.enable) left += channel3.output.left;
  if(channel4.io.enable) left += channel4.output.left;
  if(channel5.io.enable) left += channel5.output.left * io.headphonesConnected;

  s32 right = 0;
  if(channel1.io.enable) right += channel1.output.right;
  if(channel2.io.enable) right += channel2.output.right;
  if(channel3.io.enable) right += channel3.output.right;
  if(channel4.io.enable) right += channel4.output.right;
  if(channel5.io.enable) right += channel5.output.right * io.headphonesConnected;

  if(!io.headphonesConnected) {
    left = right = sclamp<16>((((left + right) >> io.speakerShift) & 0xFF) << 7);
  } else {
    left = sclip<16>(left << 5);
    right = sclip<16>(right << 5);
  }

  //ASWAN has three volume steps (0%, 50%, 100%); SPHINX and SPHINX2 have four (0%, 33%, 66%, 100%)
  f64 amplitude = 1.0 / (SoC::ASWAN() ? 2.0 : 3.0) * io.masterVolume;
  stream->frame(left / 32768.0 * amplitude, right / 32768.0 * amplitude);
}

auto APU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto APU::power() -> void {
  Thread::create(3'072'000, {&APU::main, this});

  bus.map(this, 0x004a, 0x004c);
  bus.map(this, 0x004e, 0x0050);
  bus.map(this, 0x0052);
  bus.map(this, 0x006a, 0x006b);
  bus.map(this, 0x0080, 0x0095);
  bus.map(this, 0x009e);

  dma.power();
  channel1.power();
  channel2.power();
  channel3.power();
  channel4.power();
  channel5.power();

  io = {};
  io.headphonesConnected = system.headphones->value();
  io.masterVolume = SoC::ASWAN() ? 2 : 3;
  state = {};

  state.dacClock = 0;
  state.sweepClock = 0;
}

}
