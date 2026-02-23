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
  if(accurate) {
    // TODO: Are the channels ticked before or after the memory fetch?
    channel1.tick();
    channel2.tick();
    channel3.tick();
    if(++state.sweepClock == 0 || io.seqDbgSweepClock) channel3.sweep();
    channel4.tick();

    switch(state.apuClock++) {
    case 0:   apu.output(); break;
    case 122:
      // TODO: Sound DMA should be blocking the CPU instead.
      dma.run();
      apu.sequencerClear();
      break;
    case 123: if(channel5.io.enable)                      channel5.runOutput(); break; // TODO: Which cycle is this performed on?
    case 124: if(channel1.io.enable)                      channel1.output(); break;
    case 125: if(channel2.io.enable || channel2.io.voice) channel2.output(); break;
    case 126: if(channel3.io.enable)                      channel3.output(); break;
    case 127: if(channel4.io.enable)                      channel4.output(); break;
    }

    step(1);
  } else {
    dma.run();
    apu.sequencerClear();

    for(u32 s = 0; s < 128; s++) {
      channel1.tick();
      channel2.tick();
      channel3.tick();
      if(++state.sweepClock == 0 || io.seqDbgSweepClock) channel3.sweep();
      channel4.tick();
    }
    
    channel5.runOutput();
    if(channel1.io.enable)                      channel1.output();
    if(channel2.io.enable || channel2.io.voice) channel2.output();
    if(channel3.io.enable)                      channel3.output();
    if(channel4.io.enable)                      channel4.output();

    apu.output();

    step(128);
  }
}

auto APU::sequencerClear() -> void {
  io.output = {};
  if(io.seqDbgOutputForce55) {
    io.output.left  = 0x55;
    io.output.right = 0x55;
  }
}

auto APU::sequencerHeld() -> bool {
  return io.seqDbgHold || io.seqDbgOutputForce55;
}

auto APU::sample(u32 channel, n5 index) -> n4 {
  if(io.seqDbgChForce4) return 4;
  if(io.seqDbgChForce2) return 2;
  
  n8 data = iram.read((io.waveBase << 6) + (--channel << 4) + (index >> 1));
  if(index.bit(0) == 0) return data.bit(0,3);
  if(index.bit(0) == 1) return data.bit(4,7);
  unreachable;
}

auto APU::output() -> void {
  bool outputEnable = io.headphonesConnected ? io.headphonesEnable : io.speakerEnable;

  if(!outputEnable) {
    stream->frame(0, 0);
    return;
  }

  s32 left  = io.output.left;
  s32 right = io.output.right;

  if(io.headphonesConnected) {
    left = sclip<16>(left << 5);
    right = sclip<16>(right << 5);
    if(channel5.io.enable) {
      left  += channel5.output.left;
      right += channel5.output.right;
    }
  } else {
    left = right = sclamp<16>((((left + right) >> io.speakerShift) & 0xFF) << 7);
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
  Thread::create(3'072'000, std::bind_front(&APU::main, this));

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
