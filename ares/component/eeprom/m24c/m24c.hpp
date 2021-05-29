#pragma once

namespace ares {

struct M24C {
  enum class Type : u32 {
    None,

    //8-bit address
    X24C01,   //   128 cells =>   128 x 8-bit x 1-block

    //8-bit device + 8-bit address
    M24C01,   //  1024 cells =>   128 x 8-bit x 1-block
    M24C02,   //  2048 cells =>   256 x 8-bit x 1-block
    M24C04,   //  4096 cells =>   256 x 8-bit x 2-blocks
    M24C08,   //  8192 cells =>   256 x 8-bit x 4-blocks
    M24C16,   // 16384 cells =>   256 x 8-bit x 8-blocks

    //8-bit device + 8-bit bank + 8-bit address
    M24C32,   // 32768 cells =>  4096 x 8-bit x 1-block
    M24C64,   // 65536 cells =>  8192 x 8-bit x 1-block
    M24C65,   // 65536 cells =>  8192 x 8-bit x 1-block
    M24C128,  //131072 cells => 16384 x 8-bit x 1-block
    M24C256,  //262144 cells => 32768 x 8-bit x 1-block
    M24C512,  //524288 cells => 65536 x 8-bit x 1-block
  };

  enum class Mode : u32 {
    Standby,
    Device,
    Bank,
    Address,
    Read,
    Write,
  };

  enum Area : u32 {
    Memory = 0b1010,
    IDPage = 0b1011,
  };

  explicit operator bool() const { return size() != 0; }

  //m24c.cpp
  auto size() const -> u32;
  auto reset() -> void;
  auto load(Type typeID, n3 enableID = 0) -> void;
  auto power() -> void;
  auto read() const -> bool;
  auto write() -> void;
  auto erase(u8 fill = 0xff) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Line {
    auto lo()   const -> bool { return !latch && !value; }
    auto hi()   const -> bool { return  latch &&  value; }
    auto fall() const -> bool { return  latch && !value; }
    auto rise() const -> bool { return !latch &&  value; }

    auto operator()() const -> bool {
      return value;
    }

    auto operator=(bool data) -> Line& {
      latch = value;
      value = data;
      return *this;
    }

    n1 latch = 1;
    n1 value = 1;
  };

  Line clock;     //SCL
  Line data;      //SDA
  n1   writable;  //!WP

  n8   memory[65536];
  n8   idpage[32];
  n1   locked;

private:
  //m24c.cpp
  auto select() const -> bool;
  auto offset() const -> u32;
  auto load() -> bool;
  auto store() -> bool;

  static constexpr bool Acknowledge = 0;

  Type type = Type::None;
  Mode mode = Mode::Standby;
  n3   enable;
  n8   counter;
  n8   device;
  n8   bank;
  n8   address;
  n8   input;
  n8   output;
  n1   response;
};

}
