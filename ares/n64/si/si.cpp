#include <n64/n64.hpp>

namespace ares::Nintendo64 {

SI si;
#include "io.cpp"
#include "serialization.cpp"

auto SI::main() -> void {
  ControllerPort* controllers[4] = {
    &controllerPort1,
    &controllerPort2,
    &controllerPort3,
    &controllerPort4,
  };

  uint3 channel = 0;
  if(pi.ram.readByte(0x3f) == 0x01) {
    for(uint offset = 0; offset < 64;) {
      u8 send = pi.ram.readByte(offset++);
      if(send == 0x00) { channel++; continue; }
      if(send == 0xff) continue;
      if(send == 0xfe) break;
      u8 recvOffset = offset;
      u8 recv = pi.ram.readByte(offset++);
      uint8 input[256];
      for(uint index = 0; index < send; index++) {
        input[index] = pi.ram.readByte(offset++);
      }
      uint8 output[256];
      if(input[0] == 0x01) {
        if(channel < 4 && controllers[channel]->device) {
          u32 data = controllers[channel]->device->read();
          for(uint index = 0; index < recv; index++) {
            output[index] = data >> 24;
            data <<= 8;
          }
          pi.ram.writeByte(recvOffset, 0x00 | recv & 0x3f);
        } else {
          pi.ram.writeByte(recvOffset, 0x80 | recv & 0x3f);
        }
      } else {
        pi.ram.writeByte(recvOffset, 0x80 | recv & 0x3f);
      }
      for(uint index = 0; index < recv; index++) {
        pi.ram.writeByte(offset++, output[index]);
      }
      channel++;
    }
  //pi.ram.writeByte(0x3f, 0x00);
  }
}

auto SI::power() -> void {
}

}
