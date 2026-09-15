#pragma once

#include <nall/bcd.hpp>
#include <nall/cd/crc16.hpp>
#include <nall/range.hpp>
#include <array>
#include <map>
#include <span>

namespace nall::CD {

struct SubchannelPatch {
  enum class Format { SBI, LSD };
  // Keys are absolute MSF sector counts, including the track1 pregap.
  std::map<u32, std::array<u8, 12>> sectors;

  auto decode(std::span<const u8> bytes, Format format) -> bool {
    sectors.clear();
    if(format == Format::SBI) {
      if(bytes.size() < 4 || bytes[0] != 'S' || bytes[1] != 'B' || bytes[2] != 'I' || bytes[3]) return false;
      bytes = bytes.subspan(4);
    }
    std::map<u32, std::array<u8, 12>> decoded;
    u32 stride = format == Format::SBI ? 14 : 15;
    while(bytes.size() >= stride) {
      for(u32 index : range(3)) if(!BCD::valid(bytes[index])) return false;
      if(format == Format::SBI && bytes[3] != 1) return false;
      u32 address = BCD::decode(bytes[0]) * 4500 + BCD::decode(bytes[1]) * 75 + BCD::decode(bytes[2]);
      std::array<u8, 12> q{};
      u32 start = format == Format::SBI ? 4 : 3;
      u32 length = format == Format::SBI ? 10 : 12;
      for(u32 index : range(length)) q[index] = bytes[start + index];
      if(format == Format::SBI) {
        u16 crc = CRC16({q.data(), 10}) ^ 0xffff;  // SBI explicitly represents a failed Q checksum.
        q[10] = crc >> 8;
        q[11] = crc;
      }
      decoded.emplace(address, q);  // Reference duplicate policy: first entry wins.
      bytes = bytes.subspan(stride);
    }
    // Reference loaders ignore an incomplete final record; do not turn it into a zero-filled replacement.
    sectors = std::move(decoded);
    return true;
  }
};

}
