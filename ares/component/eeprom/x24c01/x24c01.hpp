#pragma once

namespace ares {

struct X24C01 {
  auto size() const -> u32 {
    return 128;
  }

  auto addressBits() const -> u32 {
    return 8;
  }

  auto dataBits() const -> u32 {
    return 8;
  }

  //x24c01.cpp
  auto power() -> void;
  auto read() -> n1;
  auto write(n1 clock, n1 data) -> void;
  auto erase(n8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  u8 bytes[128];

private:
  enum class Mode : u32 {
    Idle,
    Address,
    AddressAcknowledge,
    Read,
    ReadAcknowledge,
    Write,
    WriteAcknowledge,
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
};

}
