#pragma once

namespace ares {

//Yamaha YM2612 (OPN2)
//Author: Talarubi

struct YM2612 {
  //ym2612.cpp
  auto clock() -> array<i16[2]>;
  auto power() -> void;

  //io.cpp
  auto readStatus() -> n8;
  auto writeAddress(n9 data) -> void;
  auto writeData(n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct IO {
    n9 address = 0;
  } io;

  struct LFO {
    n1  enable = 0;
    n3  rate = 0;
    n32 clock = 0;
    n32 divider = 0;
  } lfo;

  struct DAC {
    n1 enable = 0;
    n8 sample = 0x80;
  } dac;

  struct Envelope {
    n32 clock = 0;
    n32 divider = 0;
  } envelope;

  struct TimerA {
    //timer.cpp
    auto run() -> void;

    n1  enable = 0;
    n1  irq = 0;
    n1  line = 0;
    n10 period = 0;
    n10 counter = 0;
  } timerA;

  struct TimerB {
    //timer.cpp
    auto run() -> void;

    n1 enable = 0;
    n1 irq = 0;
    n1 line = 0;
    n8 period = 0;
    n8 counter = 0;
    n4 divider = 0;
  } timerB;

  enum : u32 { Attack, Decay, Sustain, Release };

  struct Channel {
    YM2612& ym2612;
    Channel(YM2612& ym2612) : ym2612(ym2612) {}

    //channel.cpp
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 leftEnable = 1;
    n1 rightEnable = 1;

    n3 algorithm = 0;
    n3 feedback = 0;
    n3 vibrato = 0;
    n2 tremolo = 0;

    n2 mode = 0;

    struct Operator {
      Channel& channel;
      YM2612& ym2612;
      Operator(Channel& channel) : channel(channel), ym2612(channel.ym2612) {}

      //channel.cpp
      auto trigger(bool) -> void;

      auto runEnvelope() -> void;
      auto runPhase() -> void;

      auto updateEnvelope() -> void;
      auto updatePitch() -> void;
      auto updatePhase() -> void;
      auto updateLevel() -> void;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n1 keyOn = 0;
      n1 lfoEnable = 0;
      n3 detune = 0;
      n4 multiple = 0;
      n7 totalLevel = 0;

      n16 outputLevel = 0x1fff;
      i16 output = 0;
      i16 prior = 0;

      struct Pitch {
        n11 value = 0;
        n11 reload = 0;
        n11 latch = 0;
      } pitch;

      struct Octave {
        n3 value = 0;
        n3 reload = 0;
        n3 latch = 0;
      } octave;

      struct Phase {
        n20 value = 0;
        n20 delta = 0;
      } phase;

      struct Envelope {
        u32 state = Release;
        s32 rate = 0;
        s32 divider = 11;
        n32 steps = 0;
        n10 value = 0x3ff;

        n2  keyScale = 0;
        n5  attackRate = 0;
        n5  decayRate = 0;
        n5  sustainRate = 0;
        n4  sustainLevel = 0;
        n5  releaseRate = 1;
      } envelope;

      struct SSG {
        n1 enable = 0;
        n1 attack = 0;
        n1 alternate = 0;
        n1 hold = 0;
        n1 invert = 0;
      } ssg;
    } operators[4]{*this, *this, *this, *this};

    auto operator[](n2 index) -> Operator& { return operators[index]; }
  } channels[6] = {*this, *this, *this, *this, *this, *this};

  n16 sine[0x400];
  i16 pow2[0x200];

  //constants.cpp
  struct EnvelopeRate {
    u32 divider;
    u32 steps[4];
  };

  static const u8 lfoDividers[8];
  static const u8 vibratos[8][16];
  static const u8 tremolos[4];
  static const u8 detunes[3][8];
  static const EnvelopeRate envelopeRates[16];
};

}
