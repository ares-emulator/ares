#pragma once

namespace ares {

//Seiko S-3511A Real-Time Clock

struct S3511A {
  // bytes  0- 6: date/time data, BCD
  // bytes  7- 7: status register
  // bytes  8-15: last timestamp
  // bytes 16-17: alarm register
  static constexpr u32 size = 18;
  n8 data[size];

  virtual auto irqLevel(bool value) -> void = 0;

  //s3511a.cpp
  auto load() -> void;
  auto save() -> void;
  auto power() -> void;
  auto tickSecond() -> void;
  auto checkAlarm() -> void;
  auto readSIO() -> n1;
  auto writeCS(n1 data) -> void;
  auto writeSIO(n1 data) -> void;
  auto writeSCK(n1 data) -> void;
  auto writeCommand() -> void;
  auto readRegister() -> void;
  auto writeRegister() -> void;
  auto initRegs(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //pin states
  n1 cs;
  n1 sio;
  n1 sck;

  //buffered command data
  n8 inBuffer;
  n8 outBuffer;
  n3 shift;
  n3 index;

  //internal state
  n1  rwSelect;
  n3  regSelect;
  n1  cmdLatched;
  n15 counter;

  auto year()        -> n8& { return data[ 0]; }
  auto month()       -> n8& { return data[ 1]; }
  auto day()         -> n8& { return data[ 2]; }
  auto weekday()     -> n8& { return data[ 3]; }
  auto hour()        -> n8& { return data[ 4]; }
  auto minute()      -> n8& { return data[ 5]; }
  auto second()      -> n8& { return data[ 6]; }
  auto status()      -> n8& { return data[ 7]; }
  auto alarmHour()   -> n8& { return data[16]; }
  auto alarmMinute() -> n8& { return data[17]; }
};

}
