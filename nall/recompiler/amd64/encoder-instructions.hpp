#pragma once

//{
  auto clc()  { emit.byte(0xf8); }
  auto cmc()  { emit.byte(0xf5); }
  auto lahf() { emit.byte(0x9f); }
  auto sahf() { emit.byte(0x9e); }
  auto stc()  { emit.byte(0xf9); }
  auto ret()  { emit.byte(0xc3); }

  //call reg64
  auto call(reg64 rt) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0xff);
    emit.modrm(3, 2, rt & 7);
  }

  //lea reg64,[reg64+imm8]
  auto lea(reg64 rt, dis8 ds) {
    emit.rex(1, rt & 8, 0, ds.reg & 8);
    emit.byte(0x8d);
    emit.modrm(1, rt & 7, ds.reg & 7);
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4);
    emit.byte(ds.imm);
  }

  //lea reg64,[reg64+imm32]
  auto lea(reg64 rt, dis32 ds) {
    emit.rex(1, rt & 8, 0, ds.reg & 8);
    emit.byte(0x8d);
    emit.modrm(2, rt & 7, ds.reg & 7);
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4);
    emit.dword(ds.imm);
  }

  //mov reg8,imm8
  auto mov(reg8 rt, imm8 is) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0xb0 | rt & 7);
    emit.byte(is.data);
  }

  //mov reg32,imm32
  auto mov(reg32 rt, imm32 is) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0xb8 | rt & 7);
    emit.dword(is.data);
  }

  //mov reg64,imm32
  auto mov(reg64 rt, imm32 is) {
    emit.rex(1, 0, 0, rt & 8);
    emit.byte(0xc7);
    emit.modrm(3, 0, rt & 7);
    emit.dword(is.data);
  }

  //mov reg64,imm64
  auto mov(reg64 rt, imm64 is) {
    emit.rex(1, 0, 0, rt & 8);
    emit.byte(0xb8 | rt & 7);
    emit.qword(is.data);
  }

  //mov reg8,[mem64]
  auto mov(reg8 rt, mem64 ps) {
    if(unlikely(rt != al)) throw;
    emit.byte(0xa0);
    emit.qword(ps.data);
  }

  //mov reg16,[mem64]
  auto mov(reg16 rt, mem64 ps) {
    if(unlikely(rt != ax)) throw;
    emit.byte(0x66, 0xa1);
    emit.qword(ps.data);
  }

  //mov reg32,[mem64]
  auto mov(reg32 rt, mem64 ps) {
    if(unlikely(rt != eax)) throw;
    emit.byte(0xa1);
    emit.qword(ps.data);
  }

  //mov reg64,[mem64]
  auto mov(reg64 rt, mem64 ps) {
    if(unlikely(rt != rax)) throw;
    emit.rex(1, 0, 0, 0);
    emit.byte(0xa1);
    emit.qword(ps.data);
  }

  //mov [mem64],reg8
  auto mov(mem64 pt, reg8 rs) {
    if(unlikely(rs != al)) throw;
    emit.byte(0xa2);
    emit.qword(pt.data);
  }

  //mov [mem64+imm8],imm8
  auto movb(dis8 dt, imm8 is) {
    emit.rex(0, 0, 0, dt.reg & 8);
    emit.byte(0xc6);
    emit.modrm(1, 0, dt.reg & 7);
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4);
    emit.byte(dt.imm);
    emit.byte(is.data);
  }

  //mov [mem64],reg16
  auto mov(mem64 pt, reg16 rs) {
    if(unlikely(rs != ax)) throw;
    emit.byte(0x66, 0xa3);
    emit.qword(pt.data);
  }

  //mov [mem64],reg32
  auto mov(mem64 pt, reg32 rs) {
    if(unlikely(rs != eax)) throw;
    emit.byte(0xa3);
    emit.qword(pt.data);
  }

  //mov [mem64],reg64
  auto mov(mem64 pt, reg64 rs) {
    if(unlikely(rs != rax)) throw;
    emit.rex(1, 0, 0, 0);
    emit.byte(0xa3);
    emit.qword(pt.data);
  }

  //op reg8,[reg64]
  #define op(code) \
    emit.rex(0, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(0, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4);
  auto adc(reg8 rt, dis ds) { op(0x12); }
  auto add(reg8 rt, dis ds) { op(0x02); }
  auto and(reg8 rt, dis ds) { op(0x22); }
  auto cmp(reg8 rt, dis ds) { op(0x3a); }
  auto mov(reg8 rt, dis ds) { op(0x8a); }
  auto or (reg8 rt, dis ds) { op(0x0a); }
  auto sbb(reg8 rt, dis ds) { op(0x1a); }
  auto sub(reg8 rt, dis ds) { op(0x2a); }
  auto xor(reg8 rt, dis ds) { op(0x32); }
  #undef op

  //op reg8,[reg64+imm8]
  #define op(code) \
    emit.rex(0, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(1, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(ds.imm);
  auto adc(reg8 rt, dis8 ds) { op(0x12); }
  auto add(reg8 rt, dis8 ds) { op(0x02); }
  auto and(reg8 rt, dis8 ds) { op(0x22); }
  auto cmp(reg8 rt, dis8 ds) { op(0x3a); }
  auto mov(reg8 rt, dis8 ds) { op(0x8a); }
  auto or (reg8 rt, dis8 ds) { op(0x0a); }
  auto sbb(reg8 rt, dis8 ds) { op(0x1a); }
  auto sub(reg8 rt, dis8 ds) { op(0x2a); }
  auto xor(reg8 rt, dis8 ds) { op(0x32); }
  #undef op

  //op reg32,[reg64]
  #define op(code) \
    emit.rex(0, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(0, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4);
  auto adc(reg32 rt, dis ds) { op(0x13); }
  auto add(reg32 rt, dis ds) { op(0x03); }
  auto and(reg32 rt, dis ds) { op(0x23); }
  auto cmp(reg32 rt, dis ds) { op(0x3b); }
  auto mov(reg32 rt, dis ds) { op(0x8b); }
  auto or (reg32 rt, dis ds) { op(0x0b); }
  auto sbb(reg32 rt, dis ds) { op(0x1b); }
  auto sub(reg32 rt, dis ds) { op(0x2b); }
  auto xor(reg32 rt, dis ds) { op(0x33); }
  #undef op

  //op reg32,[reg64+imm8]
  #define op(code) \
    emit.rex(0, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(1, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(ds.imm);
  auto adc(reg32 rt, dis8 ds) { op(0x13); }
  auto add(reg32 rt, dis8 ds) { op(0x03); }
  auto and(reg32 rt, dis8 ds) { op(0x23); }
  auto cmp(reg32 rt, dis8 ds) { op(0x3b); }
  auto mov(reg32 rt, dis8 ds) { op(0x8b); }
  auto or (reg32 rt, dis8 ds) { op(0x0b); }
  auto sbb(reg32 rt, dis8 ds) { op(0x1b); }
  auto sub(reg32 rt, dis8 ds) { op(0x2b); }
  auto xor(reg32 rt, dis8 ds) { op(0x33); }
  #undef op

  //op reg64,[reg64]
  #define op(code) \
    emit.rex(1, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(0, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4);
  auto adc(reg64 rt, dis ds) { op(0x13); }
  auto add(reg64 rt, dis ds) { op(0x03); }
  auto and(reg64 rt, dis ds) { op(0x23); }
  auto cmp(reg64 rt, dis ds) { op(0x3b); }
  auto mov(reg64 rt, dis ds) { op(0x8b); }
  auto or (reg64 rt, dis ds) { op(0x0b); }
  auto sbb(reg64 rt, dis ds) { op(0x1b); }
  auto sub(reg64 rt, dis ds) { op(0x2b); }
  auto xor(reg64 rt, dis ds) { op(0x33); }
  #undef op

  //op reg64,[reg64+imm8]
  #define op(code) \
    emit.rex(1, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(1, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(ds.imm);
  auto adc(reg64 rt, dis8 ds) { op(0x13); }
  auto add(reg64 rt, dis8 ds) { op(0x03); }
  auto and(reg64 rt, dis8 ds) { op(0x23); }
  auto cmp(reg64 rt, dis8 ds) { op(0x3b); }
  auto mov(reg64 rt, dis8 ds) { op(0x8b); }
  auto or (reg64 rt, dis8 ds) { op(0x0b); }
  auto sbb(reg64 rt, dis8 ds) { op(0x1b); }
  auto sub(reg64 rt, dis8 ds) { op(0x2b); }
  auto xor(reg64 rt, dis8 ds) { op(0x33); }
  #undef op

  //op reg64,[reg64+imm32]
  #define op(code) \
    emit.rex(1, rt & 8, 0, ds.reg & 8); \
    emit.byte(code); \
    emit.modrm(2, rt & 7, ds.reg & 7); \
    if(ds.reg == rsp || ds.reg == r12) emit.sib(0, 4, 4); \
    emit.dword(ds.imm);
  auto adc(reg64 rt, dis32 ds) { op(0x13); }
  auto add(reg64 rt, dis32 ds) { op(0x03); }
  auto and(reg64 rt, dis32 ds) { op(0x23); }
  auto cmp(reg64 rt, dis32 ds) { op(0x3b); }
  auto mov(reg64 rt, dis32 ds) { op(0x8b); }
  auto or (reg64 rt, dis32 ds) { op(0x0b); }
  auto sbb(reg64 rt, dis32 ds) { op(0x1b); }
  auto sub(reg64 rt, dis32 ds) { op(0x2b); }
  auto xor(reg64 rt, dis32 ds) { op(0x33); }
  #undef op

  //op [reg64+imm8],reg8
  #define op(code) \
    emit.rex(0, rs & 8, 0, dt.reg & 8); \
    emit.byte(code); \
    emit.modrm(1, rs & 7, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(dt.imm);
  auto adc(dis8 dt, reg8 rs) { op(0x10); }
  auto add(dis8 dt, reg8 rs) { op(0x00); }
  auto and(dis8 dt, reg8 rs) { op(0x20); }
  auto cmp(dis8 dt, reg8 rs) { op(0x38); }
  auto mov(dis8 dt, reg8 rs) { op(0x88); }
  auto or (dis8 dt, reg8 rs) { op(0x08); }
  auto sbb(dis8 dt, reg8 rs) { op(0x18); }
  auto sub(dis8 dt, reg8 rs) { op(0x28); }
  auto xor(dis8 dt, reg8 rs) { op(0x30); }
  #undef op

  //op reg64,imm32
  #define op(group) \
    emit.rex(1, 0, 0, rt & 8); \
    emit.byte(0x81); \
    emit.modrm(3, group, rt & 7); \
    emit.dword(is.data);
  auto add(reg64 rt, imm32 is) { op(0); }
  auto or (reg64 rt, imm32 is) { op(1); }
  auto adc(reg64 rt, imm32 is) { op(2); }
  auto sbb(reg64 rt, imm32 is) { op(3); }
  auto and(reg64 rt, imm32 is) { op(4); }
  auto sub(reg64 rt, imm32 is) { op(5); }
  auto xor(reg64 rt, imm32 is) { op(6); }
  auto cmp(reg64 rt, imm32 is) { op(7); }
  #undef op

  //op.d [reg64+imm8],imm8
  #define op(group) \
    emit.rex(0, 0, 0, dt.reg & 8); \
    emit.byte(0x83); \
    emit.modrm(1, group, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(dt.imm); \
    emit.byte(is.data);
  auto addd(dis8 dt, imm8 is) { op(0); }
  auto ord (dis8 dt, imm8 is) { op(1); }
  auto adcd(dis8 dt, imm8 is) { op(2); }
  auto sbbd(dis8 dt, imm8 is) { op(3); }
  auto andd(dis8 dt, imm8 is) { op(4); }
  auto subd(dis8 dt, imm8 is) { op(5); }
  auto xord(dis8 dt, imm8 is) { op(6); }
  auto cmpd(dis8 dt, imm8 is) { op(7); }
  #undef op

  //op [reg64],reg32
  #define op(code) \
    emit.rex(0, rs & 8, 0, dt.reg & 8); \
    emit.byte(code); \
    emit.modrm(0, rs & 7, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4);
  auto adc(dis dt, reg32 rs) { op(0x11); }
  auto add(dis dt, reg32 rs) { op(0x01); }
  auto and(dis dt, reg32 rs) { op(0x21); }
  auto cmp(dis dt, reg32 rs) { op(0x39); }
  auto mov(dis dt, reg32 rs) { op(0x89); }
  auto or (dis dt, reg32 rs) { op(0x09); }
  auto sbb(dis dt, reg32 rs) { op(0x19); }
  auto sub(dis dt, reg32 rs) { op(0x29); }
  auto xor(dis dt, reg32 rs) { op(0x31); }
  #undef op

  //op [reg64+imm8],reg32
  #define op(code) \
    emit.rex(0, rs & 8, 0, dt.reg & 8); \
    emit.byte(code); \
    emit.modrm(1, rs & 7, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(dt.imm);
  auto adc(dis8 dt, reg32 rs) { op(0x11); }
  auto add(dis8 dt, reg32 rs) { op(0x01); }
  auto and(dis8 dt, reg32 rs) { op(0x21); }
  auto cmp(dis8 dt, reg32 rs) { op(0x39); }
  auto mov(dis8 dt, reg32 rs) { op(0x89); }
  auto or (dis8 dt, reg32 rs) { op(0x09); }
  auto sbb(dis8 dt, reg32 rs) { op(0x19); }
  auto sub(dis8 dt, reg32 rs) { op(0x29); }
  auto xor(dis8 dt, reg32 rs) { op(0x31); }
  #undef op

  //op [reg64],reg64
  #define op(code) \
    emit.rex(0, rs & 8, 0, dt.reg & 8); \
    emit.byte(code); \
    emit.modrm(0, rs & 7, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4);
  auto adc(dis dt, reg64 rs) { op(0x11); }
  auto add(dis dt, reg64 rs) { op(0x01); }
  auto and(dis dt, reg64 rs) { op(0x21); }
  auto cmp(dis dt, reg64 rs) { op(0x39); }
  auto mov(dis dt, reg64 rs) { op(0x89); }
  auto or (dis dt, reg64 rs) { op(0x09); }
  auto sbb(dis dt, reg64 rs) { op(0x19); }
  auto sub(dis dt, reg64 rs) { op(0x29); }
  auto xor(dis dt, reg64 rs) { op(0x31); }
  #undef op

  //op [reg64+imm8],reg64
  #define op(code) \
    emit.rex(1, rs & 8, 0, dt.reg & 8); \
    emit.byte(code); \
    emit.modrm(1, rs & 7, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4); \
    emit.byte(dt.imm);
  auto adc(dis8 dt, reg64 rs) { op(0x11); }
  auto add(dis8 dt, reg64 rs) { op(0x01); }
  auto and(dis8 dt, reg64 rs) { op(0x21); }
  auto cmp(dis8 dt, reg64 rs) { op(0x39); }
  auto mov(dis8 dt, reg64 rs) { op(0x89); }
  auto or (dis8 dt, reg64 rs) { op(0x09); }
  auto sbb(dis8 dt, reg64 rs) { op(0x19); }
  auto sub(dis8 dt, reg64 rs) { op(0x29); }
  auto xor(dis8 dt, reg64 rs) { op(0x31); }
  #undef op

  //op [reg64+imm32],reg64
  #define op(code) \
    emit.rex(1, rs & 8, 0, dt.reg & 8); \
    emit.byte(code); \
    emit.modrm(2, rs & 7, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4); \
    emit.dword(dt.imm);
  auto adc(dis32 dt, reg64 rs) { op(0x11); }
  auto add(dis32 dt, reg64 rs) { op(0x01); }
  auto and(dis32 dt, reg64 rs) { op(0x21); }
  auto cmp(dis32 dt, reg64 rs) { op(0x39); }
  auto mov(dis32 dt, reg64 rs) { op(0x89); }
  auto or (dis32 dt, reg64 rs) { op(0x09); }
  auto sbb(dis32 dt, reg64 rs) { op(0x19); }
  auto sub(dis32 dt, reg64 rs) { op(0x29); }
  auto xor(dis32 dt, reg64 rs) { op(0x31); }
  #undef op

  //op reg32,reg8
  #define op(code) \
    emit.rex(0, rt & 8, 0, rs & 8); \
    emit.byte(0x0f, code); \
    emit.modrm(3, rt & 7, rs & 7);
  auto movsx(reg32 rt, reg8 rs) { op(0xbe); }
  auto movzx(reg32 rt, reg8 rs) { op(0xb6); }
  #undef op

  //op reg32,reg16
  #define op(code) \
    emit.rex(0, rt & 8, 0, rs & 8); \
    emit.byte(0x0f, code); \
    emit.modrm(3, rt & 7, rs & 7);
  auto movsx(reg32 rt, reg16 rs) { op(0xbf); }
  auto movzx(reg32 rt, reg16 rs) { op(0xb7); }
  #undef op

  auto movsxd(reg64 rt, reg32 rs) {
    emit.rex(1, rt & 8, 0, rs & 8);
    emit.byte(0x63);
    emit.modrm(3, rt & 7, rs & 7);
  }

  //incd [reg64+imm8]
  auto incd(dis8 dt) {
    emit.rex(0, 0, 0, dt.reg & 8);
    emit.byte(0xff);
    emit.modrm(1, 0, dt.reg & 7);
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4);
    emit.byte(dt.imm);
  }

  //decd [reg64+imm8]
  auto decd(dis8 dt) {
    emit.rex(0, 0, 0, dt.reg & 8);
    emit.byte(0xff);
    emit.modrm(1, 1, dt.reg & 7);
    if(dt.reg == rsp || dt.reg == r12) emit.sib(0, 4, 4);
    emit.byte(dt.imm);
  }

  //inc reg32
  auto inc(reg32 rt) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0xff);
    emit.modrm(3, 0, rt & 7);
  }

  //dec reg32
  auto dec(reg32 rt) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0xff);
    emit.modrm(3, 1, rt & 7);
  }

  //inc reg64
  auto inc(reg64 rt) {
    emit.rex(1, 0, 0, rt & 8);
    emit.byte(0xff);
    emit.modrm(3, 0, rt & 7);
  }

  //dec reg64
  auto dec(reg64 rt) {
    emit.rex(1, 0, 0, rt & 8);
    emit.byte(0xff);
    emit.modrm(3, 1, rt & 7);
  }

  #define op(code) \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0xd0); \
    emit.modrm(3, code, rt & 7);
  auto rol(reg8 rt) { op(0); }
  auto ror(reg8 rt) { op(1); }
  auto rcl(reg8 rt) { op(2); }
  auto rcr(reg8 rt) { op(3); }
  auto shl(reg8 rt) { op(4); }
  auto shr(reg8 rt) { op(5); }
  auto sal(reg8 rt) { op(6); }
  auto sar(reg8 rt) { op(7); }
  #undef op

  #define op(code) \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0xd1); \
    emit.modrm(3, code, rt & 7);
  auto rol(reg32 rt) { op(0); }
  auto ror(reg32 rt) { op(1); }
  auto rcl(reg32 rt) { op(2); }
  auto rcr(reg32 rt) { op(3); }
  auto shl(reg32 rt) { op(4); }
  auto shr(reg32 rt) { op(5); }
  auto sal(reg32 rt) { op(6); }
  auto sar(reg32 rt) { op(7); }
  #undef op

  #define op(code) \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0xc1); \
    emit.modrm(3, code, rt & 7); \
    emit.byte(is.data);
  auto rol(reg32 rt, imm8 is) { op(0); }
  auto ror(reg32 rt, imm8 is) { op(1); }
  auto rcl(reg32 rt, imm8 is) { op(2); }
  auto rcr(reg32 rt, imm8 is) { op(3); }
  auto shl(reg32 rt, imm8 is) { op(4); }
  auto shr(reg32 rt, imm8 is) { op(5); }
  auto sal(reg32 rt, imm8 is) { op(6); }
  auto sar(reg32 rt, imm8 is) { op(7); }
  #undef op

  #define op(code) \
    if(unlikely(rs != cl)) throw; \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0xd3); \
    emit.modrm(3, code, rt & 7);
  auto rol(reg32 rt, reg8 rs) { op(0); }
  auto ror(reg32 rt, reg8 rs) { op(1); }
  auto rcl(reg32 rt, reg8 rs) { op(2); }
  auto rcr(reg32 rt, reg8 rs) { op(3); }
  auto shl(reg32 rt, reg8 rs) { op(4); }
  auto shr(reg32 rt, reg8 rs) { op(5); }
  auto sal(reg32 rt, reg8 rs) { op(6); }
  auto sar(reg32 rt, reg8 rs) { op(7); }
  #undef op

  #define op(code) \
    emit.rex(1, 0, 0, rt & 8); \
    emit.byte(0xc1); \
    emit.modrm(3, code, rt & 7); \
    emit.byte(is.data);
  auto rol(reg64 rt, imm8 is) { op(0); }
  auto ror(reg64 rt, imm8 is) { op(1); }
  auto rcl(reg64 rt, imm8 is) { op(2); }
  auto rcr(reg64 rt, imm8 is) { op(3); }
  auto shl(reg64 rt, imm8 is) { op(4); }
  auto shr(reg64 rt, imm8 is) { op(5); }
  auto sal(reg64 rt, imm8 is) { op(6); }
  auto sar(reg64 rt, imm8 is) { op(7); }
  #undef op

  #define op(code) \
    if(unlikely(rs != cl)) throw; \
    emit.rex(1, 0, 0, rt & 8); \
    emit.byte(0xd3); \
    emit.modrm(3, code, rt & 7);
  auto rol(reg64 rt, reg8 rs) { op(0); }
  auto ror(reg64 rt, reg8 rs) { op(1); }
  auto rcl(reg64 rt, reg8 rs) { op(2); }
  auto rcr(reg64 rt, reg8 rs) { op(3); }
  auto shl(reg64 rt, reg8 rs) { op(4); }
  auto shr(reg64 rt, reg8 rs) { op(5); }
  auto sal(reg64 rt, reg8 rs) { op(6); }
  auto sar(reg64 rt, reg8 rs) { op(7); }
  #undef op

  //push reg
  auto push(reg64 rt) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0x50 | rt & 7);
  }

  //pop reg
  auto pop(reg64 rt) {
    emit.rex(0, 0, 0, rt & 8);
    emit.byte(0x58 | rt & 7);
  }

  #define op(code) \
    emit.rex(0, rs & 8, 0, rt & 8); \
    emit.byte(code); \
    emit.modrm(3, rs & 7, rt & 7);
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
    emit.byte(0x66); \
    emit.rex(0, rs & 8, 0, rt & 8); \
    emit.byte(code); \
    emit.modrm(3, rs & 7, rt & 7);
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
    emit.rex(0, rs & 8, 0, rt & 8); \
    emit.byte(code); \
    emit.modrm(3, rs & 7, rt & 7);
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
    emit.rex(1, rs & 8, 0, rt & 8); \
    emit.byte(code); \
    emit.modrm(3, rs & 7, rt & 7);
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
  #undef op

  #define op(code) \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0x83); \
    emit.modrm(3, code, rt & 7); \
    emit.byte(is.data);
  auto adc(reg32 rt, imm8 is) { op(2); }
  auto add(reg32 rt, imm8 is) { op(0); }
  auto and(reg32 rt, imm8 is) { op(4); }
  auto cmp(reg32 rt, imm8 is) { op(7); }
  auto or (reg32 rt, imm8 is) { op(1); }
  auto sbb(reg32 rt, imm8 is) { op(3); }
  auto sub(reg32 rt, imm8 is) { op(5); }
  auto xor(reg32 rt, imm8 is) { op(6); }
  #undef op

  #define op(code) \
    emit.rex(1, 0, 0, rt & 8); \
    emit.byte(0x83); \
    emit.modrm(3, code, rt & 7); \
    emit.byte(is.data);
  auto adc(reg64 rt, imm8 is) { op(2); }
  auto add(reg64 rt, imm8 is) { op(0); }
  auto and(reg64 rt, imm8 is) { op(4); }
  auto cmp(reg64 rt, imm8 is) { op(7); }
  auto or (reg64 rt, imm8 is) { op(1); }
  auto sbb(reg64 rt, imm8 is) { op(3); }
  auto sub(reg64 rt, imm8 is) { op(5); }
  auto xor(reg64 rt, imm8 is) { op(6); }
  #undef op

  #define op(code, group) \
    if(rt == al) { \
      emit.byte(code); \
      emit.byte(is.data); \
    } else { \
      emit.rex(0, 0, 0, rt & 8); \
      emit.byte(0x80); \
      emit.modrm(3, group, rt & 7); \
      emit.byte(is.data); \
    }
  auto adc(reg8 rt, imm8 is) { op(0x14, 2); }
  auto add(reg8 rt, imm8 is) { op(0x04, 0); }
  auto and(reg8 rt, imm8 is) { op(0x24, 4); }
  auto cmp(reg8 rt, imm8 is) { op(0x3c, 7); }
  auto or (reg8 rt, imm8 is) { op(0x0c, 1); }
  auto sbb(reg8 rt, imm8 is) { op(0x1c, 3); }
  auto sub(reg8 rt, imm8 is) { op(0x2c, 5); }
  auto xor(reg8 rt, imm8 is) { op(0x34, 6); }
  #undef op

  #define op(code, group) \
    if(rt == eax) { \
      emit.byte(code); \
      emit.dword(is.data); \
    } else { \
      emit.rex(0, 0, 0, rt & 8); \
      emit.byte(0x81); \
      emit.modrm(3, group, rt & 7); \
      emit.dword(is.data); \
    }
  auto adc(reg32 rt, imm32 is) { op(0x15, 2); }
  auto add(reg32 rt, imm32 is) { op(0x05, 0); }
  auto and(reg32 rt, imm32 is) { op(0x25, 4); }
  auto cmp(reg32 rt, imm32 is) { op(0x3d, 7); }
  auto or (reg32 rt, imm32 is) { op(0x0d, 1); }
  auto sbb(reg32 rt, imm32 is) { op(0x1d, 3); }
  auto sub(reg32 rt, imm32 is) { op(0x2d, 5); }
  auto xor(reg32 rt, imm32 is) { op(0x35, 6); }
  #undef op

  #define op(code) \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0xf7); \
    emit.modrm(3, code, rt & 7);
  auto not (reg32 rt) { op(2); }
  auto neg (reg32 rt) { op(3); }
  auto mul (reg32 rt) { op(4); }
  auto imul(reg32 rt) { op(5); }
  auto div (reg32 rt) { op(6); }
  auto idiv(reg32 rt) { op(7); }
  #undef op

  #define op(code) \
    emit.rex(1, 0, 0, rt & 8); \
    emit.byte(0xf7); \
    emit.modrm(3, code, rt & 7);
  auto not (reg64 rt) { op(2); }
  auto neg (reg64 rt) { op(3); }
  auto mul (reg64 rt) { op(4); }
  auto imul(reg64 rt) { op(5); }
  auto div (reg64 rt) { op(6); }
  auto idiv(reg64 rt) { op(7); }
  #undef op

  #define op(code) \
    emit.byte(code); \
    emit.byte(it.data);
  auto jmp(imm8 it) { op(0xeb); }
  auto jnz(imm8 it) { op(0x75); }
  auto jz (imm8 it) { op(0x74); }
  #undef op

  //op reg8
  #define op(code) \
    emit.rex(0, 0, 0, rt & 8); \
    emit.byte(0x0f); \
    emit.byte(code); \
    emit.modrm(3, 0, rt & 7);
  auto seta (reg8 rt) { op(0x97); }
  auto setbe(reg8 rt) { op(0x96); }
  auto setb (reg8 rt) { op(0x92); }
  auto setc (reg8 rt) { op(0x92); }
  auto setg (reg8 rt) { op(0x9f); }
  auto setge(reg8 rt) { op(0x9d); }
  auto setl (reg8 rt) { op(0x9c); }
  auto setle(reg8 rt) { op(0x9e); }
  auto setnc(reg8 rt) { op(0x93); }
  auto setno(reg8 rt) { op(0x91); }
  auto setnp(reg8 rt) { op(0x9b); }
  auto setns(reg8 rt) { op(0x99); }
  auto setnz(reg8 rt) { op(0x95); }
  auto seto (reg8 rt) { op(0x90); }
  auto setp (reg8 rt) { op(0x9a); }
  auto sets (reg8 rt) { op(0x98); }
  auto setz (reg8 rt) { op(0x94); }
  #undef op

  //op [reg64]
  #define op(code) \
    emit.rex(0, 0, 0, dt.reg & 8); \
    emit.byte(0x0f); \
    emit.byte(code); \
    if(dt.reg == rsp || dt.reg == r12) { \
      emit.modrm(0, 0, dt.reg & 7); \
      emit.sib(0, 4, 4); \
    } else if(dt.reg == rbp || dt.reg == r13) { \
      emit.modrm(1, 0, dt.reg & 7); \
      emit.byte(0x00); \
    } else { \
      emit.modrm(0, 0, dt.reg & 7); \
    }
  auto seta (dis dt) { op(0x97); }
  auto setbe(dis dt) { op(0x96); }
  auto setb (dis dt) { op(0x92); }
  auto setc (dis dt) { op(0x92); }
  auto setg (dis dt) { op(0x9f); }
  auto setge(dis dt) { op(0x9d); }
  auto setl (dis dt) { op(0x9c); }
  auto setle(dis dt) { op(0x9e); }
  auto setnc(dis dt) { op(0x93); }
  auto setno(dis dt) { op(0x91); }
  auto setnp(dis dt) { op(0x9b); }
  auto setns(dis dt) { op(0x99); }
  auto setnz(dis dt) { op(0x95); }
  auto seto (dis dt) { op(0x90); }
  auto setp (dis dt) { op(0x9a); }
  auto sets (dis dt) { op(0x98); }
  auto setz (dis dt) { op(0x94); }
  #undef op

  //op [reg64+imm8]
  #define op(code) \
    emit.rex(0, 0, 0, dt.reg & 8); \
    emit.byte(0x0f); \
    emit.byte(code); \
    emit.modrm(1, 0, dt.reg & 7); \
    if(dt.reg == rsp || dt.reg == r12) { \
      emit.sib(0, 4, 4); \
    } \
    emit.byte(dt.imm);
  auto seta (dis8 dt) { op(0x97); }
  auto setbe(dis8 dt) { op(0x96); }
  auto setb (dis8 dt) { op(0x92); }
  auto setc (dis8 dt) { op(0x92); }
  auto setg (dis8 dt) { op(0x9f); }
  auto setge(dis8 dt) { op(0x9d); }
  auto setl (dis8 dt) { op(0x9c); }
  auto setle(dis8 dt) { op(0x9e); }
  auto setnc(dis8 dt) { op(0x93); }
  auto setno(dis8 dt) { op(0x91); }
  auto setnp(dis8 dt) { op(0x9b); }
  auto setns(dis8 dt) { op(0x99); }
  auto setnz(dis8 dt) { op(0x95); }
  auto seto (dis8 dt) { op(0x90); }
  auto setp (dis8 dt) { op(0x9a); }
  auto sets (dis8 dt) { op(0x98); }
  auto setz (dis8 dt) { op(0x94); }
  #undef op
//};
