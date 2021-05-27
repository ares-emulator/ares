#include <ares/ares.hpp>
#include "m24cxxx.hpp"

namespace ares {

#include "serialization.cpp"

auto M24Cxxx::power(Type typeID, n3 enableID) -> void {
  type     = typeID;
  mode     = Mode::Standby;
  clock    = {};
  data     = {};
  enable   = enableID;
  counter  = 0;
  device   = 0;
  address  = 0;
  input    = 0;
  output   = 0;
  response = Acknowledge;
  writable = 1;
  locked   = 0;
}

auto M24Cxxx::read() -> n1 {
  if(mode == Mode::Standby) return data.line;
  return response;
}

auto M24Cxxx::write(maybe<n1> scl, maybe<n1> sda) -> void {
  auto phase = mode;
  clock.write(scl(clock.line));
  data.write(sda(data.line));

  if(clock.hi) {
    if(data.fall) counter = 0, mode = Mode::Device;
    if(data.rise) counter = 0, mode = Mode::Standby;
  }

  if(clock.fall) {
    if(counter++ > 8) counter = 1;
  }

  if(clock.rise)
  switch(phase) {
  case Mode::Device:
    if(counter <= 8) {
      device = device << 1 | data.line;
    } else if(!select()) {
      mode = Mode::Standby;
    } else if(device & 1) {
      mode = Mode::Read;
      response = load();
    } else {
      mode = Mode::AddressUpper;
      response = Acknowledge;
    }
    break;

  case Mode::AddressUpper:
    if(counter <= 8) {
      address = address << 1 | data.line;
    } else {
      mode = Mode::AddressLower;
      response = Acknowledge;
    }
    break;

  case Mode::AddressLower:
    if(counter <= 8) {
      address = address << 1 | data.line;
    } else {
      mode = Mode::Write;
      response = Acknowledge;
    }
    break;

  case Mode::Read:
    if(counter <= 8) {
      response = output >> 8 - counter;
    } else if(data.line == Acknowledge) {
      address++;
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
      address++;
    }
    break;
  }
}

auto M24Cxxx::select() -> bool {
  if(device >> 4 != 0b1010 && device >> 4 != 0b1011) return !Acknowledge;
//if((device >> 1 & 0b111) != enable) return !Acknowledge;
  return Acknowledge;
}

auto M24Cxxx::load() -> bool {
  switch(device >> 4) {
  case 0b1010:
    output = memory[address & size() - 1];
    return Acknowledge;
  case 0b1011:
    output = idpage[address & sizeof(idpage) - 1];
    return Acknowledge;
  }
  return !Acknowledge;
}

auto M24Cxxx::store() -> bool {
  switch(device >> 4) {
  case 0b1010:
    if(!writable) return !Acknowledge;
    memory[address & size() - 1] = input;
    return Acknowledge;
  case 0b1011:
    if(!writable) return !Acknowledge;
    if(address.bit(10)) {
      locked |= input.bit(1);
      return Acknowledge;
    } else if(!locked) {
      idpage[address & sizeof(idpage) - 1] = input;
      return Acknowledge;
    } else {
      return !Acknowledge;
    }
  }
  return !Acknowledge;
}

auto M24Cxxx::erase(n8 fill) -> void {
  for(auto& byte : memory) byte = fill;
}

auto M24Cxxx::Line::write(n1 data) -> void {
  lo   = !line && !data;
  hi   =  line &&  data;
  fall =  line && !data;
  rise = !line &&  data;
  line =  data;
}

}
