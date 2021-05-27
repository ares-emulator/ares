#pragma once

namespace ares {

//X24C01

struct M24Cx {
  auto size() const -> u32 {
    return 128;
  }

  auto writeProtected() const -> bool {
    return !writable;
  }

  auto setWriteProtected(bool protect) -> void {
    writable = !protect;
  }

  //m24cx.cpp
  auto power() -> void;
  auto read() -> n1;
  auto write(maybe<n1> clock, maybe<n1> data) -> void;
  auto erase(n8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  u8 memory[128];

private:
  //m24cx.cpp
  auto load() -> bool;
  auto store() -> bool;

  static constexpr bool Acknowledge = 0;

  enum class Mode : u32 {
    Standby,
    Address,
    Read,
    Write,
  };

  struct Line {
    //m24cx.cpp
    auto write(n1 data) -> void;

    n1 lo;
    n1 hi;
    n1 fall;
    n1 rise;
    n1 line;
  };

  Mode mode;
  Line clock;
  Line data;
  n8   counter;
  n8   address;
  n8   input;
  n8   output;
  n1   response;
  n1   writable;
};

}
