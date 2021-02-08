auto HG51B::disassembleInstruction(maybe<n15> _pb, maybe<n8> _pc) -> string {
  n15 pb = _pb ? *_pb : (n15)r.pb;
  n8  pc = _pc ? *_pc : (n8 )r.pc;

  return {};  //todo
}

auto HG51B::disassembleADD(n7 reg, n5 shift) -> string {
  return {"add   "};
}

auto HG51B::disassembleADD(n8 imm, n5 shift) -> string {
  return {"add   "};
}

auto HG51B::disassembleAND(n7 reg, n5 shift) -> string {
  return {"and   "};
}

auto HG51B::disassembleAND(n8 imm, n5 shift) -> string {
  return {"and   "};
}

auto HG51B::disassembleASR(n7 reg) -> string {
  return {"asr   "};
}

auto HG51B::disassembleASR(n5 imm) -> string {
  return {"imm   "};
}

auto HG51B::disassembleCLEAR() -> string {
  return {"clear "};
}

auto HG51B::disassembleCMP(n7 reg, n5 shift) -> string {
  return {"cmp   "};
}

auto HG51B::disassembleCMP(n8 imm, n5 shift) -> string {
  return {"cmp   "};
}

auto HG51B::disassembleCMPR(n7 reg, n5 shift) -> string {
  return {"cmpr  "};
}

auto HG51B::disassembleCMPR(n8 imm, n5 shift) -> string {
  return {"cmpr  "};
}

auto HG51B::disassembleHALT() -> string {
  return {"halt  "};
}

auto HG51B::disassembleINC(n24& reg) -> string {
  return {"inc   "};
}

auto HG51B::disassembleJMP(n8 data, n1 far, const n1& take) -> string {
  return {"jmp   "};
}

auto HG51B::disassembleJSR(n8 data, n1 far, const n1& take) -> string {
  return {"jsr   "};
}

auto HG51B::disassembleLD(n24& out, n7 reg) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLD(n15& out, n4 reg) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLD(n24& out, n8 imm) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLD(n15& out, n8 imm) -> string {
  return {"ld    "};
}

auto HG51B::disassembleLDL(n15& out, n8 imm) -> string {
  return {"ldl   "};
}

auto HG51B::disassembleLDH(n15& out, n7 imm) -> string {
  return {"ldh   "};
}

auto HG51B::disassembleMUL(n7 reg) -> string {
  return {"mul   "};
}

auto HG51B::disassembleMUL(n8 imm) -> string {
  return {"mul   "};
}

auto HG51B::disassembleNOP() -> string {
  return {"nop   "};
}

auto HG51B::disassembleOR(n7 reg, n5 shift) -> string {
  return {"or    "};
}

auto HG51B::disassembleOR(n8 imm, n5 shift) -> string {
  return {"or    "};
}

auto HG51B::disassembleRDRAM(n2 byte, n24& a) -> string {
  return {"rdram "};
}

auto HG51B::disassembleRDRAM(n2 byte, n8 imm) -> string {
  return {"rdram "};
}

auto HG51B::disassembleRDROM(n24& reg) -> string {
  return {"rdrom "};
}

auto HG51B::disassembleRDROM(n10 imm) -> string {
  return {"rdrom "};
}

auto HG51B::disassembleROR(n7 reg) -> string {
  return {"ror   "};
}

auto HG51B::disassembleROR(n5 imm) -> string {
  return {"ror   "};
}

auto HG51B::disassembleRTS() -> string {
  return {"rts   "};
}

auto HG51B::disassembleSHL(n7 reg) -> string {
  return {"shl   "};
}

auto HG51B::disassembleSHL(n5 imm) -> string {
  return {"shl   "};
}

auto HG51B::disassembleSHR(n7 reg) -> string {
  return {"shr   "};
}

auto HG51B::disassembleSHR(n5 imm) -> string {
  return {"shr   "};
}

auto HG51B::disassembleSKIP(n1 take, const n1& flag) -> string {
  return {"skip  "};
}

auto HG51B::disassembleST(n7 reg, n24& in) -> string {
  return {"st    "};
}

auto HG51B::disassembleSUB(n7 reg, n5 shift) -> string {
  return {"sub   "};
}

auto HG51B::disassembleSUB(n8 imm, n5 shift) -> string {
  return {"sub   "};
}

auto HG51B::disassembleSUBR(n7 reg, n5 shift) -> string {
  return {"subr  "};
}

auto HG51B::disassembleSUBR(n8 imm, n5 shift) -> string {
  return {"subr  "};
}

auto HG51B::disassembleSWAP(n24& a, n4 reg) -> string {
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

auto HG51B::disassembleWRRAM(n2 byte, n24& a) -> string {
  return {"wrram "};
}

auto HG51B::disassembleWRRAM(n2 byte, n8 imm) -> string {
  return {"wrram "};
}

auto HG51B::disassembleXNOR(n7 reg, n5 shift) -> string {
  return {"xnor  "};
}

auto HG51B::disassembleXNOR(n8 imm, n5 shift) -> string {
  return {"xnor  "};
}

auto HG51B::disassembleXOR(n7 reg, n5 shift) -> string {
  return {"xor   "};
}

auto HG51B::disassembleXOR(n8 imm, n5 shift) -> string {
  return {"xor   "};
}

auto HG51B::disassembleContext() -> string {
  string output;

  output.append("a:", hex(r.a, 6L), " ");
  output.append("p:", hex(r.p, 4L), " ");
  for(u32 n : range(16)) {
    output.append("r", n, ":", hex(r.gpr[n], 6L), " ");
  }

  output.append(r.n ? "N" : "n");
  output.append(r.z ? "Z" : "z");
  output.append(r.c ? "C" : "c");
  output.append(r.v ? "V" : "v");
  output.append(r.i ? "I" : "i");

  return output;
}
