#include <ps1/ps1.hpp>

namespace ares::PlayStation {

auto amplify(i32 sample, i16 volume) -> i32 {
  return sample * volume >> 15;
}

SPU spu;
#include "io.cpp"
#include "fifo.cpp"
#include "adsr.cpp"
#include "gaussian.cpp"
#include "adpcm.cpp"
#include "reverb.cpp"
#include "noise.cpp"
#include "voice.cpp"
#include "envelope.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto SPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("SPU");

  stream = node->append<Node::Stream>("SPU");
  stream->setChannels(2);
  stream->setFrequency(44100.0);

  ram.allocate(512_KiB);
  fifo.resize(32);

  debugger.load(node);
}

auto SPU::unload() -> void {
  debugger = {};
  fifo.reset();
  ram.reset();
  stream.reset();
  node.reset();
}

auto SPU::main() -> void {
  sample();
  step(33'868'800 / 44'100);
}

auto SPU::sample() -> void {
  if(!master.enable) return stream->sample(0.0, 0.0);
  i32 lsum = 0, lreverb = 0;
  i32 rsum = 0, rreverb = 0;
  i16 modulation = 0;
  for(auto& voice : this->voice) {
    auto [lvoice, rvoice] = voice.sample(modulation);
    modulation = sclamp<16>(voice.adsr.lastVolume);
    lsum += lvoice;
    lsum += rvoice;
    if(voice.eon ) lreverb += lvoice;
    if(voice.eon ) rreverb += rvoice;
    if(voice.koff) voice.keyOff();
    if(voice.kon ) voice.keyOn();
  }
  noise.update();
  auto [lfb, rfb] = reverb.process(sclamp<16>(lreverb), sclamp<16>(rreverb));
  lsum = amplify(sclamp<16>(lsum + lfb), volume[0].level) * master.mute;
  rsum = amplify(sclamp<16>(rsum + rfb), volume[1].level) * master.mute;
  stream->sample(lsum / 32768.0, rsum / 32768.0);
}

auto SPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto SPU::power(bool reset) -> void {
  Thread::reset();
  adsrConstructTable();
  gaussianConstructTable();
}

}
