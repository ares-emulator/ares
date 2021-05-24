#include <ares/ares.hpp>
#include "m24cxx.hpp"

namespace ares {

#include "serialization.cpp"

auto M24Cxx::power(Type typeID) -> void {
  type    = typeID;
  clock   = {};
  data    = {};
  mode    = Mode::Idle;
  counter = 0;
  control = 0;
  address = 0;
  input   = 0;
  output  = 0;
  line    = 1;
}

auto M24Cxx::read() -> n1 {
  return line;
}

auto M24Cxx::write(n1 scl, n1 sda) -> void {
  clock.write(scl);
  data.write(sda);

  if(clock.hi && data.fall) {
    line = 1;
    mode = Mode::Control;
    counter = 0;
    return;
  }

  if(clock.hi && data.rise) {
    line = 1;
    mode = Mode::Idle;
    return;
  }

  if(clock.rise) {
    if(mode == Mode::Control && counter < controlBits()) {
      control = data.line << controlBits() - 1 | control >> 1;
      counter++;
      return;
    }

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

    if(mode == Mode::ControlAcknowledge) {
      line = 0;
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
    if(mode == Mode::Control && counter == controlBits()) {
      line = 1;
      mode = Mode::ControlAcknowledge;
      if(control.bit(0,3) != 0b0101) {
        mode = Mode::Idle;
        debug(unusual, "[M24Cxx::write] deviceTypeID mismatch");
      }
      return;
    }

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
      bytes[(block() | address) & size() - 1] = input;
      mode = Mode::WriteAcknowledge;
      return;
    }

    if(mode == Mode::ControlAcknowledge) {
      mode = Mode::Address;
      counter = 0;
      return;
    }

    if(mode == Mode::AddressAcknowledge) {
      mode = control.bit(7) ? Mode::Read : Mode::Write;
      output = bytes[(block() | address) & size() - 1];
      counter = 0;
      return;
    }

    if(mode == Mode::ReadAcknowledge) {
      mode = data.line ? Mode::Read : Mode::Idle;
      address++;
      return;
    }

    if(mode == Mode::WriteAcknowledge) {
      mode = data.line ? Mode::Write : Mode::Idle;
      address++;
      return;
    }
  }
}

auto M24Cxx::erase(n8 fill) -> void {
  memory::fill<u8>(bytes, size());
}

auto M24Cxx::Line::write(n1 data) -> void {
  lo   = !line && !data;
  hi   =  line &&  data;
  fall =  line && !data;
  rise = !line &&  data;
  line =  data;
}

}
