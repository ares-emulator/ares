#pragma once

#include <nall/hash/hash.hpp>

namespace nall::Hash {

struct SHA256 : Hash {
  using Hash::input;

  SHA256(array_view<u8> buffer = {}) {
    reset();
    input(buffer);
  }

  auto reset() -> void override {
    for(auto& n : queue) n = 0;
    for(auto& n : w) n = 0;
    for(auto  n : range(8)) h[n] = square(n);
    queued = length = 0;
  }

  auto input(u8 value) -> void override {
    byte(value);
    length++;
  }

  auto output() const -> vector<u8> override {
    SHA256 self(*this);
    self.finish();
    vector<u8> result;
    for(auto h : self.h) {
      for(auto n : reverse(range(4))) result.append(h >> n * 8);
    }
    return result;
  }

  auto value() const -> u256 {
    u256 value = 0;
    for(auto byte : output()) value = value << 8 | byte;
    return value;
  }

private:
  auto byte(u8 value) -> void {
    u32 shift = (3 - (queued & 3)) * 8;
    queue[queued >> 2] &= ~(0xff << shift);
    queue[queued >> 2] |= (value << shift);
    if(++queued == 64) block(), queued = 0;
  }

  auto block() -> void {
    for(auto n : range(16)) w[n] = queue[n];
    for(auto n : range(16, 64)) {
      u32 a = ror(w[n - 15],  7) ^ ror(w[n - 15], 18) ^ (w[n - 15] >>  3);
      u32 b = ror(w[n -  2], 17) ^ ror(w[n -  2], 19) ^ (w[n -  2] >> 10);
      w[n] = w[n - 16] + w[n - 7] + a + b;
    }
    u32 t[8];
    for(auto n : range(8)) t[n] = h[n];
    for(auto n : range(64)) {
      u32 a = ror(t[0], 2) ^ ror(t[0], 13) ^ ror(t[0], 22);
      u32 b = ror(t[4], 6) ^ ror(t[4], 11) ^ ror(t[4], 25);
      u32 c = (t[0] & t[1]) ^ (t[0] & t[2]) ^ (t[1] & t[2]);
      u32 d = (t[4] & t[5]) ^ (~t[4] & t[6]);
      u32 e = t[7] + w[n] + cube(n) + b + d;
      t[7] = t[6]; t[6] = t[5]; t[5] = t[4]; t[4] = t[3] + e;
      t[3] = t[2]; t[2] = t[1]; t[1] = t[0]; t[0] = a + c + e;
    }
    for(auto n : range(8)) h[n] += t[n];
  }

  auto finish() -> void {
    byte(0x80);
    while(queued != 56) byte(0x00);
    for(auto n : range(8)) byte(length * 8 >> (7 - n) * 8);
  }

  auto square(u32 n) -> u32 {
    static const u32 value[8] = {
      0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };
    return value[n];
  }

  auto cube(u32 n) -> u32 {
    static const u32 value[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };
    return value[n];
  }

  u32 queue[16] = {};
  u32 w[64] = {};
  u32 h[8] = {};
  u32 queued = 0;
  u64 length = 0;
};

}
