#pragma once

struct emitter {
  auto byte() {
  }

  template<typename... P>
  alwaysinline auto byte(u8 data, P&&... p) {
    span.write(data);
    byte(forward<P>(p)...);
  }

  alwaysinline auto word(u16 data) {
    span.write(data >> 0);
    span.write(data >> 8);
  }

  alwaysinline auto dword(u32 data) {
    span.write(data >>  0);
    span.write(data >>  8);
    span.write(data >> 16);
    span.write(data >> 24);
  }

  alwaysinline auto qword(u64 data) {
    span.write(data >>  0);
    span.write(data >>  8);
    span.write(data >> 16);
    span.write(data >> 24);
    span.write(data >> 32);
    span.write(data >> 40);
    span.write(data >> 48);
    span.write(data >> 56);
  }

  alwaysinline auto rex(bool w, bool r, bool x, bool b) {
    u8 data = 0x40 | w << 3 | r << 2 | x << 1 | b << 0;
    if(data == 0x40) return;  //rex prefix not needed
    byte(data);
  }

  //mod: {[r/m], [r/m+dis8], [r/m+dis32], r/m}
  alwaysinline auto modrm(u8 mod, u8 reg, u8 rm) {
    byte(mod << 6 | reg << 3 | rm << 0);
  }

  //scale: {index*1, index*2, index*4, index*8}
  //index: {eax, ecx, edx, ebx, invalid, ebp, esi, edi}
  //base:  {eax, ecx, edx, ebx, esp, displacement, esi, edi}
  alwaysinline auto sib(u8 scale, u8 index, u8 base) {
    byte(scale << 6 | index << 3 | base << 0);
  }

  array_span<u8> span, origin;
} emit;

alwaysinline auto bind(array_span<u8> span) {
  emit.span = span;
  emit.origin = span;
}

alwaysinline auto distance(u64 target) const -> s64 {
  return target - (u64)emit.span.data();
}

alwaysinline auto size() const -> u32 {
  return emit.span.data() - emit.origin.data();
}
