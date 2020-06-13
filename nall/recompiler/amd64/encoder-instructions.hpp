#pragma once

//{
  auto call(reg64 rt) {
    auto _rt = (uint)rt;
    emit.rex(0, 0, 0, _rt & 8);
    emit.byte(0xff);
    emit.modrm(3, 2, _rt & 7);
  }

  auto mov(reg64 rt, imm32 is) {
    auto _rt = (uint)rt;
    emit.rex(1, 0, 0, _rt & 8);
    emit.byte(0xc7);
    emit.modrm(3, 0, _rt & 7);
    emit.dword(is.data);
  }

  auto mov(reg64 rt, imm64 is) {
    auto _rt = (uint)rt;
    emit.rex(1, 0, 0, _rt & 8);
    emit.byte(0xb8 | _rt & 7);
    emit.qword(is.data);
  }

  auto mov(reg64 rt, mem64 ps) {
    if(unlikely(rt != rax)) throw;
    emit.rex(1, 0, 0, 0);
    emit.byte(0xa1);
    emit.qword(ps.data);
  }

  auto ret() { emit.byte(0xc3); }

  #define op(code) \
    auto _rt = (uint)rt, _rs = (uint)rs; \
    emit.rex(0, _rs & 8, 0, _rt & 8); \
    emit.byte(code); \
    emit.modrm(3, _rs & 7, _rt & 7);
  auto adc (reg8 rt, reg8 rs) { op(0x10); }
  auto add (reg8 rt, reg8 rs) { op(0x00); }
  auto and (reg8 rt, reg8 rs) { op(0x20); }
  auto cmp (reg8 rt, reg8 rs) { op(0x38); }
  auto mov (reg8 rt, reg8 rs) { op(0x88); }
  auto or  (reg8 rt, reg8 rs) { op(0x08); }
  auto sbb (reg8 rt, reg8 rs) { op(0x18); }
  auto sub (reg8 rt, reg8 rs) { op(0x28); }
  auto test(reg8 rt, reg8 rs) { op(0x84); }
  auto xor (reg8 rt, reg8 rs) { op(0x30); }
  #undef op

  #define op(code) \
    auto _rt = (uint)rt, _rs = (uint)rs; \
    emit.byte(0x66); \
    emit.rex(0, _rs & 8, 0, _rt & 8); \
    emit.byte(code); \
    emit.modrm(3, _rs & 7, _rt & 7);
  auto adc (reg16 rt, reg16 rs) { op(0x11); }
  auto add (reg16 rt, reg16 rs) { op(0x01); }
  auto and (reg16 rt, reg16 rs) { op(0x21); }
  auto cmp (reg16 rt, reg16 rs) { op(0x39); }
  auto mov (reg16 rt, reg16 rs) { op(0x89); }
  auto or  (reg16 rt, reg16 rs) { op(0x09); }
  auto sbb (reg16 rt, reg16 rs) { op(0x19); }
  auto sub (reg16 rt, reg16 rs) { op(0x29); }
  auto test(reg16 rt, reg16 rs) { op(0x85); }
  auto xor (reg16 rt, reg16 rs) { op(0x31); }
  #undef op

  #define op(code) \
    auto _rt = (uint)rt, _rs = (uint)rs; \
    emit.rex(0, _rs & 8, 0, _rt & 8); \
    emit.byte(code); \
    emit.modrm(3, _rs & 7, _rt & 7);
  auto adc (reg32 rt, reg32 rs) { op(0x11); }
  auto add (reg32 rt, reg32 rs) { op(0x01); }
  auto and (reg32 rt, reg32 rs) { op(0x21); }
  auto cmp (reg32 rt, reg32 rs) { op(0x39); }
  auto mov (reg32 rt, reg32 rs) { op(0x89); }
  auto or  (reg32 rt, reg32 rs) { op(0x09); }
  auto sbb (reg32 rt, reg32 rs) { op(0x19); }
  auto sub (reg32 rt, reg32 rs) { op(0x29); }
  auto test(reg32 rt, reg32 rs) { op(0x85); }
  auto xor (reg32 rt, reg32 rs) { op(0x31); }
  #undef op

  #define op(code) \
    auto _rt = (uint)rt, _rs = (uint)rs; \
    emit.rex(1, _rs & 8, 0, _rt & 8); \
    emit.byte(code); \
    emit.modrm(3, _rs & 7, _rt & 7);
  auto adc (reg64 rt, reg64 rs) { op(0x11); }
  auto add (reg64 rt, reg64 rs) { op(0x01); }
  auto and (reg64 rt, reg64 rs) { op(0x21); }
  auto cmp (reg64 rt, reg64 rs) { op(0x39); }
  auto mov (reg64 rt, reg64 rs) { op(0x89); }
  auto or  (reg64 rt, reg64 rs) { op(0x09); }
  auto sbb (reg64 rt, reg64 rs) { op(0x19); }
  auto sub (reg64 rt, reg64 rs) { op(0x29); }
  auto test(reg64 rt, reg64 rs) { op(0x85); }
  auto xor (reg64 rt, reg64 rs) { op(0x31); }

  #define op(code, reg) \
    auto _rt = (uint)rt; \
    emit.rex(1, 0, 0, _rt & 8); \
    emit.byte(code); \
    emit.modrm(3, reg, _rt & 7); \
    emit.byte(is.data);
  auto adc(reg64 rt, imm8 is) { op(0x83, 2); }
  auto add(reg64 rt, imm8 is) { op(0x83, 0); }
  auto and(reg64 rt, imm8 is) { op(0x83, 4); }
  auto cmp(reg64 rt, imm8 is) { op(0x83, 7); }
  auto or (reg64 rt, imm8 is) { op(0x83, 1); }
  auto sbb(reg64 rt, imm8 is) { op(0x83, 3); }
  auto sub(reg64 rt, imm8 is) { op(0x83, 5); }
  auto xor(reg64 rt, imm8 is) { op(0x83, 6); }
  #undef op

  #define op(code) \
    emit.byte(code); \
    emit.byte(it.data);
  auto jnz(imm8 it) { op(0x75); }
  auto jz (imm8 it) { op(0x74); }
  #undef op
//};
