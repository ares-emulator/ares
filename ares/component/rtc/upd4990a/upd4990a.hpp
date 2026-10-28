#pragma once

namespace ares {

//NEC uPD4990A Calendar & Clock

struct uPD4990A {
  //upd4990a.cpp
  auto load() -> void;
  auto save() -> void;
  auto power() -> void;
  auto tick() -> void;
  auto tickSecond() -> void;

  auto dataOut() -> n1;
  auto timePulse() -> n1;
  auto write(n1 din, n1 clock, n1 strobe) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  static constexpr u32 size = 16;
  n8 data[size];

  auto second()       -> n8& { return data[0]; }  //BCD 00-59
  auto minute()       -> n8& { return data[1]; }  //BCD 00-59
  auto hour()         -> n8& { return data[2]; }  //BCD 00-23
  auto day()          -> n8& { return data[3]; }  //BCD 01-31
  auto monthWeekday() -> n8& { return data[4]; }  //(month << 4) | weekday
  auto year()         -> n8& { return data[5]; }  //BCD 00-99

  auto getMonth()       -> u8 { return data[4] >> 4; }
  auto getWeekday()     -> u8 { return data[4] & 0x0f; }
  auto setMonth(u8 m)   -> void { data[4] = (m << 4) | (data[4] & 0x0f); }
  auto setWeekday(u8 w) -> void { data[4] = (data[4] & 0xf0) | (w & 0x0f); }

  n1 clockPin;
  n1 strobePin;

  n8 shiftRegister[6];

  n4  commandRegister;
  n4  command;
  n1  outputBit;
  n1  timePulseOutput;
  n15 counter;
  n4  timePulseMode;
  u32 timePulseCounter;

  auto timestamp() -> n64 {
    n64 value = 0;
    for(auto index : range(8)) value.byte(index) = data[8 + index];
    return value;
  }

  enum Command : u32 {
    RegisterHold    = 0x0,
    RegisterShift   = 0x1,
    TimeSet         = 0x2,
    TimeRead        = 0x3,
    TimePulse64Hz   = 0x4,
    TimePulse256Hz  = 0x5,
    TimePulse2048Hz = 0x6,
    TimePulse1s     = 0x8,
    TimePulse10s    = 0x9,
    TimePulse30s    = 0xa,
    TimePulse60s    = 0xb,
    Test            = 0xf,
  };
};

}
