auto HG51B::disassembleInstruction(maybe<uint15> _pb, maybe<uint8> _pc) -> string {
  uint15 pb = _pb ? *_pb : (uint15)r.pb;
  uint8 pc = _pc ? *_pc : (uint8)r.pc;

  return {};  //todo
}

auto HG51B::disassembleADD(uint7 reg, uint5 shift) -> string {
  return {"add   "};
}

auto HG51B::disassembleADD(uint8 imm, uint5 shift) -> string {
  return {"add   "};
}

auto HG51B::disassembleAND(uint7 reg, uint5 shift) -> string {
  return {"and   "};
}

auto HG51B::disassembleAND(uint8 imm, uint5 shift) -> string {
  return {"and   "};
}

auto HG51B::disassembleASR(uint7 reg) -> string {
  return {"asr   "};
}

auto HG51B::disassembleASR(uint5 imm) -> string {
  return {"imm   "};
}

auto HG51B::disassembleCLEAR() -> string {
  return {"clear "};
}

auto HG51B::disassembleCMP(uint7 reg, uint5 shift) -> string {
  return {"cmp   "};
}

auto HG51B::disassembleCMP(uint8 imm, uint5 shift) -> string {
  return {"cmp   "};
}

auto HG51B::disassembleCMPR(uint7 reg, uint5 shift) -> string {
  return {"cmpr  "};
}

auto HG51B::disassembleCMPR(uint8 imm, uint5 shift) -> string {
  return {"cmpr  "};
}

auto HG51B::disassembleHALT() -> string {
  return {"halt  "};
}

auto HG51B::disassembleINC(uint24& reg) -> string {
  return {"inc   "};
}

auto HG51B::disassembleJMP(uint8 data, uint1 far, const uint1& take) -> string {
  return {"jmp   "};
}

auto HG51B::disassembleJSR(uint8 data, uint1 far, const uint1& take) -> string {
  return {"jsr   "};
}

auto HG51B::disassembleLD(uint24& out, uint7 reg) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLD(uint15& out, uint4 reg) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLD(uint24& out, uint8 imm) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLD(uint15& out, uint8 imm) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLDL(uint15& out, uint8 imm) -> string {
  return {"ldl   "};
}

auto HG51B::disassembleLDH(uint15& out, uint7 imm) -> string {
  return {"ldh   "};
}

auto HG51B::disassembleMUL(uint7 reg) -> string {
  return {"mul   "};
}

auto HG51B::disassembleMUL(uint8 imm) -> string {
  return {"mul   "};
}

auto HG51B::disassembleNOP() -> string {
  return {"nop   "};
}

auto HG51B::disassembleOR(uint7 reg, uint5 shift) -> string {
  return {"or    "};
}

auto HG51B::disassembleOR(uint8 imm, uint5 shift) -> string {
  return {"or    "};
}

auto HG51B::disassembleRDRAM(uint2 byte, uint24& a) -> string {
  return {"rdram "};
}

auto HG51B::disassembleRDRAM(uint2 byte, uint8 imm) -> string {
  return {"rdram "};
}

auto HG51B::disassembleRDROM(uint24& reg) -> string {
  return {"rdrom "};
}

auto HG51B::disassembleRDROM(uint10 imm) -> string {
  return {"rdrom "};
}

auto HG51B::disassembleROR(uint7 reg) -> string {
  return {"ror   "};
}

auto HG51B::disassembleROR(uint5 imm) -> string {
  return {"ror   "};
}

auto HG51B::disassembleRTS() -> string {
  return {"rts   "};
}

auto HG51B::disassembleSHL(uint7 reg) -> string {
  return {"shl   "};
}

auto HG51B::disassembleSHL(uint5 imm) -> string {
  return {"shl   "};
}

auto HG51B::disassembleSHR(uint7 reg) -> string {
  return {"shr   "};
}

auto HG51B::disassembleSHR(uint5 imm) -> string {
  return {"shr   "};
}

auto HG51B::disassembleSKIP(uint1 take, const uint1& flag) -> string {
  return {"skip  "};
}

auto HG51B::disassembleST(uint7 reg, uint24& in) -> string {
  return {"st    "};
}

auto HG51B::disassembleSUB(uint7 reg, uint5 shift) -> string {
  return {"sub   "};
}

auto HG51B::disassembleSUB(uint8 imm, uint5 shift) -> string {
  return {"sub   "};
}

auto HG51B::disassembleSUBR(uint7 reg, uint5 shift) -> string {
  return {"subr  "};
}

auto HG51B::disassembleSUBR(uint8 imm, uint5 shift) -> string {
  return {"subr  "};
}

auto HG51B::disassembleSWAP(uint24& a, uint4 reg) -> string {
  return {"swap  "};
}

auto HG51B::disassembleSXB() -> string {
  return {"sxb   "};
}

auto HG51B::disassembleSXW() -> string {
  return {"sxw   "};
}

auto HG51B::disassembleWAIT() -> string {
  return {"wait  "};
}

auto HG51B::disassembleWRRAM(uint2 byte, uint24& a) -> string {
  return {"wrram "};
}

auto HG51B::disassembleWRRAM(uint2 byte, uint8 imm) -> string {
  return {"wrram "};
}

auto HG51B::disassembleXNOR(uint7 reg, uint5 shift) -> string {
  return {"xnor  "};
}

auto HG51B::disassembleXNOR(uint8 imm, uint5 shift) -> string {
  return {"xnor  "};
}

auto HG51B::disassembleXOR(uint7 reg, uint5 shift) -> string {
  return {"xor   "};
}

auto HG51B::disassembleXOR(uint8 imm, uint5 shift) -> string {
  return {"xor   "};
}

auto HG51B::disassembleContext() -> string {
  string output;

  output.append("a:", hex(r.a, 6L), " ");
  output.append("p:", hex(r.p, 4L), " ");
  for(uint n : range(16)) {
    output.append("r", n, ":", hex(r.gpr[n], 6L), " ");
  }

  output.append(r.n ? "N" : "n");
  output.append(r.z ? "Z" : "z");
  output.append(r.c ? "C" : "c");
  output.append(r.v ? "V" : "v");
  output.append(r.i ? "I" : "i");

  return output;
}
