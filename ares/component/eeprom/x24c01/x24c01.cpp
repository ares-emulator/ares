#include <ares/ares.hpp>
#include "x24c01.hpp"

namespace ares {

#include "serialization.cpp"

auto X24C01::reset() -> void {
  clock = {};
  data = {};
  mode = Mode::Idle;
  counter = 0;
  address = 0;
  input = 0;
  output = 0;
  line = 1;
}

auto X24C01::read() -> uint1 {
  return line;
}

auto X24C01::write(uint1 scl, uint1 sda) -> void {
  clock.write(scl);
  data.write(sda);

  if(clock.hi && data.fall) {
    line = 1;
    mode = Mode::Address;
    counter = 0;
    return;
  }

  if(clock.hi && data.rise) {
    line = 1;
    mode = Mode::Idle;
    return;
  }

  if(clock.rise) {
    if(mode == Mode::Address && counter < 8) {
      address = data.line << 7 | address >> 1;
      counter++;
      return;
    }

    if(mode == Mode::Read && counter < 8) {
      line = output & 1;
      output >>= 1;
      counter++;
      return;
    }

    if(mode == Mode::Write && counter < 8) {
      input = data.line << 7 | input >> 1;
      counter++;
      return;
    }

    if(mode == Mode::AddressAcknowledge) {
      line = 0;
      return;
    }

    if(mode == Mode::ReadAcknowledge) {
      line = 0;
      return;
    }

    if(mode == Mode::WriteAcknowledge) {
      line = 0;
      return;
    }
  }

  if(clock.fall) {
    if(mode == Mode::Address && counter == 8) {
      line = 1;
      mode = Mode::AddressAcknowledge;
      return;
    }

    if(mode == Mode::Read && counter == 8) {
      mode = Mode::ReadAcknowledge;
      address.bit(1,7)++;
      counter = 0;
      return;
    }

    if(mode == Mode::Write && counter == 8) {
      memory[address & 0x7f] = input;
      mode = Mode::WriteAcknowledge;
      return;
    }

    if(mode == Mode::AddressAcknowledge) {
      mode = address & 0x80 ? Mode::Read : Mode::Write;
      output = memory[address & 0x7f];
      counter = 0;
      return;
    }

    if(mode == Mode::ReadAcknowledge) {
      mode = Mode::Read;
      if(!data.line) mode = Mode::Idle;
      return;
    }

    if(mode == Mode::WriteAcknowledge) {
      mode = Mode::Idle;
      return;
    }
  }
}

auto X24C01::erase(uint8 fill) -> void {
  for(auto& byte : memory) byte = fill;
}

auto X24C01::Line::write(uint1 data) -> void {
  lo   = !line && !data;
  hi   =  line &&  data;
  fall =  line && !data;
  rise = !line &&  data;
  line =  data;
}

}
