#include <ares/ares.hpp>
#include "m24cx.hpp"

namespace ares {

#include "serialization.cpp"

auto M24Cx::power() -> void {
  mode     = Mode::Standby;
  clock    = {};
  data     = {};
  counter  = 0;
  address  = 0;
  input    = 0;
  output   = 0;
  response = Acknowledge;
  writable = 1;
}

auto M24Cx::read() -> n1 {
  if(mode == Mode::Standby) return data.line;
  return response;
}

auto M24Cx::write(maybe<n1> scl, maybe<n1> sda) -> void {
  auto phase = mode;
  clock.write(scl(clock.line));
  data.write(sda(data.line));

  if(clock.hi) {
    if(data.fall) counter = 0, mode = Mode::Address;
    if(data.rise) counter = 0, mode = Mode::Standby;
  }

  if(clock.fall) {
    if(counter++ > 8) counter = 1;
  }

  if(clock.rise)
  switch(phase) {
  case Mode::Address:
    if(counter <= 8) {
      address = address << 1 | data.line;
    } else if(address & 1) {
      mode = Mode::Read;
      response = load();
    } else {
      mode = Mode::Write;
      response = Acknowledge;
    }
    break;

  case Mode::Read:
    if(counter <= 8) {
      response = output >> 8 - counter;
    } else if(data.line == Acknowledge) {
      address += 2;
      response = load();
    } else {
      mode = Mode::Standby;
    }
    break;

  case Mode::Write:
    if(counter <= 8) {
      input = input << 1 | data.line;
    } else {
      response = store();
      address += 2;
    }
    break;
  }
}

auto M24Cx::load() -> bool {
  output = memory[address >> 1 & size() - 1];
  return Acknowledge;
}

auto M24Cx::store() -> bool {
  if(!writable) return !Acknowledge;
  memory[address >> 1 & size() - 1] = input;
  return Acknowledge;
}

auto M24Cx::erase(n8 fill) -> void {
  for(auto& byte : memory) byte = fill;
}

auto M24Cx::Line::write(n1 data) -> void {
  lo   = !line && !data;
  hi   =  line &&  data;
  fall =  line && !data;
  rise = !line &&  data;
  line =  data;
}

}
