#pragma once

namespace ares {

//Yamaha YM2413 (OPLL)
//Author: Talarubi

struct YM2413 {
  //ym2413.cpp
  auto clock() -> double;
  auto reload(uint4 voice) -> void;
  auto power(bool isVRC7 = false) -> void;

  //io.cpp
  auto address(uint8 data) -> void;
  auto write(uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  static const   int8 levelScaling[16];
  static const uint64 envelopeSteps[17][2];
  static const  uint8 melodicTonesOPLL[15][8];
  static const  uint8 rhythmTonesOPLL[3][8];
  static const  uint8 melodicTonesVRC7[15][8];
  static const  uint8 rhythmTonesVRC7[3][8];
  static uint13 sinTable[1 << 10];
  static  int12 expTable[1 <<  9];
  uint8 melodicTones[15][8];
  uint8 rhythmTones[3][8];
  uint8 customTone[8];

  enum : uint { Trigger, Attack, Decay, Sustain, Release };

  struct Operator {
    //operator.cpp
    auto releaseRate() const -> uint4;
    auto update(maybe<uint> state = {}) -> void;
    auto synchronize(uint1 hard, maybe<Operator&> modulator = {}) -> void;
    auto trigger(uint1 key, uint1 sustain = 0) -> void;
    auto clock(natural clock, integer offset, integer modulation = 0) -> int12;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint5 slot;  //1-18
    uint1 keyOn;
    uint1 sustainOn;
    uint4 multiple;
    uint1 scaleRate;
    uint1 sustainable;
    uint1 vibrato;
    uint1 tremolo;
    uint2 scaleLevel;
    uint1 waveform;
    uint4 attack;
    uint4 decay;
    uint4 sustain;
    uint4 release;
    uint6 totalLevel;
    uint1 audible;

    //envelope generator
     uint3 state = Release;
     uint7 rate;
     uint4 divider;
    uint64 sequence;
     uint7 envelope = 0x7f;
     uint7 level;

    //phase generator
     uint9 fnumber;
     uint3 block;
    uint19 pitch[8];
    uint19 phase;

    //output
    int12 output;
    int12 prior;
  };

  struct Voice {
    //voice.cpp
    auto update(const uint8* data = nullptr) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint4 tone;
    uint9 fnumber;
    uint3 block;
    uint4 level;
    uint3 feedback;

    Operator modulator;
    Operator carrier;
  } voices[9];

  struct IO {
    uint32 clock;
     uint8 address;
     uint1 rhythmMode;
    uint23 noise = 1;
     uint1 isVRC7 = 0;
  } io;

  Voice& bass      = voices[6];
  Operator& hihat  = voices[7].modulator;
  Operator& snare  = voices[7].carrier;
  Operator& tomtom = voices[8].modulator;
  Operator& cymbal = voices[8].carrier;
};

}
