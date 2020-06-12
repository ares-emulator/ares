#pragma once

namespace nall::dynarec::amd64 {

struct emitter {
  auto bind(array_span<u8> span) {
    _span = span;
  }

  auto byte(u8 data) {
    _span.write(data);
  }

  auto word(u16 data) {
    _span.write(data >> 0);
    _span.write(data >> 8);
  }

  auto dword(u32 data) {
    _span.write(data >>  0);
    _span.write(data >>  8);
    _span.write(data >> 16);
    _span.write(data >> 24);
  }

  auto qword(u64 data) {
    _span.write(data >>  0);
    _span.write(data >>  8);
    _span.write(data >> 16);
    _span.write(data >> 24);
    _span.write(data >> 32);
    _span.write(data >> 40);
    _span.write(data >> 48);
    _span.write(data >> 56);
  }

  auto rex(bool w, bool r, bool x, bool b) {
    u8 data = 0x40 | w << 3 | r << 2 | x << 1 | b << 0;
    if(data == 0x40) return;  //rex prefix not needed
    byte(data);
  }

  auto modrm(u8 mod, u8 reg, u8 rm) {
    byte(mod << 6 | reg << 3 | rm << 0);
  }

  array_span<u8> _span;
};

}
