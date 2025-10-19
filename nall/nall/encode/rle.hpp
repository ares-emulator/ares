#pragma once

namespace nall::Encode {

template<u32 S = 1, u32 M = 4 / S>  //S = word size; M = match length
inline auto RLE(std::span<const u8> input) -> std::vector<u8> {
  std::vector<u8> output;
  for(u32 byte : range(8)) output.push_back((u8)(input.size() >> (byte * 8)));

  u32 base = 0;
  u32 skip = 0;

  auto load = [&](u32 offset) -> u8 {
    return input(offset);
  };

  auto read = [&](u32 offset) -> u64 {
    u64 value = 0;
    for(u32 byte : range(S)) value |= (u64)load(offset + byte) << (byte * 8);
    return value;
  };

  auto write = [&](u64 value) -> void {
    for(u32 byte : range(S)) output.push_back((u8)(value >> (byte * 8)));
  };

  auto flush = [&] {
    output.push_back((u8)(skip - 1));
    do {
      write(read(base));
      base += S;
    } while(--skip);
  };

  while(base + S * skip < input.size()) {
    u32 same = 1;
    for(u32 offset = base + S * (skip + 1); offset < input.size(); offset += S) {
      if(read(offset) != read(base + S * skip)) break;
      if(++same == 127 + M) break;
    }

    if(same < M) {
      if(++skip == 128) flush();
    } else {
      if(skip) flush();
      output.push_back((u8)(128 | (same - M)));
      write(read(base));
      base += S * same;
    }
  }
  if(skip) flush();

  return output;
}

}
