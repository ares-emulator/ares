#include <ws/ws.hpp>

namespace ares::WonderSwan {

#include "serialization.cpp"

auto EEPROM::power() -> void {
  M93LCx6::power();
  io = {};
  io.ready = 1;
}

auto EEPROM::read(u32 port) -> n8 {
  n8 data;
  if(!size) return data = 0xff;

  if(port == DataLo) return io.read.byte(0);
  if(port == DataHi) return io.read.byte(1);

  if(port == CommandLo) return io.command.byte(0);
  if(port == CommandHi) return io.command.byte(1);

  if(port == Status) {
    data.bit(0) = io.readComplete;
    data.bit(1) = io.ready;
    data.bit(4,7) = io.control;
    return data;
  }

  return data;
}

auto EEPROM::write(u32 port, n8 data) -> void {
  if(!size) return;

  if(port == DataLo) {
    io.write.byte(0) = data;
    return;
  }

  if(port == DataHi) {
    io.write.byte(1) = data;
    return;
  }

  if(port == CommandLo) {
    io.command.byte(0) = data;
    return;
  }

  if(port == CommandHi) {
    io.command.byte(1) = data;
    return;
  }

  if(port == Control) {
    io.control = data.bit(4,7);
    n16 command = padCommand(io.command);

    if(io.control == 1) { // read
      for(u32 index : reverse(range(16))) input.write(command.bit(index));
      edge();
      output.read();  //padding bit
      for(u32 index : reverse(range(16))) io.read.bit(index) = output.read();
      io.readComplete = 1;
    } else if(io.control == 2 && canWrite(command)) { // write
      for(u32 index : reverse(range(16))) input.write(command.bit(index));
      for(u32 index : reverse(range(16))) input.write(io.write.bit(index));
      edge();
    } else if(io.control == 4 && canWrite(command)) { // erase/other
      for(u32 index : reverse(range(16))) input.write(command.bit(index));
      edge();
    } else if (io.control == 8) { // protect (only used on internal EEPROM)
      io.protect = 1;
    }

    io.control = 0;
    io.ready = 1;

    input.flush();
    output.flush();
    return;
  }
}

auto EEPROM::canWrite(n16 command) -> bool {
  return true;
}

auto EEPROM::padCommand(n16 command) -> n16 {
  return command;
}

auto InternalEEPROM::read(u32 port) -> n8 {
  n8 data = EEPROM::read(port);
  if(port == Status) data.bit(7) |= io.protect;
  return data;
}

auto InternalEEPROM::canWrite(n16 command) -> bool {
  // TODO: This is a guess that gets the basic cases (read, write, write lock/unlock) correct.
  // More testing is required for full accuracy.
  if(!io.protect) return true;
  if(SoC::ASWAN()) return !(command & 0xC0) || ((command & 0x3F) < 0x30);
  return !(command & 0xC00) || ((command & 0x3FF) < 0x30);
}

auto InternalEEPROM::padCommand(n16 command) -> n16 {
  if(!SoC::ASWAN() && !system.color()) {
    // Emulate 93C46 EEPROM on 93C86.
    // TODO: This is a guess that gets the basic cases (read, write, write lock/unlock) correct.
    // More testing is required for full accuracy.
    if(command & 0xC0) {
      return ((command & 0xFFC0) << 4) | (command & 0x3F);
    } else {
      return (command << 4);
    }
  }
  
  return command;
}

}
