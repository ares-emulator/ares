#pragma once

#include <nall/hash/hash.hpp>
#include <span>

namespace nall::Hash {

struct CRC16 : Hash {
  using Hash::input;

  CRC16(std::span<const u8> buffer = {}) {
    reset();
    input(buffer);
  }

  auto reset() -> void override {
    checksum = ~0;
  }

  auto input(u8 value) -> void override {
    checksum = (checksum >> 8) ^ table(checksum ^ value);
  }

  auto output() const -> std::vector<u8> override {
    std::vector<u8> result;
    result.reserve(2);
    for(auto n : reverse(range(2))) result.push_back((u8)(~checksum >> (n * 8)));
    return result;
  }

  auto value() const -> u16 {
    return ~checksum;
  }

private:
  static auto table(u8 index) -> u16 {
    static u16 table[256] = {};
    static bool initialized = false;

    if(!initialized) {
      initialized = true;
      for(auto index : range(256)) {
        u16 crc = index;
        for(auto bit : range(8)) {
          crc = (crc >> 1) ^ (crc & 1 ? 0x8408 : 0);
        }
        table[index] = crc;
      }
    }

    return table[index];
  }

  u16 checksum = 0;
};

}
