auto HG51B::push() -> void {
  stack[7] = stack[6];
  stack[6] = stack[5];
  stack[5] = stack[4];
  stack[4] = stack[3];
  stack[3] = stack[2];
  stack[2] = stack[1];
  stack[1] = stack[0];
  stack[0] = r.pb << 8 | r.pc << 0;
}

auto HG51B::pull() -> void {
  auto pc  = stack[0];
  stack[0] = stack[1];
  stack[1] = stack[2];
  stack[2] = stack[3];
  stack[3] = stack[4];
  stack[4] = stack[5];
  stack[5] = stack[6];
  stack[6] = stack[7];
  stack[7] = 0x0000;

  r.pb = pc >> 8;
  r.pc = pc >> 0;
}

//

auto HG51B::algorithmADD(n24 x, n24 y) -> n24 {
  s32 z = x + y;
  r.n = z & 0x800000;
  r.z = (n24)z == 0;
  r.c = z > 0xffffff;
  r.v = ~(x ^ y) & (x ^ z) & 0x800000;
  return z;
}

auto HG51B::algorithmAND(n24 x, n24 y) -> n24 {
  x = x & y;
  r.n = x & 0x800000;
  r.z = x == 0;
  return x;
}

auto HG51B::algorithmASR(n24 a, n5 s) -> n24 {
  if(s > 24) s = 0;
  a = (i24)a >> s;
  r.n = a & 0x800000;
  r.z = a == 0;
  return a;
}

auto HG51B::algorithmMUL(i24 x, i24 y) -> n48 {
  return (i48)x * (i48)y;
}

auto HG51B::algorithmOR(n24 x, n24 y) -> n24 {
  x = x | y;
  r.n = x & 0x800000;
  r.z = x == 0;
  return x;
}

auto HG51B::algorithmROR(n24 a, n5 s) -> n24 {
  if(s > 24) s = 0;
  a = (a >> s) | (a << 24 - s);
  r.n = a & 0x800000;
  r.z = a == 0;
  return a;
}

auto HG51B::algorithmSHL(n24 a, n5 s) -> n24 {
  if(s > 24) s = 0;
  a = a << s;
  r.n = a & 0x800000;
  r.z = a == 0;
  return a;
}

auto HG51B::algorithmSHR(n24 a, n5 s) -> n24 {
  if(s > 24) s = 0;
  a = a >> s;
  r.n = a & 0x800000;
  r.z = a == 0;
  return a;
}

auto HG51B::algorithmSUB(n24 x, n24 y) -> n24 {
  s32 z = x - y;
  r.n = z & 0x800000;
  r.z = (n24)z == 0;
  r.c = z >= 0;
  r.v = ~(x ^ y) & (x ^ z) & 0x800000;
  return z;
}

auto HG51B::algorithmSX(n24 x) -> n24 {
  r.n = x & 0x800000;
  r.z = x == 0;
  return x;
}

auto HG51B::algorithmXNOR(n24 x, n24 y) -> n24 {
  x = ~x ^ y;
  r.n = x & 0x800000;
  r.z = x == 0;
  return x;
}

auto HG51B::algorithmXOR(n24 x, n24 y) -> n24 {
  x = x ^ y;
  r.n = x & 0x800000;
  r.z = x == 0;
  return x;
}

//

auto HG51B::instructionADD(n7 reg, n5 shift) -> void {
  r.a = algorithmADD(r.a << shift, readRegister(reg));
}

auto HG51B::instructionADD(n8 imm, n5 shift) -> void {
  r.a = algorithmADD(r.a << shift, imm);
}

auto HG51B::instructionAND(n7 reg, n5 shift) -> void {
  r.a = algorithmAND(r.a << shift, readRegister(reg));
}

auto HG51B::instructionAND(n8 imm, n5 shift) -> void {
  r.a = algorithmAND(r.a << shift, imm);
}

auto HG51B::instructionASR(n7 reg) -> void {
  r.a = algorithmASR(r.a, readRegister(reg));
}

auto HG51B::instructionASR(n5 imm) -> void {
  r.a = algorithmASR(r.a, imm);
}

auto HG51B::instructionCLEAR() -> void {
  r.a = 0;
  r.p = 0;
  r.ram = 0;
  r.dpr = 0;
}

auto HG51B::instructionCMP(n7 reg, n5 shift) -> void {
  algorithmSUB(r.a << shift, readRegister(reg));
}

auto HG51B::instructionCMP(n8 imm, n5 shift) -> void {
  algorithmSUB(r.a << shift, imm);
}

auto HG51B::instructionCMPR(n7 reg, n5 shift) -> void {
  algorithmSUB(readRegister(reg), r.a << shift);
}

auto HG51B::instructionCMPR(n8 imm, n5 shift) -> void {
  algorithmSUB(imm, r.a << shift);
}

auto HG51B::instructionHALT() -> void {
  halt();
}

auto HG51B::instructionINC(n24& reg) -> void {
  reg++;
}

auto HG51B::instructionJMP(n8 data, n1 far, const n1& take) -> void {
  if(!take) return;
  if(far) r.pb = r.p;
  r.pc = data;
  step(2);
}

auto HG51B::instructionJSR(n8 data, n1 far, const n1& take) -> void {
  if(!take) return;
  push();
  if(far) r.pb = r.p;
  r.pc = data;
  step(2);
}

auto HG51B::instructionLD(n24& out, n7 reg) -> void {
  out = readRegister(reg);
}

