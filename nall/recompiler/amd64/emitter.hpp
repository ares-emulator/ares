#pragma once

struct emitter {
  auto byte(u8 data) {
    span.write(data);
  }

  auto word(u16 data) {
    span.write(data >> 0);
    span.write(data >> 8);
  }

  auto dword(u32 data) {
    span.write(data >>  0);
    span.write(data >>  8);
    span.write(data >> 16);
    span.write(data >> 24);
  }

  auto qword(u64 data) {
    span.write(data >>  0);
    span.write(data >>  8);
    span.write(data >> 16);
    span.write(data >> 24);
    span.write(data >> 32);
    span.write(data >> 40);
    span.write(data >> 48);
    span.write(data >> 56);
  }

  auto rex(bool w, bool r, bool x, bool b) {
    u8 data = 0x40 | w << 3 | r << 2 | x << 1 | b << 0;
    if(data == 0x40) return;  //rex prefix not needed
    byte(data);
  }

  auto modrm(u8 mod, u8 reg, u8 rm) {
    byte(mod << 6 | reg << 3 | rm << 0);
  }

  array_span<u8> span, origin;
} emit;

auto bind(array_span<uint8_t> span) {
  emit.span = span;
  emit.origin = span;
}

auto size() const -> u32 {
  return emit.span.data() - emit.origin.data();
}
