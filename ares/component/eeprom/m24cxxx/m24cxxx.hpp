#pragma once

namespace ares {

struct M24Cxxx {
  enum class Type : u32 {
    M24C32,   // 32768 cells =>  4096 x 8-bit x 1-block
    M24C64,   // 65536 cells =>  8192 x 8-bit x 1-block
    M24C65,   // 65536 cells =>  8192 x 8-bit x 1-block
    M24C128,  //131072 cells => 16384 x 8-bit x 1-block
    M24C256,  //262144 cells => 32768 x 8-bit x 1-block
    M24C512,  //524288 cells => 65536 x 8-bit x 1-block
  };

  auto size() const -> u32 {
    switch(type) { default:
    case Type::M24C32:  return  4096;
    case Type::M24C64:  return  8192;
    case Type::M24C65:  return  8192;
    case Type::M24C128: return 16384;
    case Type::M24C256: return 32768;
    case Type::M24C512: return 65536;
    }
  }

  auto writeProtected() const -> bool {
    return !writable;
  }

  auto setWriteProtected(bool protect) -> void {
    writable = !protect;
  }

  //m24cxxx.cpp
  auto power(Type, n3 enableID = 0) -> void;
  auto read() -> n1;
  auto write(maybe<n1> clock, maybe<n1> data) -> void;
  auto erase(n8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8 memory[65536];
  n8 idpage[32];

private:
  //m24cxxx.cpp
  auto select() -> bool;
  auto load() -> bool;
  auto store() -> bool;

  static constexpr bool Acknowledge = 0;

  enum class Mode : u32 {
    Standby,
    Device,
    AddressUpper,
    AddressLower,
    Read,
    Write,
  };

  struct Line {
    //m24cxxx.cpp
    auto write(n1 data) -> void;

    n1 lo;
    n1 hi;
    n1 fall;
    n1 rise;
    n1 line;
  };

  Type type;
  Mode mode;
  Line clock;
  Line data;
  n3   enable;
  n8   counter;
  n8   device;
  n16  address;
  n8   input;
  n8   output;
  n1   response;
  n1   writable;
  n1   locked;
};

}