auto HG51B::instructionLD(n15& out, n4 reg) -> void {
  out = r.gpr[reg];
}

auto HG51B::instructionLD(n24& out, n8 imm) -> void {
  out = imm;
}

auto HG51B::instructionLD(n15& out, n8 imm) -> void {
  out = imm;
}

auto HG51B::instructionLDL(n15& out, n8 imm) -> void {
  out.bit(0,7) = imm;
}

auto HG51B::instructionLDH(n15& out, n7 imm) -> void {
  out.bit(8,14) = imm;
}

auto HG51B::instructionMUL(n7 reg) -> void {
  r.mul = algorithmMUL(r.a, readRegister(reg));
}

auto HG51B::instructionMUL(n8 imm) -> void {
  r.mul = algorithmMUL(r.a, imm);
}

auto HG51B::instructionNOP() -> void {
}

auto HG51B::instructionOR(n7 reg, n5 shift) -> void {
  r.a = algorithmOR(r.a << shift, readRegister(reg));
}

auto HG51B::instructionOR(n8 imm, n5 shift) -> void {
  r.a = algorithmOR(r.a << shift, imm);
}

auto HG51B::instructionRDRAM(n2 byte, n24& a) -> void {
  n12 address = a;
  if(address >= 0xc00) address -= 0x400;
  r.ram.byte(byte) = dataRAM[address];
}

auto HG51B::instructionRDRAM(n2 byte, n8 imm) -> void {
  n12 address = r.dpr + imm;
  if(address >= 0xc00) address -= 0x400;
  r.ram.byte(byte) = dataRAM[address];
}

auto HG51B::instructionRDROM(n24& reg) -> void {
  r.rom = dataROM[(n10)reg];
}

auto HG51B::instructionRDROM(n10 imm) -> void {
  r.rom = dataROM[imm];
}

auto HG51B::instructionROR(n7 reg) -> void {
  r.a = algorithmROR(r.a, readRegister(reg));
}

auto HG51B::instructionROR(n5 imm) -> void {
  r.a = algorithmROR(r.a, imm);
}

auto HG51B::instructionRTS() -> void {
  pull();
  step(2);
}

auto HG51B::instructionSKIP(n1 take, const n1& flag) -> void {
  if(flag != take) return;
  advance();
  step(1);
}

auto HG51B::instructionSHL(n7 reg) -> void {
  r.a = algorithmSHL(r.a, readRegister(reg));
}

auto HG51B::instructionSHL(n5 imm) -> void {
  r.a = algorithmSHL(r.a, imm);
}

auto HG51B::instructionSHR(n7 reg) -> void {
  r.a = algorithmSHR(r.a, readRegister(reg));
}

auto HG51B::instructionSHR(n5 imm) -> void {
  r.a = algorithmSHR(r.a, imm);
}

auto HG51B::instructionST(n7 reg, n24& in) -> void {
  writeRegister(reg, in);
}

auto HG51B::instructionSUB(n7 reg, n5 shift) -> void {
  r.a = algorithmSUB(r.a << shift, readRegister(reg));
}

auto HG51B::instructionSUB(n8 imm, n5 shift) -> void {
  r.a = algorithmSUB(r.a << shift, imm);
}

auto HG51B::instructionSUBR(n7 reg, n5 shift) -> void {
  r.a = algorithmSUB(readRegister(reg), r.a << shift);
}

auto HG51B::instructionSUBR(n8 imm, n5 shift) -> void {
  r.a = algorithmSUB(imm, r.a << shift);
}

auto HG51B::instructionSWAP(n24& a, n4 reg) -> void {
  swap(a, r.gpr[reg]);
}

auto HG51B::instructionSXB() -> void {
  r.a = algorithmSX((i8)r.a);
}

auto HG51B::instructionSXW() -> void {
  r.a = algorithmSX((i16)r.a);
}

auto HG51B::instructionWAIT() -> void {
  if(!io.bus.enable) return;
  return step(io.bus.pending);
}

auto HG51B::instructionWRRAM(n2 byte, n24& a) -> void {
  n12 address = a;
  if(address >= 0xc00) address -= 0x400;
  switch(byte) {
  case 0: dataRAM[address] = r.ram >>  0; break;
  case 1: dataRAM[address] = r.ram >>  8; break;
  case 2: dataRAM[address] = r.ram >> 16; break;
  }
}

auto HG51B::instructionWRRAM(n2 byte, n8 imm) -> void {
  n12 address = r.dpr + imm;
  if(address >= 0xc00) address -= 0x400;
  switch(byte) {
  case 0: dataRAM[address] = r.ram >>  0; break;
  case 1: dataRAM[address] = r.ram >>  8; break;
  case 2: dataRAM[address] = r.ram >> 16; break;
  }
}

auto HG51B::instructionXNOR(n7 reg, n5 shift) -> void {
  r.a = algorithmXNOR(r.a << shift, readRegister(reg));
}

auto HG51B::instructionXNOR(n8 imm, n5 shift) -> void {
  r.a = algorithmXNOR(r.a << shift, imm);
}

auto HG51B::instructionXOR(n7 reg, n5 shift) -> void {
  r.a = algorithmXOR(r.a << shift, readRegister(reg));
}

auto HG51B::instructionXOR(n8 imm, n5 shift) -> void {
  r.a = algorithmXOR(r.a << shift, imm);
}
