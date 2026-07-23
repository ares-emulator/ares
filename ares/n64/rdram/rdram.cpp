#include <n64/n64.hpp>

namespace ares::Nintendo64 {

RDRAM rdram;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto RDRAM::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("RDRAM");

  if(!system.expansionPak) {
    ram.allocate(4_MiB);
  } else {
    ram.allocate(4_MiB + 4_MiB);
  }

  debugger.load(node);
}

auto RDRAM::unload() -> void {
  debugger = {};
  ram.reset();
  node.reset();
}

auto RDRAM::power(bool reset) -> void {
  if(!reset) {
    ram.fill();
    u32 count = system.expansionPak ? 4 : 2;
    for(u32 n : range(4)) {
      auto& chip = chips[n];
      chip = {};
      chip.present = n < count;
      if(!chip.present) continue;
      chip.deviceType = 0xb419'0010;
      chip.deviceManufacturer = 0x0000'0500;
      chip.delay = 0x0000'0023;  //WriteDelay=4, WriteBits=3
      chip.writeDelay = 4;
      chip.deviceID = 0;
      chip.deviceIDReg = 0;
      chip.ccLow = 8 + (random() & 3);
      chip.ccHigh = 14 + (random() & 3);
      if(chip.ccHigh <= chip.ccLow) chip.ccHigh = chip.ccLow + 1;
    }
    mapIdentity = 0;
  }
  profile = {};
}

auto RDRAM::decodeDeviceID(u32 value) -> u16 {
  return (value >> 26 & 0x3f)
       | (value >> 23 & 1) << 6
       | (value >>  8 & 0xff) << 7
       | (value >>  7 & 1) << 15;
}

auto RDRAM::decodeCCI(u32 mode_) -> n6 {
  n32 mode = mode_;
  n6 cci = 0;
  cci.bit(0) = mode.bit( 6);
  cci.bit(1) = mode.bit(14);
  cci.bit(2) = mode.bit(22);
  cci.bit(3) = mode.bit( 7);
  cci.bit(4) = mode.bit(15);
  cci.bit(5) = mode.bit(23);
  return cci ^ 0x3f;
}

auto RDRAM::encodeCCI(n6 cci) -> u32 {
  cci ^= 0x3f;
  n32 value = 0;
  value.bit( 6) = cci.bit(0);
  value.bit(14) = cci.bit(1);
  value.bit(22) = cci.bit(2);
  value.bit( 7) = cci.bit(3);
  value.bit(15) = cci.bit(4);
  value.bit(23) = cci.bit(5);
  return value;
}

auto RDRAM::updateMapping() -> void {
  mapIdentity = 1;
  for(u32 n : range(4)) {
    auto& chip = chips[n];
    if(!chip.present) continue;
    if(!chip.enable || chip.deviceID != n * 2 || chip.cci < chip.ccHigh) {
      mapIdentity = 0;
      return;
    }
  }
}

auto RDRAM::chipIndex(Chip* chip) -> u32 {
  return chip - chips;
}

auto RDRAM::selectChip(u32 select, bool broadcast) -> Chip* {
  if(broadcast) return nullptr;
  if(!ri.active()) return nullptr;

  n1 seenDisabled = 0;
  for(auto& chip : chips) {
    if(!chip.present) continue;
    n1 match = (chip.deviceID & ~1) == (select & ~1);
    if(chip.enable) {
      if(match) return &chip;
    } else if(!seenDisabled) {
      seenDisabled = 1;
      if(match) return &chip;
    }
  }
  return nullptr;
}

auto RDRAM::readRegister(Chip& chip, u32 index) -> u32 {
  if(!chip.enable) return 0;
  if(index >= 10) return 0;

  u32 data = 0;
  if(index == 0) data = chip.deviceType;
  if(index == 1) data = chip.deviceIDReg;
  if(index == 2) {
    data = chip.delay;
    data |= 3 << 24;
    data |= 3 << 16;
    data |= 2 <<  8;
    data |= 3 <<  0;
  }
  if(index == 3) {
    data = chip.mode & ~0x00c0c0c0;
    n6 cci = chip.autoCurrent ? chip.ccInternal : decodeCCI(chip.mode);
    data |= encodeCCI(cci);
    data ^= 0xc0c0'c0c0;
  }
  if(index == 4) data = chip.refreshInterval;
  if(index == 5) data = chip.refreshRow;
  if(index == 6) data = chip.rasInterval;
  if(index == 7) data = chip.minInterval;
  if(index == 8) data = chip.addressSelect;
  if(index == 9) data = chip.deviceManufacturer;
  return data;
}

auto RDRAM::writeRegister(Chip& chip, u32 index, u32 data, u8 repeatLength) -> void {
  if(index >= 10) return;

  if(chip.writeDelay != 1) {
    if(repeatLength >= 16) {
      data = data << 16 | data >> 16;
    } else {
      return;
    }
  }

  if(index == 0) return;
  if(index == 1) {
    chip.deviceIDReg = data;
    chip.deviceID = decodeDeviceID(data);
  }
  if(index == 2) {
    chip.delay = data & 0x3838'1838;
    chip.writeDelay = chip.delay >> 3 & 7;
  }
  if(index == 3) {
    chip.mode = data;
    n32 mode = data;
    chip.enable = mode.bit(25);
    chip.autoCurrent = mode.bit(31);
    chip.cci = decodeCCI(data);
    if(chip.autoCurrent) chip.ccInternal = chip.cci;
  }
  if(index == 4) chip.refreshInterval = data;
  if(index == 5) chip.refreshRow = data;
  if(index == 6) chip.rasInterval = data;
  if(index == 7) chip.minInterval = data;
  if(index == 8) chip.addressSelect = data;
  if(index == 9) return;

  updateMapping();
}

auto RDRAM::Writable::translate(u32 address) -> maybe<u32> {
  if(!ri.active()) return nothing;
  for(u32 n : range(4)) {
    auto& chip = self.chips[n];
    if(!chip.present || !chip.enable) continue;
    if((chip.deviceID >> 1) != (address >> 21)) continue;
    return n * 2_MiB + (address & 0x1f'ffff);
  }
  return nothing;
}

auto RDRAM::Writable::degrade(u32 address, u64 value, u32 chipIndex) -> u64 {
  auto& chip = self.chips[chipIndex];
  if(chip.cci >= chip.ccHigh) return value;
  if(chip.cci <= chip.ccLow) return 0;

  u32 span = chip.ccHigh - chip.ccLow;
  u32 progress = (u32)(chip.cci - chip.ccLow) * 256 / span;
  u64 result = 0;
  for(u32 bit : range(64)) {
    if(!((value >> bit) & 1)) continue;
    if((random() & 255) < progress) result |= 1ull << bit;
  }
  return result;
}

}
