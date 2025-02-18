#pragma once

//CRC-16/KERMIT

namespace nall::CD {

inline auto CRC16(array_view<u8> data) -> u16 {
  u16 crc = 0;
  while(data) {
    crc ^= *data++ << 8;
    for(u32 bit : range(8)) {
      crc = crc << 1 ^ (crc & 0x8000 ? 0x1021 : 0);
    }
  }
  return ~crc;
}

}
