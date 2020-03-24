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
  auto allocate(uint size, uint width, uint8 fill = 0xff) -> bool;
  auto program(uint11 address, uint8 data) -> void;
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
  uint8 data[2048];  //uint8 was chosen solely for easier serialization and saving
  uint size;         //in bytes
  uint width;        //8-bit (ORG=0) or 16-bit (ORG=1) configuration

  boolean writable;  //EWEN, EWDS
  uint busy;         //busy cycles in milliseconds remaining for programming (write) operations to complete

  struct ShiftRegister {
    auto flush() -> void;
    auto edge() -> uint1;
    auto read() -> uint1;
    auto write(uint1 data) -> void;

    uint32 value;
    uint32 count;
  };

  struct InputShiftRegister : ShiftRegister {
    auto start() -> maybe<uint1>;
    auto opcode() -> maybe<uint2>;
    auto mode() -> maybe<uint2>;
    auto address() -> maybe<uint11>;
    auto data() -> maybe<uint16>;
    auto increment() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint32 addressLength;
    uint32 dataLength;
  } input;

  struct OutputShiftRegister : ShiftRegister {
    //serialization.cpp
    auto serialize(serializer&) -> void;
  } output;
};

}
