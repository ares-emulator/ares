#define _sa string{"#", opcode >>  6 & 31}
#define _rd string{"r", opcode >> 11 & 31}
#define _rt string{"r", opcode >> 16 & 31}
#define _rs string{"r", opcode >> 21 & 31}

auto VR4300::disassembleInstruction() -> string {
  vector<string> v;

  switch(opcode >> 26) {
  case 0x00: v = disassembleInstructionSpecial(); break;
  }

  if(!v) v.append("invalid");
  auto opcode = pad(v.takeFirst(), -8L);
  return {opcode, v.merge(",")};
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
  return "invalid";
}

auto VR4300::disassembleContext() -> string {
  string s;
  for(uint n : range(32)) {
    s.append("r", n, ":", hex(context.gpr[n], 16L), " ");
  }
  s.append("hi:", hex(context.hi, 16L), " ");
  s.append("lo:", hex(context.lo, 16L));
  return s;
}
