#include <n64/n64.hpp>

namespace ares::Nintendo64 {

SI si;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto SI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("SI");
  debugger.load(node);
}

auto SI::unload() -> void {
  debugger = {};
  node.reset();
}

auto SI::addressCRC(u16 address) const -> n5 {
  n5 crc = 0;
  for(u32 i : range(16)) {
    n5 xor = crc & 0x10 ? 0x15 : 0x00;
    crc <<= 1;
    if(address & 0x8000) crc |= 1;
    address <<= 1;
    crc ^= xor;
  }
  return crc;
}

auto SI::dataCRC(array_view<u8> data) const -> n8 {
  n8 crc = 0;
  for(u32 i : range(33)) {
    for(u32 j : reverse(range(8))) {
      n8 xor = crc & 0x80 ? 0x85 : 0x00;
      crc <<= 1;
      if(i < 32) {
        if(data[i] & 1 << j) crc |= 1;
      }
      crc ^= xor;
    }
  }
  return crc;
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
    for(u32 y : range(8)) {
      print("  ");
      for(u32 x : range(8)) {
        print(hex(pi.ram.readByte(y * 8 + x), 2L), " ");
      }
      print("\n");
    }
    print("}\n");
  }

  n3 channel = 0;
  for(u32 offset = 0; offset < 64;) {
    n6 send = pi.ram.readByte(offset++);
    if(send == 0x00) { channel++; continue; }
    if(send == 0x3e) break;
    if(send == 0x3f) continue;
    n8 recvOffset = offset;
    n6 recv = pi.ram.readByte(offset++);
    n8 input[256];
    for(u32 index : range(send)) {
      input[index] = pi.ram.readByte(offset++);
    }
    n8 output[256];
    b1 valid = 0;
    if(input[0] == 0x00 || input[0] == 0xff) {
      //status
      if(channel < 4 && send == 1 && recv == 3) {
        output[0] = 0x05;  //0x05 = gamepad; 0x02 = mouse
        output[1] = 0x00;
        output[2] = 0x02;  //0x02 = nothing present in controller slot
        if(auto& device = controllers[channel]->device) {
          if(auto gamepad = dynamic_cast<Gamepad*>(device.data())) {
            if(gamepad->ram || gamepad->motor) {
              output[2] = 0x01;  //0x01 = pak present
            }
          }
        }
        valid = 1;
      }
      if(channel >= 4 && send == 1 && recv == 3 && cartridge.eeprom) {
        output[0] = 0x00;
        output[1] = 0x80;
        output[2] = 0x00;
        valid = 1;
      }
    }
    if(input[0] == 0x01) {
      //read controller state
      if(channel < 4 && controllers[channel]->device) {
        u32 data = controllers[channel]->device->read();
        for(u32 index = 0; index < min(4, recv); index++) {
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
    if(input[0] == 0x02) {
      //read memory pak
      if(send == 3 && recv == 33) {
        if(auto& device = controllers[channel]->device) {
          if(auto gamepad = dynamic_cast<Gamepad*>(device.data())) {
            if(auto& ram = gamepad->ram) {
              u32 address = (input[1] << 8 | input[2] << 0) & ~31;
              if(addressCRC(address) == (n5)input[2]) {
                for(u32 index : range(32)) {
                  if(address >> 15 == 0) output[index] = ram.readByte(address++);
                  if(address >> 15 == 1) output[index] = 0x00;
                }
                output[32] = dataCRC({&output[0], 32});
                valid = 1;
              }
            }
            if(gamepad->motor) {
              u32 address = (input[1] << 8 | input[2] << 0) & ~31;
              if(addressCRC(address) == (n5)input[2]) {
                for(u32 index : range(32)) {
                  output[index] = 0x80;
                }
                output[32] = dataCRC({&output[0], 32});
                valid = 1;
              }
            }
          }
        }
      }
    }
    if(input[0] == 0x03) {
      //write memory pak
      if(recv == 1 && send == 35) {
        if(auto& device = controllers[channel]->device) {
          if(auto gamepad = dynamic_cast<Gamepad*>(device.data())) {
            if(auto& ram = gamepad->ram) {
              u32 address = (input[1] << 8 | input[2] << 0) & ~31;
              if(addressCRC(address) == (n5)input[2]) {
                for(u32 index : range(32)) {
                  if(address >> 15 == 0) ram.writeByte(address++, input[3 + index]);
                }
                output[0] = dataCRC({&input[3], 32});
                valid = 1;
              }
            }
            if(gamepad->motor) {
              u32 address = (input[1] << 8 | input[2] << 0) & ~31;
              if(addressCRC(address) == (n5)input[2]) {
                output[0] = dataCRC({&input[3], 32});
                valid = 1;
                gamepad->rumble(input[3] & 1);
              }
            }
          }
        }
      }
    }
    if(input[0] == 0x04) {
      //read EEPROM
      if(cartridge.eeprom && send == 2 && recv == 8) {
        u32 address = input[1] * 8;
        for(u32 index : range(8)) {
          output[index] = cartridge.eeprom.readByte(address++);
        }
        valid = 1;
      }
    }
    if(input[0] == 0x05) {
      //write EEPROM
      if(cartridge.eeprom && recv == 1 && send == 10) {
        u32 address = input[1] * 8;
        for(u32 index : range(8)) {
          cartridge.eeprom.writeByte(address++, input[2 + index]);
        }
        output[0] = 0x00;
        valid = 1;
      }
    }
    if(!valid) {
      pi.ram.writeByte(recvOffset, 0x80 | recv & 0x3f);
    }
    for(u32 index : range(recv)) {
      pi.ram.writeByte(offset++, output[index]);
    }
    channel++;
  }

//this is supposed to happen, but it breaks input (games don't set the bit back again)
//pi.ram.writeByte(0x3f, 0x00);

  if constexpr(Debug) {
    print("[\n");
    for(u32 y : range(8)) {
      print("  ");
      for(u32 x : range(8)) {
        print(hex(pi.ram.readByte(y * 8 + x), 2L), " ");
      }
      print("\n");
    }
    print("]\n");
  }
}

auto SI::power(bool reset) -> void {
  io = {};
  resetStrobe = 0;
}

}
