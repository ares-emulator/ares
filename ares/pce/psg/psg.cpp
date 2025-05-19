#include <pce/pce.hpp>

namespace ares::PCEngine {

PSG psg;
#include "io.cpp"
#include "channel.cpp"
#include "serialization.cpp"

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = parent->append<Node::Audio::Stream>("PSG");
  stream->setChannels(2);
  #if defined(PROFILE_ACCURACY)
  stream->setFrequency(system.colorburst());
  #endif
  #if defined(PROFILE_PERFORMANCE)
  stream->setFrequency(system.colorburst() / 64.0);
  #endif
  stream->addHighPassFilter(20.0, 1);
}

auto PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto PSG::main() -> void {
  i16 outputLeft;
  i16 outputRight;

  #if defined(PROFILE_ACCURACY)
  frame<1>(outputLeft, outputRight);
  stream->frame(sclamp<16>(outputLeft) / 32768.0, sclamp<16>(outputRight) / 32768.0);
  step(1);
  #endif

  #if defined(PROFILE_PERFORMANCE)
  //3.57MHz stereo audio through a 6th-order biquad IIR filter is very demanding.
  //decimate the audio to ~56KHz, which is still well above the range of human hearing.
  frame<64>(outputLeft, outputRight);
  stream->frame(sclamp<16>(outputLeft) / 32768.0, sclamp<16>(outputRight) / 32768.0);
  step(64);
  #endif
}

template<int stepArg> auto PSG::frame(i16& outputLeft, i16& outputRight) -> void {
  static const n5 volumeScale[16] = {
    0x00, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f,
    0x10, 0x13, 0x15, 0x17, 0x19, 0x1b, 0x1d, 0x1f,
  };

  outputLeft  = 0;
  outputRight = 0;

  n5 lmal = volumeScale[io.volumeLeft];
  n5 rmal = volumeScale[io.volumeRight];

  n5 output[6];
  if(channel[0].io.enable) output[0] = channel[0].run<0, stepArg>();
  if(channel[1].io.enable) output[1] = channel[1].run<1, stepArg>();
  if(channel[2].io.enable) output[2] = channel[2].run<2, stepArg>();
  if(channel[3].io.enable) output[3] = channel[3].run<3, stepArg>();
  if(channel[4].io.enable) output[4] = channel[4].run<4, stepArg>();
  if(channel[5].io.enable) output[5] = channel[5].run<5, stepArg>();

  for(u32 C : range(6)) {
    n5  al = channel[C].io.volume;
    n5 lal = volumeScale[channel[C].io.volumeLeft];
    n5 ral = volumeScale[channel[C].io.volumeRight];

    n5 volumeLeft  = min(0x1f, (0x1f - lmal) + (0x1f - lal) + (0x1f - al));
    n5 volumeRight = min(0x1f, (0x1f - rmal) + (0x1f - ral) + (0x1f - al));

    if(C == 1 && io.lfoEnable) {
      //todo: frequency modulation of channel 0 using channel 1's output
    } else {
      outputLeft  += output[C] * volumeScalar[volumeLeft];
      outputRight += output[C] * volumeScalar[volumeRight];
    }
  }
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

auto PSG::power() -> void {
  Thread::create(system.colorburst(), {&PSG::main, this});

  io = {};
  for(auto C : range(6)) channel[C].power(C);

  f64 level = 32768.0 / 6.0 / 32.0;  //max volume / channels / steps
  f64 step = 48.0 / 32.0;            //48dB volume range spread over 32 steps
  for(u32 n : range(31)) {
    volumeScalar[n] = level;
    level /= pow(10.0, step / 20.0);
  }
  volumeScalar[31] = 0.0;
}

}
