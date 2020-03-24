#include <ws/ws.hpp>

namespace ares::WonderSwan {

#include "serialization.cpp"

auto EEPROM::power() -> void {
  M93LCx6::power();
  r = {};
}

auto EEPROM::read(uint port) -> uint8 {
  uint8 data;
  if(!size) return data = 0xff;

  if(port == DataLo) return r.data.byte(0);
  if(port == DataHi) return r.data.byte(1);

  if(port == AddressLo) return r.address.byte(0);
  if(port == AddressHi) return r.address.byte(1);

  if(port == Status) {
    data.bit(0) = r.readReady;
    data.bit(1) = r.writeReady;
    data.bit(2) = r.eraseReady;
    data.bit(3) = r.resetReady;
    data.bit(4) = r.readPending;
    data.bit(5) = r.writePending;
    data.bit(6) = r.erasePending;
    data.bit(7) = r.resetPending;
    return data;
  }

  return data;
}

auto EEPROM::write(uint port, uint8 data) -> void {
  if(!size) return;

  if(port == DataLo) {
    r.data.byte(0) = data;
    return;
  }

  if(port == DataHi) {
    r.data.byte(1) = data;
    return;
  }

  if(port == AddressLo) {
    r.address.byte(0) = data;
    return;
  }

  if(port == AddressHi) {
    r.address.byte(1) = data;
    return;
  }

  if(port == Command) {
    r.readPending  = data.bit(4);
    r.writePending = data.bit(5);
    r.erasePending = data.bit(6);
    r.resetPending = data.bit(7);

    //nothing happens unless only one bit is set.
    if(bit::count(data.bit(4,7)) != 1) return;

    if(r.resetPending) {
      M93LCx6::power();
      r.resetPending = 0;
      return;
    }

    //start bit + command bits + address bits
    for(uint index : reverse(range(1 + 2 + input.addressLength))) input.write(r.address.bit(index));

    if(r.readPending) {
      edge();
      output.read();  //padding bit
      for(uint index : reverse(range(input.dataLength))) r.data.bit(index) = output.read();
      r.readPending = 0;
    }

    if(r.writePending) {
      for(uint index : reverse(range(input.dataLength))) input.write(r.data.bit(index));
      edge();
      r.writePending = 0;
    }

    if(r.erasePending) {
      edge();
      r.erasePending = 0;
    }

    input.flush();
    output.flush();
    return;
  }
}

}
