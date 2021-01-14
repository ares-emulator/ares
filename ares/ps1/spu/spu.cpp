#include <ps1/ps1.hpp>

namespace ares::PlayStation {

auto amplify(s32 sample, s16 volume) -> s32 {
  return (s64)sample * volume >> 15;
}

SPU spu;
#include "io.cpp"
#include "fifo.cpp"
#include "capture.cpp"
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
  node = parent->append<Node::Object>("SPU");

  stream = node->append<Node::Audio::Stream>("SPU");
  stream->setChannels(2);
  stream->setFrequency(44100.0);

  ram.allocate(512_KiB);

  debugger.load(node);
}

auto SPU::unload() -> void {
  debugger = {};
  ram.reset();
  stream.reset();
  node.reset();
}

auto SPU::main() -> void {
  sample();
  step(33'868'800 / 44'100);
}

auto SPU::sample() -> void {
  s32 lsum = 0, lreverb = 0;
  s32 rsum = 0, rreverb = 0;
  s16 lcdaudio = 0, rcdaudio = 0;
  s32 modulation = 0;
  for(auto& voice : this->voice) {
    auto [lvoice, rvoice] = voice.sample(modulation);
    modulation = voice.adsr.lastVolume;
    lsum += lvoice;
    rsum += rvoice;
    if(voice.eon) {
      lreverb += lvoice;
      rreverb += rvoice;
    }
    if(voice.koff) voice.keyOff();
    if(voice.kon ) voice.keyOn();
  }
  if(!master.unmute || !master.enable) {
    lsum = 0;
    rsum = 0;
  }
  noise.update();
  if(cdaudio.enable) {
    lcdaudio = disc.cdda.sample.left  + disc.cdxa.sample.left;
    rcdaudio = disc.cdda.sample.right + disc.cdxa.sample.right;
    lcdaudio = amplify(lcdaudio, cdaudio.volume[0]);
    rcdaudio = amplify(rcdaudio, cdaudio.volume[1]);
    lsum += lcdaudio;
    rsum += rcdaudio;
    if(cdaudio.reverb) {
      lreverb += lcdaudio;
      rreverb += rcdaudio;
    }
  }
  auto [lfb, rfb] = reverb.process(sclamp<16>(lreverb), sclamp<16>(rreverb));
  lsum = amplify(sclamp<16>(lsum + lfb), volume[0].current);
  rsum = amplify(sclamp<16>(rsum + rfb), volume[1].current);
  volume[0].tick();
  volume[1].tick();
  captureVolume(0, lcdaudio);
  captureVolume(1, rcdaudio);
  captureVolume(2, sclamp<16>(voice[1].adsr.lastVolume));
  captureVolume(3, sclamp<16>(voice[3].adsr.lastVolume));
  capture.address += 2;
  stream->frame(lsum / 32768.0, rsum / 32768.0);
}

auto SPU::step(uint clocks) -> void {
  Thread::clock += clocks;
}

auto SPU::power(bool reset) -> void {
  Thread::reset();
  Memory::Interface::setWaitStates(17, 17, 18);
  adsrConstructTable();
  gaussianConstructTable();
}

}
