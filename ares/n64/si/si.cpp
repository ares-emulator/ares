#include <n64/n64.hpp>

namespace ares::Nintendo64 {

SI si;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto SI::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("SI");
  debugger.load(node);
}

auto SI::unload() -> void {
  node = {};
  debugger = {};
}

auto SI::main() -> void {
  //hack to allow the PIF IPLROM to complete:
  if(resetStrobe < 10) {
    resetStrobe++;
    auto data = pi.ram.readByte(0x3f);
    pi.ram.writeByte(0x3f, 0x80 | data);
  }

  ControllerPort* controllers[4] = {
    &controllerPort1,
    &controllerPort2,
    &controllerPort3,
    &controllerPort4,
  };

  static constexpr bool Debug = 0;

  if((pi.ram.readByte(0x3f) & 0x3f) != 0x01) return;

  if constexpr(Debug) {
    print("{\n");
    for(uint y : range(8)) {
      print("  ");
      for(uint x : range(8)) {
        print(hex(pi.ram.readByte(y * 8 + x), 2L), " ");
      }
      print("\n");
    }
    print("}\n");
  }

  uint3 channel = 0;
  for(uint offset = 0; offset < 64;) {
    u8 send = pi.ram.readByte(offset++) & 0x3f;
    if(send == 0x00) { channel++; continue; }
    if(send == 0x3f) continue;
    if(send == 0x3e) break;
    u8 recvOffset = offset;
    u8 recv = pi.ram.readByte(offset++) & 0x3f;
    uint8 input[256];
    for(uint index = 0; index < send; index++) {
      input[index] = pi.ram.readByte(offset++) & 0x3f;
    }
    uint8 output[256];
    bool valid = 0;
    if(input[0] == 0x00 || input[0] == 0x3f) {
      if(channel < 4 && send == 1 && recv == 3) {
        output[0] = 0x05;  //0x05 = gamepad; 0x02 = mouse
        output[1] = 0x00;
        output[2] = 0x01;
        valid = 1;
      }
    }
    if(input[0] == 0x01) {
      if(channel < 4 && controllers[channel]->device) {
        u32 data = controllers[channel]->device->read();
        for(uint index = 0; index < min(4, recv); index++) {
          output[index] = data >> 24;
          data <<= 8;
        }
        if(recv <= 4) {
          pi.ram.writeByte(recvOffset, 0x00 | recv & 0x3f);
        } else {
          pi.ram.writeByte(recvOffset, 0x40 | recv & 0x3f);
        }
        valid = 1;
      }
    }
    if(!valid) {
      pi.ram.writeByte(recvOffset, 0x80 | recv & 0x3f);
    }
    for(uint index = 0; index < recv; index++) {
      pi.ram.writeByte(offset++, output[index]);
    }
    channel++;
  }

//this is supposed to happen, but it breaks input (games don't set the bit back again)
//pi.ram.writeByte(0x3f, 0x00);

  if constexpr(Debug) {
    print("[\n");
    for(uint y : range(8)) {
      print("  ");
      for(uint x : range(8)) {
        print(hex(pi.ram.readByte(y * 8 + x), 2L), " ");
      }
      print("\n");
    }
    print("]\n");
  }
}

auto SI::power() -> void {
  resetStrobe = 0;
}

}
