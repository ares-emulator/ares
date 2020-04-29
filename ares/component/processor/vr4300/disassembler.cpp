#define _sa string{"#", opcode >>  6 & 31}
#define _rd string{"r", opcode >> 11 & 31}
#define _rt string{"r", opcode >> 16 & 31}
#define _rs string{"r", opcode >> 21 & 31}
#define _imm16i string{"#0x",  int16_t(opcode)}
#define _imm16u string{"#0x", uint16_t(opcode)}
#define _imm26u string{"#0x", hex(opcode & 0x03ffffff, 8L)}
#define _offsetbase string{"#0x", int16_t(opcode), "(r", opcode >> 21 & 31 ,")"}

auto VR4300::disassembleInstruction() -> string {
  vector<string> v = disassembleInstructionDecode();
  if(!v) v.append("invalid");
  auto opcode = pad(v.takeFirst(), -8L);
  return {opcode, v.merge(",")};
}

auto VR4300::disassembleInstructionDecode() -> vector<string> {
  switch(opcode >> 26) {
  case 0x00: return disassembleInstructionSpecial();
  case 0x01: return disassembleInstructionRegisterImmediate();
  case 0x02: return {"j", _imm26u};
  case 0x03: return {"jal", _imm26u};
  case 0x04: return {"beq", _rs, _rt, _imm16i};
  case 0x05: return {"bne", _rs, _rt, _imm16i};
  case 0x06: return {"blez", _rs, _imm16i};
  case 0x07: return {"bgtz", _rs, _imm16i};
  case 0x08: return {"addi", _rt, _rs, _imm16i};
  case 0x09: return {"addiu", _rt, _rs, _imm16i};
  case 0x0a: return {"slti", _rt, _rs, _imm16i};
  case 0x0b: return {"sltiu", _rt, _rs, _imm16i};
  case 0x0c: return {"andi", _rt, _rs, _imm16i};
  case 0x0d: return {"ori", _rt, _rs, _imm16i};
  case 0x0e: return {"xori", _rt, _rs, _imm16i};
  case 0x0f: return {"lui", _rt, _imm16i};
  case 0x10: return {"cop0"};
  case 0x11: return {"cop1"};
  case 0x12: return {"cop2"};
  case 0x13: return {"cop3"};
  case 0x14: return {"beql", _rs, _rt, _imm16i};
  case 0x15: return {"bnel", _rs, _rt, _imm16i};
  case 0x16: return {"blezl", _rs, _imm16i};
  case 0x17: return {"bgtzl", _rs, _imm16i};
  case 0x18: return {"daddi", _rt, _rs, _imm16i};
  case 0x19: return {"daddiu", _rt, _rs, _imm16i};
  case 0x1a: return {"ldl", _rt, _offsetbase};
  case 0x1b: return {"ldr", _rt, _offsetbase};
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
  }
  return {"invalid"};
}

