#include <ares/ares.hpp>
#include "m93lcx6.hpp"

namespace ares {

#include "serialization.cpp"

M93LCx6::operator bool() const {
  return size;
}

auto M93LCx6::reset() -> void {
  size = 0;
}

auto M93LCx6::allocate(uint size, uint width, uint8 fill) -> bool {
  if(size != 128 && size != 256 && size != 512 && size != 1024 && size != 2048) return false;
  if(width != 8 && width != 16) return false;

  this->size  = size;
  this->width = width;

  for(auto& byte : data) byte = fill;

  input.addressLength = 0;
  input.dataLength = width;

  //note: the 93LC56 and 93LC76 have an extra dummy address bit
  if(size ==  128) input.addressLength = width == 16 ?  6 :  7;  //93LC46
  if(size ==  256) input.addressLength = width == 16 ?  8 :  9;  //93LC56
  if(size ==  512) input.addressLength = width == 16 ?  8 :  9;  //93LC66
  if(size == 1024) input.addressLength = width == 16 ? 10 : 11;  //93LC76
  if(size == 2048) input.addressLength = width == 16 ? 10 : 11;  //93LC86

  return true;
}

//used externally to program initial state values for uninitialized EEPROMs
auto M93LCx6::program(uint11 address, uint8 value) -> void {
  data[address] = value;
}

//1ms frequency
auto M93LCx6::clock() -> void {
  //set during programming commands
  if(busy) busy--;
}

auto M93LCx6::power() -> void {
//writable = 0;
  busy = 0;

  input.flush();
  output.flush();
}

auto M93LCx6::edge() -> void {
  //wait for start
  if(!input.start()) return;
  uint start = *input.start();

  //start bit must be set
  if(start != 1) return input.flush();

  //wait for opcode
  if(!input.opcode()) return;
  uint opcode = *input.opcode();

  //wait for address
  if(!input.address()) return;

  if(opcode == 0b00) {
    auto mode = *input.mode();
    if(mode == 0b00) return writeDisable();
    if(mode == 0b01) return writeAll();
    if(mode == 0b10) return eraseAll();
    if(mode == 0b11) return writeEnable();
  }
  if(opcode == 0b01) return write();
  if(opcode == 0b10) return read();
  if(opcode == 0b11) return erase();
}

//

auto M93LCx6::read() -> void {
  uint address = *input.address() << (width == 16) & size - 1;
  output.flush();
  for(uint4 index : range(width)) {
    output.write(data[address + index.bit(3)].bit(index.bit(0,2)));
  }
  output.write(0);  //reads have an extra dummy data bit
}

auto M93LCx6::write() -> void {
  if(!input.data()) return;  //wait for data
  if(!writable) return input.flush();
  uint address = *input.address() << (width == 16) & size - 1;
  for(uint4 index : range(width)) {
    data[address + index.bit(3)].bit(index.bit(0,2)) = input.read();
  }
  busy = 4;  //milliseconds
  return input.flush();
}

auto M93LCx6::erase() -> void {
  if(!writable) return input.flush();
  uint address = *input.address() << (width == 16) & size - 1;
  for(uint4 index : range(width)) {
    data[address + index.bit(3)].bit(index.bit(0,2)) = 1;
  }
  busy = 4;  //milliseconds
  return input.flush();
}

auto M93LCx6::writeAll() -> void {
  if(!input.data()) return;  //wait for data
  if(!writable) return input.flush();
  auto word = *input.data();
  for(uint address = 0; address < 512;) {
    data[address++] = uint8(word >> (width == 16 ? 8 : 0));
    data[address++] = uint8(word >> 0);
  }
  busy = 16;  //milliseconds
  return input.flush();
}

auto M93LCx6::eraseAll() -> void {
  if(!writable) return input.flush();
  for(auto& byte : data) byte = 0xff;
  busy = 8;  //milliseconds
  return input.flush();
}

auto M93LCx6::writeEnable() -> void {
  writable = true;
  return input.flush();
}

auto M93LCx6::writeDisable() -> void {
  writable = false;
  return input.flush();
}

//

auto M93LCx6::ShiftRegister::flush() -> void {
  value = 0;
  count = 0;
}

auto M93LCx6::ShiftRegister::edge() -> uint1 {
  return value.bit(0);
}

auto M93LCx6::ShiftRegister::read() -> uint1 {
  uint1 bit = value.bit(0);
  value >>= 1;
  count--;
  return bit;
}

auto M93LCx6::ShiftRegister::write(uint1 bit) -> void {
  value <<= 1;
  value.bit(0) = bit;
  count++;
}

//

auto M93LCx6::InputShiftRegister::start() -> maybe<uint1> {
  if(count < 1) return {};
  return {value >> count - 1 & 1};
}

auto M93LCx6::InputShiftRegister::opcode() -> maybe<uint2> {
  if(count < 1 + 2) return {};
  return {value >> count - 3 & 3};
}

auto M93LCx6::InputShiftRegister::mode() -> maybe<uint2> {
  if(count < 1 + 2 + addressLength) return {};
  return {value >> count - 5 & 3};
}

auto M93LCx6::InputShiftRegister::address() -> maybe<uint11> {
  if(count < 1 + 2 + addressLength) return {};
  return {value >> count - (3 + addressLength) & (1 << addressLength) - 1};
}

auto M93LCx6::InputShiftRegister::data() -> maybe<uint16> {
  if(count < 1 + 2 + addressLength + dataLength) return {};
  return {value >> count - (3 + addressLength + dataLength) & (1 << dataLength) - 1};
}

auto M93LCx6::InputShiftRegister::increment() -> void {
  value.bit(0, addressLength - 1)++;
}

}
