#pragma once

namespace ares {

//X24C01 => 1024 cells => 128 x 8-bit

struct X24C01 {
  //x24c01.cpp
  auto reset() -> void;
  auto read() -> n1;
  auto write(n1 clock, n1 data) -> void;
  auto erase(n8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8 memory[128];

private:
  enum class Mode : u32 {
    Idle,
    Address, AddressAcknowledge,
    Read, ReadAcknowledge,
    Write, WriteAcknowledge,
  };

  struct Line {
    //x24c01.cpp
    auto write(n1 data) -> void;

    n1 lo;
    n1 hi;
    n1 fall;
    n1 rise;
    n1 line;
  };

  Line clock;
  Line data;
  Mode mode;
  n8 counter;
  n8 address;
  n8 input;
  n8 output;
  n1 line;
};

}
