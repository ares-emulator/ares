#pragma once

namespace nall {

//note: this function assumes the string contains valid UTF-8 characters
//invalid characters will result in an incorrect result from this function
//invalid case 1: byte 1 == 0b'01xxxxxx
//invalid case 2: bytes 2-4 != 0b'10xxxxxx
//invalid case 3: end of string without bytes 2-4 present
inline auto characters(string_view self, s32 offset, s32 length) -> u32 {
  u32 characters = 0;
  if(offset < 0) offset = self.size() - abs(offset);
  if(offset >= 0 && offset < self.size()) {
    if(length < 0) length = self.size() - offset;
    if(length >= 0) {
      for(s32 index = offset; index < offset + length;) {
        auto byte = self.data()[index++];
        if((byte & 0b111'00000) == 0b110'00000) index += 1;
        if((byte & 0b1111'0000) == 0b1110'0000) index += 2;
        if((byte & 0b11111'000) == 0b11110'000) index += 3;
        characters++;
      }
    }
  }
  return characters;
}

inline auto string::characters(s32 offset, s32 length) const -> u32 {
  return nall::characters(*this, offset, length);
}

}
