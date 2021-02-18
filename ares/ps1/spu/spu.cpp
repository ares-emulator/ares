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

  adsrConstructTable();
  gaussianConstructTable();

  debugger.load(node);
}

auto SPU::unload() -> void {
  debugger = {};
  ram.reset();
  node->remove(stream);
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

auto SPU::step(u32 clocks) -> void {
  Thread::clock += clocks;
}

auto SPU::power(bool reset) -> void {
  Thread::reset();
  Memory::Interface::setWaitStates(17, 17, 18);
  ram.fill();

  master = {};
  noise.step = 0;
  noise.shift = 0;
  noise.level = 0;
  noise.count = 0;
  transfer = {};
  irq = {};
  envelope = {};
  volume[0] = {};
  volume[1] = {};
  cdaudio = {};
  external = {};
  current = {};
  reverb.enable = 0;
  reverb.vLOUT = 0;
  reverb.vROUT = 0;
  reverb.mBASE = 0;
  reverb.FB_SRC_A = 0;
  reverb.FB_SRC_B = 0;
  reverb.IIR_ALPHA = 0;
  reverb.ACC_COEF_A = 0;
  reverb.ACC_COEF_B = 0;
  reverb.ACC_COEF_C = 0;
  reverb.ACC_COEF_D = 0;
  reverb.IIR_COEF = 0;
  reverb.FB_ALPHA = 0;
  reverb.FB_X = 0;
  reverb.IIR_DEST_A0 = 0;
  reverb.IIR_DEST_A1 = 0;
  reverb.ACC_SRC_A0 = 0;
  reverb.ACC_SRC_A1 = 0;
  reverb.ACC_SRC_B0 = 0;
  reverb.ACC_SRC_B1 = 0;
  reverb.IIR_SRC_A0 = 0;
  reverb.IIR_SRC_A1 = 0;
  reverb.IIR_DEST_B0 = 0;
  reverb.IIR_DEST_B1 = 0;
  reverb.ACC_SRC_C0 = 0;
  reverb.ACC_SRC_C1 = 0;
  reverb.ACC_SRC_D0 = 0;
  reverb.ACC_SRC_C1 = 0;
  reverb.IIR_SRC_B1 = 0;
  reverb.IIR_SRC_B0 = 0;
  reverb.MIX_DEST_A0 = 0;
  reverb.MIX_DEST_A1 = 0;
  reverb.MIX_DEST_B0 = 0;
  reverb.MIX_DEST_B1 = 0;
  reverb.IN_COEF_L = 0;
  reverb.IN_COEF_R = 0;
  reverb.lastInput[0] = 0;
  reverb.lastInput[1] = 0;
  reverb.lastOutput[0] = 0;
  reverb.lastOutput[1] = 0;
  for(u32 x : range(2)) {
    for(u32 y : range(128)) reverb.downsampleBuffer[x][y] = 0;
    for(u32 y : range(64)) reverb.upsampleBuffer[x][y] = 0;
  }
  reverb.resamplePosition = 0;
  reverb.baseAddress = 0;
  reverb.currentAddress = 0;
  for(auto& v : voice) {
    v.adpcm = {};
    v.block = {};
    v.attack = {};
    v.decay = {};
    v.sustain = {};
    v.release = {};
    v.adsr = {};
    v.current = {};
    v.volume[0] = {};
    v.volume[1] = {};
    v.envelope = {};
    v.counter = 0;
    v.pmon = 0;
    v.non = 0;
    v.eon = 0;
    v.kon = 0;
    v.koff = 0;
    v.endx = 0;
  }
  capture = {};
  fifo.flush();
}

}
