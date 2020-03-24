#pragma once

namespace ares {

//X24C01 => 1024 cells => 128 x 8-bit

struct X24C01 {
  //x24c01.cpp
  auto reset() -> void;
  auto read() -> uint1;
  auto write(uint1 clock, uint1 data) -> void;
  auto erase(uint8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint8 memory[128];

private:
  enum class Mode : uint {
    Idle,
    Address, AddressAcknowledge,
    Read, ReadAcknowledge,
    Write, WriteAcknowledge,
  };

  struct Line {
    //x24c01.cpp
    auto write(uint1 data) -> void;

    uint1 lo;
    uint1 hi;
    uint1 fall;
    uint1 rise;
    uint1 line;
  };

  Line clock;
  Line data;
  Mode mode;
  uint8 counter;
  uint8 address;
  uint8 input;
  uint8 output;
  uint1 line;
};

}
