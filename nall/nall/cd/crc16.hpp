#pragma once

#include <nall/span-helpers.hpp>
#include <span>

//CRC-16/KERMIT

namespace nall::CD {

inline auto CRC16(std::span<const u8> data) -> u16 {
  u16 crc = 0;
  while(data.size()) {
    crc ^= consume_front(data) << 8;
    for(u32 bit : range(8)) {
      crc = crc << 1 ^ (crc & 0x8000 ? 0x1021 : 0);
    }
  }
  return ~crc;
}

}
