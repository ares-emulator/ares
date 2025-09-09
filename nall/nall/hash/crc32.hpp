#pragma once

#include <nall/hash/hash.hpp>
#include <nall/iterator.hpp>

namespace nall::Hash {

struct CRC32 : Hash {
  using Hash::input;

  CRC32(std::span<const u8> buffer = {}) {
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
    result.reserve(4);
    for(auto n : reverse(range(4))) result.push_back((u8)(~checksum >> (n * 8)));
    return result;
  }

  auto value() const -> u32 {
    return ~checksum;
  }

private:
  static auto table(u8 index) -> u32 {
    static u32 table[256] = {};
    static bool initialized = false;

    if(!initialized) {
      initialized = true;
      for(auto index : range(256)) {
        u32 crc = index;
        for(auto bit : range(8)) {
          crc = (crc >> 1) ^ (crc & 1 ? 0xedb8'8320 : 0);
        }
        table[index] = crc;
      }
    }

    return table[index];
  }

  u32 checksum = 0;
};

}
