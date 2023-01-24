#pragma once

#include <component/audio/ym2612/ym2612.hpp>
#include <component/audio/ym2149/ym2149.hpp>

namespace ares {

//Yamaha YM2610 (OPNB)

struct YM2610 {
  //ym2610.cpp
  auto power() -> void;

  //io.cpp
  auto read(n2 address) -> n8;
  auto write(n2 address, n8 data) -> void;
  auto writeLower(n8 data) -> void;
  auto writeUpper(n8 data) -> void;
  auto pcmStatus() -> n8;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  virtual auto readPCMA(u32 address) -> u8 { return 0; }
  virtual auto readPCMB(u32 address) -> u8 { return 0; }

protected:
  n9 registerAddress;
  YM2612 fm;
  YM2149 ssg;

  struct PCMA {
    YM2610& self;

    // pcm.cpp
    auto power() -> void;
    auto clock() -> array<i16[2]>;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Channel {
      PCMA& self;
      auto keyOn() -> void;
      auto keyOff() -> void;
      auto decode(n8) -> i12;
      n1 playing;
      n1 ended;
      n1 endedMask;
      n1 left;
      n1 right;
      n5 volume;
      n32 startAddress;
      n32 endAddress;
      n32 currentAddress;
      i12 decodeAccumulator;
      i32 decodeStep;
      bool currentNibble;
    } channels[6]{{*this}, {*this}, {*this}, {*this}, {*this}, {*this}};

    n6 volume;
    int decodeTable[16 * 49];
  } pcmA{*this};

  struct PCMB {
    YM2610& self;

    // pcm.cpp
    auto power() -> void;
    auto clock() -> array<i16[2]>;
    auto decode() -> void;
    auto beginPlay() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1  playing;
    n1  ended;
    n1  endedMask;
    n1  repeat;
    n1  left;
    n2  right;
    n32 startAddress;
    n32 endAddress;
    n16 delta;
    n8  volume;

    n32 currentAddress;
    bool currentNibble;
    i32 decodeAccumulator;
    u32 decodePosition;
    i32 previousAccumulator;
    i32 decodeStep;
  } pcmB{*this};
};

}
