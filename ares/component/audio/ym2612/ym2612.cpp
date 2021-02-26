#include <ares/ares.hpp>
#include "ym2612.hpp"

namespace ares {

#include "io.cpp"
#include "timer.cpp"
#include "channel.cpp"
#include "constants.cpp"
#include "serialization.cpp"

auto YM2612::clock() -> array<i16[2]> {
  s32 left  = 0;
  s32 right = 0;

  for(auto& channel : channels) {
    auto& op = channel.operators;

    const s32 modMask = -(1 << 1);
    const s32 sumMask = -(1 << 5);
    const s32 outMask = -(1 << 5);

    auto old = [&](u32 n) -> s32 { return op[n].prior  & modMask; };
    auto mod = [&](u32 n) -> s32 { return op[n].output & modMask; };
    auto out = [&](u32 n) -> s32 { return op[n].output & sumMask; };

    auto wave = [&](u32 n, u32 modulation) -> s32 {
      s32 x = (modulation >> 1) + (op[n].phase.value >> 10);
      s32 y = sine[x & 0x3ff] + op[n].outputLevel;
      return y < 0x2000 ? pow2[y & 0x1ff] << 2 >> (y >> 9) : 0;
    };

    s32 feedback = modMask & op[0].output + op[0].prior >> 9 - channel.feedback;
    s32 accumulator = 0;

    for(auto n : range(4)) op[n].prior = op[n].output;

    op[0].output = wave(0, feedback * (channel.feedback > 0));

    if(channel.algorithm == 0) {
      //0 -> 1 -> 2 -> 3
      op[1].output = wave(1, mod(0));
      op[2].output = wave(2, old(1));
      op[3].output = wave(3, mod(2));
      accumulator += out(3);
    }

    if(channel.algorithm == 1) {
      //(0 + 1) -> 2 -> 3
      op[1].output = wave(1, 0);
      op[2].output = wave(2, mod(0) + old(1));
      op[3].output = wave(3, mod(2));
      accumulator += out(3);
    }

    if(channel.algorithm == 2) {
      //0 + (1 -> 2) -> 3
      op[1].output = wave(1, 0);
      op[2].output = wave(2, old(1));
      op[3].output = wave(3, mod(0) + mod(2));
      accumulator += out(3);
    }

    if(channel.algorithm == 3) {
      //(0 -> 1) + 2 -> 3
      op[1].output = wave(1, mod(0));
      op[2].output = wave(2, 0);
      op[3].output = wave(3, mod(1) + mod(2));
      accumulator += out(3);
    }

    if(channel.algorithm == 4) {
      //(0 -> 1) + (2 -> 3)
      op[1].output = wave(1, mod(0));
      op[2].output = wave(2, 0);
      op[3].output = wave(3, mod(2));
      accumulator += out(1) + out(3);
    }

    if(channel.algorithm == 5) {
      //0 -> (1 + 2 + 3)
      op[1].output = wave(1, mod(0));
      op[2].output = wave(2, old(0));
      op[3].output = wave(3, mod(0));
      accumulator += out(1) + out(2) + out(3);
    }

    if(channel.algorithm == 6) {
      //(0 -> 1) + 2 + 3
      op[1].output = wave(1, mod(0));
      op[2].output = wave(2, 0);
      op[3].output = wave(3, 0);
      accumulator += out(1) + out(2) + out(3);
    }

    if(channel.algorithm == 7) {
      //0 + 1 + 2 + 3
      op[1].output = wave(1, 0);
      op[2].output = wave(2, 0);
      op[3].output = wave(3, 0);
      accumulator += out(0) + out(1) + out(2) + out(3);
    }

    s32 voiceData = sclamp<14>(accumulator) & outMask;
    if(dac.enable && (&channel == &channels[5])) voiceData = (s32)dac.sample - 0x80 << 6;

    if(channel.leftEnable ) left  += voiceData;
    if(channel.rightEnable) right += voiceData;
  }

  timerA.run();
  timerB.run();

  if(lfo.enable && ++lfo.divider == lfoDividers[lfo.rate]) {
    lfo.divider = 0;
    lfo.clock++;
    for(auto& channel : channels) {
      for(auto& op : channel.operators) {
        op.updatePhase();  //due to vibrato
        op.updateLevel();  //due to tremolo
      }
    }
  }

  if(++envelope.divider == 3) {
    envelope.divider = 0;
    envelope.clock++;
  }

  for(auto& channel : channels) {
    for(auto& op : channel.operators) {
      op.runPhase();
      if(envelope.divider) continue;
      op.runEnvelope();
    }
  }

  return {sclamp<16>(left), sclamp<16>(right)};
}

auto YM2612::power() -> void {
  io = {};
  lfo = {};
  dac = {};
  envelope = {};
  timerA = {};
  timerB = {};
  for(auto& channel : channels) channel.power();

  const u32 positive = 0;
  const u32 negative = 1;

  for(s32 x = 0; x <= 0xff; x++) {
    s32 y = -256 * log(sin((2 * x + 1) * Math::Pi / 1024)) / log(2) + 0.5;
    sine[0x000 + x] = positive + (y << 1);
    sine[0x1ff - x] = positive + (y << 1);
    sine[0x200 + x] = negative + (y << 1);
    sine[0x3ff - x] = negative + (y << 1);
  }

  for(s32 y = 0; y <= 0xff; y++) {
    s32 z = 1024 * pow(2, (0xff - y) / 256.0) + 0.5;
    pow2[positive + (y << 1)] = +z;
    pow2[negative + (y << 1)] = ~z;  //not -z
  }
}

}
