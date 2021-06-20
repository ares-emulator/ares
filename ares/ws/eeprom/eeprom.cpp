#include <ws/ws.hpp>

namespace ares::WonderSwan {

#include "serialization.cpp"

auto EEPROM::power() -> void {
  M93LCx6::power();
  io = {};
}

auto EEPROM::read(u32 port) -> n8 {
  n8 data;
  if(!size) return data = 0xff;

  if(port == DataLo) return io.data.byte(0);
  if(port == DataHi) return io.data.byte(1);

  if(port == AddressLo) return io.address.byte(0);
  if(port == AddressHi) return io.address.byte(1);

  if(port == Status) {
    data.bit(0) = io.readReady;
    data.bit(1) = io.writeReady;
    data.bit(2) = io.eraseReady;
    data.bit(3) = io.resetReady;
    data.bit(4) = io.readPending;
    data.bit(5) = io.writePending;
    data.bit(6) = io.erasePending;
    data.bit(7) = io.resetPending;
    return data;
  }

  return data;
}

auto EEPROM::write(u32 port, n8 data) -> void {
  if(!size) return;

  if(port == DataLo) {
    io.data.byte(0) = data;
    return;
  }

  if(port == DataHi) {
    io.data.byte(1) = data;
    return;
  }

  if(port == AddressLo) {
    io.address.byte(0) = data;
    return;
  }

  if(port == AddressHi) {
    io.address.byte(1) = data;
    return;
  }

  if(port == Command) {
    io.readPending  = data.bit(4);
    io.writePending = data.bit(5);
    io.erasePending = data.bit(6);
    io.resetPending = data.bit(7);

    //nothing happens unless only one bit is set.
    if(bit::count(data.bit(4,7)) != 1) return;

    if(io.resetPending) {
      M93LCx6::power();
      io.resetPending = 0;
      return;
    }

    //start bit + command bits + address bits
    for(u32 index : reverse(range(1 + 2 + input.addressLength))) input.write(io.address.bit(index));

    if(io.readPending) {
      edge();
      output.read();  //padding bit
      for(u32 index : reverse(range(input.dataLength))) io.data.bit(index) = output.read();
      io.readPending = 0;
    }

    if(io.writePending) {
      for(u32 index : reverse(range(input.dataLength))) input.write(io.data.bit(index));
      edge();
      io.writePending = 0;
    }

    if(io.erasePending) {
      edge();
      io.erasePending = 0;
    }

    input.flush();
    output.flush();
    return;
  }
}

}