auto VR4300::disassembleInstructionSpecial() -> vector<string> {
  switch(opcode & 0x3f) {
  case 0x00: return {"sll", _rd, _rt, _sa};
  case 0x01: break;
  case 0x02: return {"srl", _rd, _rt, _sa};
  case 0x03: return {"sra", _rd, _rt, _sa};
  case 0x04: return {"sllv", _rd, _rt, _rs};
  case 0x05: break;
  case 0x06: return {"srlv", _rd, _rt, _rs};
  case 0x07: return {"srav", _rd, _rt, _rs};
  case 0x08: return {"jr", _rs};
  case 0x09: return {"jalr", _rd, _rs};
  case 0x0a: break;
  case 0x0b: break;
  case 0x0c: return {"syscall"};
  case 0x0d: return {"break"};
  case 0x0e: break;
  case 0x0f: return {"sync"};
  case 0x10: return {"mfhi"};
  case 0x11: return {"mthi"};
  case 0x12: return {"mflo"};
  case 0x13: return {"mtlo"};
  case 0x14: return {"dsllv", _rd, _rt, _rs};
  case 0x15: break;
  case 0x16: return {"dsrlv", _rd, _rt, _rs};
  case 0x17: return {"dsrav", _rd, _rt, _rs};
  case 0x18: return {"mult", _rs, _rt};
  case 0x19: return {"multu", _rs, _rt};
  case 0x1a: return {"div", _rs, _rt};
  case 0x1b: return {"divu", _rs, _rt};
  case 0x1c: return {"dmult", _rs, _rt};
  case 0x1d: return {"dmultu", _rs, _rt};
  case 0x1e: return {"ddiv", _rs, _rt};
  case 0x1f: return {"ddivu", _rs, _rt};
  case 0x20: return {"add", _rd, _rs, _rt};
  case 0x21: return {"addu", _rd, _rs, _rt};
  case 0x22: return {"sub", _rd, _rs, _rt};
  case 0x23: return {"subu", _rd, _rs, _rt};
  case 0x24: return {"and", _rd, _rs, _rt};
  case 0x25: return {"or", _rd, _rs, _rt};
  case 0x26: return {"xor", _rd, _rs, _rt};
  case 0x27: return {"nor", _rd, _rs, _rt};
  case 0x28: break;
  case 0x29: break;
  case 0x2a: return {"slt", _rd, _rs, _rt};
  case 0x2b: return {"sltu", _rd, _rs, _rt};
  case 0x2c: return {"dadd", _rd, _rs, _rt};
  case 0x2d: return {"daddu", _rd, _rs, _rt};
  case 0x2e: return {"dsub", _rd, _rs, _rt};
  case 0x2f: return {"dsubu", _rd, _rs, _rt};
  case 0x30: return {"tge", _rs, _rt};
  case 0x31: return {"tgeu", _rs, _rt};
  case 0x32: return {"tlt", _rs, _rt};
  case 0x33: return {"tltu", _rs, _rt};
  case 0x34: return {"teq", _rs, _rt};
  case 0x35: break;
  case 0x36: return {"tne", _rs, _rt};
  case 0x37: break;
  case 0x38: return {"dsll", _rd, _rt, _sa};
  case 0x39: break;
  case 0x3a: return {"dsrl", _rd, _rt, _sa};
  case 0x3b: return {"dsra", _rd, _rt, _sa};
  case 0x3c: return {"dsll32", _rd, _rt, _sa};
  case 0x3d: break;
  case 0x3e: return {"dsrl32", _rd, _rt, _sa};
  case 0x3f: return {"dsra32", _rd, _rt, _sa};
  }
  return {"invalid"};
}

auto VR4300::disassembleInstructionRegisterImmediate() -> vector<string> {
  switch(opcode >> 16 & 0x1f) {
  case 0x00: return {"bltz", _rs, _imm16i};
  case 0x01: return {"bgez", _rs, _imm16i};
  case 0x02: return {"bltzl", _rs, _imm16i};
  case 0x03: return {"bgezl", _rs, _imm16i};
  case 0x04: break;
  case 0x05: break;
  case 0x06: break;
  case 0x07: break;
  case 0x08: return {"tgei", _rs, _imm16i};
  case 0x09: return {"tgeiu", _rs, _imm16u};
  case 0x0a: return {"tlti", _rs, _imm16i};
  case 0x0b: return {"tltiu", _rs, _imm16u};
  case 0x0c: return {"teqi", _rs, _imm16i};
  case 0x0d: break;
  case 0x0e: return {"tnei", _rs, _imm16u};
  case 0x0f: break;
  case 0x10: return {"bltzal", _rs, _imm16i};
  case 0x11: return {"bgezal", _rs, _imm16i};
  case 0x12: return {"bltzall", _rs, _imm16i};
  case 0x13: return {"bgezall", _rs, _imm16i};
  case 0x14: break;
  case 0x15: break;
  case 0x16: break;
  case 0x17: break;
  case 0x18: break;
  case 0x19: break;
  case 0x1a: break;
  case 0x1b: break;
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
  }
  return {"invalid"};
}

auto VR4300::disassembleContext() -> string {
  string s;
  for(uint n : range(32)) {
    s.append("r", n, ":", hex(GPR[n], 16L), " ");
  }
  s.append("hi:", hex(HI, 16L), " ");
  s.append("lo:", hex(LO, 16L));
  return s;
}
