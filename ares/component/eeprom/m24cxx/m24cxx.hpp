#pragma once

namespace ares {

struct M24Cxx {
  enum class Type : u32 {
    M24C01,   //  1024 cells =>   128 x 8-bit x 1-block
    M24C02,   //  2048 cells =>   256 x 8-bit x 1-block
    M24C04,   //  4096 cells =>   256 x 8-bit x 2-blocks
    M24C08,   //  8192 cells =>   256 x 8-bit x 4-blocks
    M24C16,   // 16384 cells =>   256 x 8-bit x 8-blocks
    M24C32,   // 32768 cells =>  4096 x 8-bit x 1-block
    M24C64,   // 65536 cells =>  8192 x 8-bit x 1-block
    M24C65,   // 65536 cells =>  8192 x 8-bit x 1-block
    M24C128,  //131072 cells => 16384 x 8-bit x 1-block
    M24C256,  //262144 cells => 32768 x 8-bit x 1-block
    M24C512,  //524288 cells => 65536 x 8-bit x 1-block
  };

  auto size() const -> u32 {
    switch(type) { default:
    case Type::M24C01:  return   128;
    case Type::M24C02:  return   256;
    case Type::M24C04:  return   512;
    case Type::M24C08:  return  1024;
    case Type::M24C16:  return  2048;
    case Type::M24C32:  return  4096;
    case Type::M24C64:  return  8192;
    case Type::M24C65:  return  8192;
    case Type::M24C128: return 16384;
    case Type::M24C256: return 32768;
    case Type::M24C512: return 65536;
    }
  }

  auto controlBits() const -> u32 {
    return 8;
  }

  auto addressBits() const -> u32 {
    switch(type) { default:
    case Type::M24C01:  return  8;
    case Type::M24C02:  return  8;
    case Type::M24C04:  return  8;
    case Type::M24C08:  return  8;
    case Type::M24C16:  return  8;
    case Type::M24C32:  return 16;
    case Type::M24C64:  return 16;
    case Type::M24C65:  return 16;
    case Type::M24C128: return 16;
    case Type::M24C256: return 16;
    case Type::M24C512: return 16;
    }
  }

  auto dataBits() const -> u32 {
    return 8;
  }

  auto block() const -> u32 {
    if(type == Type::M24C04) {
      return control.bit(6) << 8;
    }
    if(type == Type::M24C08) {
      return control.bit(6) << 8 | control.bit(5) << 9;
    }
    if(type == Type::M24C16) {
      return control.bit(6) << 8 | control.bit(5) << 9 | control.bit(4) << 10;
    }
    return 0;
  }

  //m24cxx.cpp
  auto power(Type) -> void;
  auto read() -> n1;
  auto write(n1 clock, n1 data) -> void;
  auto erase(n8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8 bytes[65536];

private:
  enum class Mode : u32 {
    Idle,
    Control,
    ControlAcknowledge,
    Address,
    AddressAcknowledge,
    Read,
    ReadAcknowledge,
    Write,
    WriteAcknowledge,
  };

  struct Line {
    //m24cxx.cpp
    auto write(n1 data) -> void;

    n1 lo;
    n1 hi;
    n1 fall;
    n1 rise;
    n1 line;
  };

  Type type;
  Line clock;
  Line data;
  Mode mode;
  n8   counter;
  n8   control;
  n16  address;
  n8   input;
  n8   output;
  n1   line;
};

}
