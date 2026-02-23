#pragma once

namespace ares {

//Microchip 93LCx6
//  93LC46 =>  1024 cells =>  128 x 8-bit or   64 x 16-bit
//  93LC56 =>  2048 cells =>  256 x 8-bit or  128 x 16-bit
//  93LC66 =>  4096 cells =>  512 x 8-bit or  256 x 16-bit
//  93LC76 =>  8192 cells => 1024 x 8-bit or  512 x 16-bit
//  93LC86 => 16384 cells => 2048 x 8-bit or 1024 x 16-bit

struct M93LCx6 {
  //m93lcx6.cpp
  explicit operator bool() const;
  auto reset() -> void;
  auto allocate(u32 size, u32 width, bool endian, n8 fill) -> bool;
  auto program(n11 address, n8 data) -> void;
  auto clock() -> void;
  auto power() -> void;
  auto edge() -> void;

  //chip commands
  auto read() -> void;
  auto write() -> void;
  auto erase() -> void;
  auto writeAll() -> void;
  auto eraseAll() -> void;
  auto writeEnable() -> void;
  auto writeDisable() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //it is awkward no matter if data is uint1[bits], uint8[bytes], or uint16[words]
  n8   data[2048];  //uint8 was chosen solely for easier serialization and saving
  u32  size;        //in bytes
  u32  width;       //8-bit (ORG=0) or 16-bit (ORG=1) configuration
  bool endian;      //16-bit mode: 0 = little-endian; 1 = big-endian

  b1   writable;    //EWEN, EWDS
  u32  busy;        //busy cycles in milliseconds remaining for programming (write) operations to complete

  struct ShiftRegister {
    auto flush() -> void;
    auto edge() -> n1;
    auto read() -> n1;
    auto write(n1 data) -> void;

    n32 value;
    n32 count;
  };

  struct InputShiftRegister : ShiftRegister {
    auto start() -> maybe<n1>;
    auto opcode() -> maybe<n2>;
    auto mode() -> maybe<n2>;
    auto address() -> maybe<n11>;
    auto data() -> maybe<n16>;
    auto increment() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n32 addressLength;
    n32 dataLength;
  } input;

  struct OutputShiftRegister : ShiftRegister {
    //serialization.cpp
    auto serialize(serializer&) -> void;
  } output;
};

}
