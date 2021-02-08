#pragma once

namespace ares {

//Yamaha YM2413 (OPLL)
//Author: Talarubi

struct YM2413 {
  //ym2413.cpp
  auto clock() -> f64;
  auto reload(n4 voice) -> void;
  auto power(bool isVRC7 = false) -> void;

  //io.cpp
  auto address(n8 data) -> void;
  auto write(n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  static const i8  levelScaling[16];
  static const n64 envelopeSteps[17][2];
  static const n8  melodicTonesOPLL[15][8];
  static const n8  rhythmTonesOPLL[3][8];
  static const n8  melodicTonesVRC7[15][8];
  static const n8  rhythmTonesVRC7[3][8];
  static n13 sinTable[1 << 10];
  static i12 expTable[1 <<  9];
  n8 melodicTones[15][8];
  n8 rhythmTones[3][8];
  n8 customTone[8];

  enum : u32 { Trigger, Attack, Decay, Sustain, Release };

  struct Operator {
    //operator.cpp
    auto releaseRate() const -> n4;
    auto update(maybe<u32> state = {}) -> void;
    auto synchronize(n1 hard, maybe<Operator&> modulator = {}) -> void;
    auto trigger(n1 key, n1 sustain = 0) -> void;
    auto clock(natural clock, integer offset, integer modulation = 0) -> i12;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n5 slot;  //1-18
    n1 keyOn;
    n1 sustainOn;
    n4 multiple;
    n1 scaleRate;
    n1 sustainable;
    n1 vibrato;
    n1 tremolo;
    n2 scaleLevel;
    n1 waveform;
    n4 attack;
    n4 decay;
    n4 sustain;
    n4 release;
    n6 totalLevel;
    n1 audible;

    //envelope generator
    n3  state = Release;
    n7  rate;
    n4  divider;
    n64 sequence;
    n7  envelope = 0x7f;
    n7  level;

    //phase generator
    n9  fnumber;
    n3  block;
    n19 pitch[8];
    n19 phase;

    //output
    i12 output;
    i12 prior;
  };

  struct Voice {
    //voice.cpp
    auto update(const n8* data = nullptr) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n4 tone;
    n9 fnumber;
    n3 block;
    n4 level;
    n3 feedback;

    Operator modulator;
    Operator carrier;
  } voices[9];

  struct IO {
    n32 clock;
    n8  address;
    n1  rhythmMode;
    n23 noise = 1;
    n1  isVRC7 = 0;
  } io;

  Voice& bass      = voices[6];
  Operator& hihat  = voices[7].modulator;
  Operator& snare  = voices[7].carrier;
  Operator& tomtom = voices[8].modulator;
  Operator& cymbal = voices[8].carrier;
};

}
