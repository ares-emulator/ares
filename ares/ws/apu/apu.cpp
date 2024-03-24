#include <ws/ws.hpp>

namespace ares::WonderSwan {

APU apu;
#include "debugger.cpp"
#include "io.cpp"
#include "dma.cpp"
#include "channel1.cpp"
#include "channel2.cpp"
#include "channel3.cpp"
#include "channel4.cpp"
#include "channel5.cpp"
#include "serialization.cpp"

auto APU::setAccurate(bool value) -> void {
  accurate = value;
}

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("APU");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(2);
  stream->setFrequency(24'000);
  stream->addHighPassFilter(15.915, 1);

  debugger.load(node);
}

auto APU::unload() -> void {
  debugger.unload(node);

  node->remove(stream);
  stream.reset();
  node.reset();
}

auto APU::main() -> void {
  // further verification could always be useful
  u32 steps = accurate ? 1 : 128;
  for(u32 s = 0; s < steps; s++) {
    // TODO: is the period value (run()) updated before or after the outputs (runOutput())?
    channel1.run();
    channel2.run();
    channel3.run();
    if(++state.sweepClock == 0) channel3.sweep(); // TODO: which cycle is this, or is it separate?
    channel4.run();

    // TODO: are voice/noise modes handled on different cycles than tone modes?
    switch(state.apuClock++) {
    case 0: if(channel1.io.enable)                      channel1.runOutput(); break;
    case 1: if(channel2.io.enable || channel2.io.voice) channel2.runOutput(); break;
    case 2: if(channel3.io.enable)                      channel3.runOutput(); break;
    case 3: if(channel4.io.enable)                      channel4.runOutput(); break;
    case 4: if(channel5.io.enable)                      channel5.runOutput(); break; // TODO: which cycle is this?
    case 5: dma.run(); break; // TODO: which cycle is this?
    case 6: dacRun(); break; // TODO: which cycle is this?
    }
  }
  step(steps);
}

auto APU::sample(u32 channel, n5 index) -> n4 {
  n8 data = iram.read((io.waveBase << 6) + (--channel << 4) + (index >> 1));
  if(index.bit(0) == 0) return data.bit(0,3);
  if(index.bit(0) == 1) return data.bit(4,7);
  unreachable;
}

auto APU::dacRun() -> void {
  bool outputEnable = io.headphonesConnected ? io.headphonesEnable : io.speakerEnable;

  if(!outputEnable) {
    stream->frame(0, 0);
    return;
  }

  s32 left = 0;
  if(channel1.io.enable)                      left += channel1.output.left;
  if(channel2.io.enable || channel2.io.voice) left += channel2.output.left;
  if(channel3.io.enable)                      left += channel3.output.left;
  if(channel4.io.enable)                      left += channel4.output.left;

  s32 right = 0;
  if(channel1.io.enable)                      right += channel1.output.right;
  if(channel2.io.enable || channel2.io.voice) right += channel2.output.right;
  if(channel3.io.enable)                      right += channel3.output.right;
  if(channel4.io.enable)                      right += channel4.output.right;

  state.seqOutputLeft = left;
  state.seqOutputRight = right;
  state.seqOutputSum = left + right;

  if(!io.headphonesConnected) {
    left = right = sclamp<16>(((state.seqOutputSum >> io.speakerShift) & 0xFF) << 7);
  } else {
    left = sclip<16>(left << 5);
    right = sclip<16>(right << 5);

    if(channel5.io.enable) {
      left += channel5.output.left;
      right += channel5.output.right;
    }
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
  bus.map(this, 0x0064, 0x006b);
  bus.map(this, 0x0080, 0x009b);
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

  state.apuClock = 0;
  state.sweepClock = 0;
}

}
