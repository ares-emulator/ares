#include <ares/ares.hpp>
#include "x24c01.hpp"

namespace ares {

#include "serialization.cpp"

auto X24C01::power() -> void {
  clock   = {};
  data    = {};
  mode    = Mode::Idle;
  counter = 0;
  address = 0;
  input   = 0;
  output  = 0;
  line    = 1;
}

auto X24C01::read() -> n1 {
  return line;
}

auto X24C01::write(n1 scl, n1 sda) -> void {
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
    if(mode == Mode::Address && counter < addressBits()) {
      address = data.line << addressBits() - 1 | address >> 1;
      counter++;
      return;
    }

    if(mode == Mode::Read && counter < dataBits()) {
      line = output & 1;
      output >>= 1;
      counter++;
      return;
    }

    if(mode == Mode::Write && counter < dataBits()) {
      input = data.line << dataBits() - 1 | input >> 1;
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
    if(mode == Mode::Address && counter == addressBits()) {
      line = 1;
      mode = Mode::AddressAcknowledge;
      return;
    }

    if(mode == Mode::Read && counter == dataBits()) {
      mode = Mode::ReadAcknowledge;
      counter = 0;
      return;
    }

    if(mode == Mode::Write && counter == dataBits()) {
      bytes[address & size() - 1] = input;
      mode = Mode::WriteAcknowledge;
      return;
    }

    if(mode == Mode::AddressAcknowledge) {
      mode = address.bit(7) ? Mode::Read : Mode::Write;
      output = bytes[address & size() - 1];
      counter = 0;
      return;
    }

    if(mode == Mode::ReadAcknowledge) {
      mode = data.line ? Mode::Read : Mode::Idle;
      address.bit(0,6)++;
      return;
    }

    if(mode == Mode::WriteAcknowledge) {
      mode = data.line ? Mode::Write : Mode::Idle;
      address.bit(0,6)++;
      return;
    }
  }
}

auto X24C01::erase(n8 fill) -> void {
  memory::fill<u8>(bytes, size());
}

auto X24C01::Line::write(n1 data) -> void {
  lo   = !line && !data;
  hi   =  line &&  data;
  fall =  line && !data;
  rise = !line &&  data;
  line =  data;
}

}
