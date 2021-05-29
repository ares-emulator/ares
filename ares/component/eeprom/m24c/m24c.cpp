#include <ares/ares.hpp>
#include "m24c.hpp"

namespace ares {

#include "serialization.cpp"

auto M24C::size() const -> u32 {
  switch(type) {
  default:            return     0;
  case Type::X24C01:  return   128;
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

auto M24C::reset() -> void {
  type = Type::None;
}

auto M24C::load(Type typeID, n3 enableID) -> void {
  type   = typeID;
  enable = enableID;
  erase();
}

auto M24C::power() -> void {
  mode     = Mode::Standby;
  clock    = {};
  data     = {};
  counter  = 0;
  device   = Area::Memory << 4;
  bank     = 0;
  address  = 0;
  input    = 0;
  output   = 0;
  response = Acknowledge;
  writable = 1;
}

auto M24C::read() const -> bool {
  if(mode == Mode::Standby) return data();
  return response;
}

auto M24C::write() -> void {
  auto phase = mode;

  if(clock.hi()) {
    if(data.fall()) {
      counter = 0;
      mode = (type == Type::X24C01) ? Mode::Address : Mode::Device;
    } else if(data.rise()) {
      counter = 0;
      mode = Mode::Standby;
    }
  }

  if(clock.fall()) {
    if(counter++ > 8) counter = 1;
  }

  if(clock.rise())
  switch(phase) {
  case Mode::Device:
    if(counter <= 8) {
      device = device << 1 | data();
    } else if(!select()) {
      mode = Mode::Standby;
    } else if(device & 1) {
      mode = Mode::Read;
      response = load();
    } else {
      mode = (type <= Type::M24C16) ? Mode::Address : Mode::Bank;
      response = Acknowledge;
    }
    break;

  case Mode::Bank:
    if(counter <= 8) {
      bank = bank << 1 | data();
    } else {
      mode = Mode::Address;
      response = Acknowledge;
    }
    break;

  case Mode::Address:
    if(counter <= 8) {
      address = address << 1 | data();
    } else if(type == Type::X24C01 && address & 1) {
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
    } else if(data() == Acknowledge) {
      address += (type == Type::X24C01) ? 2 : 1;
      if(!address) bank++;
      response = load();
    } else {
      mode = Mode::Standby;
    }
    break;

  case Mode::Write:
    if(counter <= 8) {
      input = input << 1 | data();
    } else {
      response = store();
      address += (type == Type::X24C01) ? 2 : 1;
      if(!address) bank++;
    }
    break;
  }
}

auto M24C::erase(u8 fill) -> void {
  for(auto& byte : memory) byte = fill;
  for(auto& byte : idpage) byte = fill;
  locked = 0;
}

//

auto M24C::select() const -> bool {
/*switch(type) {
  default:           if((device >> 1 & 7) != (enable & 7)) return !Acknowledge; break;
  case Type::M24C04: if((device >> 1 & 6) != (enable & 6)) return !Acknowledge; break;
  case Type::M24C08: if((device >> 1 & 4) != (enable & 4)) return !Acknowledge; break;
  case Type::M24C16: break;
  }*/

  switch(device >> 4) {
  case Area::Memory:
    return Acknowledge;

  case Area::IDPage:
    if(type <= Type::M24C16) return !Acknowledge;
    return Acknowledge;

  default:
    return !Acknowledge;
  }
}

auto M24C::offset() const -> u32 {
  if(type == Type::X24C01) return address >> 1;
  if(type <= Type::M24C16) return device >> 1 << 8 | address;
  return device >> 1 << 16 | bank << 8 | address;
}

auto M24C::load() -> bool {
  switch(device >> 4) {
  case Area::Memory:
    output = memory[offset() & size() - 1];
    return Acknowledge;

  case Area::IDPage:
    if(type <= Type::M24C16) return !Acknowledge;
    output = idpage[address & sizeof(idpage) - 1];
    return Acknowledge;

  default:
    return !Acknowledge;
  }
}

auto M24C::store() -> bool {
  switch(device >> 4) {
  case Area::Memory:
    if(!writable) return !Acknowledge;
    memory[offset() & size() - 1] = input;
    return Acknowledge;

  case Area::IDPage:
    if(!writable) return !Acknowledge;
    if(type <= Type::M24C16) return !Acknowledge;
    if(address.bit(10)) {
      locked |= input.bit(1);
      return Acknowledge;
    }
    if(locked) return !Acknowledge;
    idpage[address & sizeof(idpage) - 1] = input;
    return Acknowledge;

  default:
    return !Acknowledge;
  }
}

}
