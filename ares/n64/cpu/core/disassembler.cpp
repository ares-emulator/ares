auto CPU::disassembleInstruction() -> string {
  static const string registers[32] = {
    "$0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
  };

  static const string registersCOP0[32] = {
    "Index",    "Random",   "EntryLo0",    "EntryLo1",
    "Context",  "PageMask", "Wired",       "Unused7",
    "BadVAddr", "Count",    "EntryHi",     "Compare",
    "Status",   "Cause",    "EPC",         "PrID",
    "Config",   "LLAddr",   "WatchLo",     "WatchHi",
    "XContext", "Unused21", "Unused22",    "Unused23",
    "Unused24", "Unused25", "ParityError", "CacheError",
    "TagLo",    "TagHi",    "ErrorEPC",    "Unused31",
  };

  static const string registersCOP1[32] = {
     "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
     "f8",  "f9", "f10", "f11", "f12", "f13", "f14", "f15",
    "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
    "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  };

  u32 opcode = pipeline.instruction;
  u32 sa = opcode >>  6 & 31;
  u32 rd = opcode >> 11 & 31;
  u32 rt = opcode >> 16 & 31;
  u32 rs = opcode >> 21 & 31;

  auto branchAddress = [&]() -> string {
    u32 pc = PC + (i16(opcode) << 2);
    return {"$", hex(pc)};
  };

  auto offsetBase = [&]() -> string {
    return {"$", hex(u32(i16(opcode))), "(", registers[rs], ")"};
  };

  auto immediate16i = [&]() -> string {
    if(i16(opcode) < 0) return {"-$", hex(-i16(opcode))};
    return {"$", hex(u16(opcode))};
  };

  auto immediate16u = [&]() -> string {
    return {"$", hex(u16(opcode))};
  };

  auto immediate26u = [&]() -> string {
    return {"$", hex(u32(opcode & 0x3ffffff))};
  };

  auto immediate32i = [&]() -> string {
    return {"$", hex(u32(i16(opcode) << 16))};
  };

  function<vector<string> ()> disassemble;
  function<vector<string> ()> special;
  function<vector<string> ()> regimm;
  function<vector<string> ()> cop0;
  function<vector<string> ()> cop1;
  function<vector<string> ()> cop2;
  function<vector<string> ()> cop3;

  disassemble = [&]() -> vector<string> {
    switch(opcode >> 26) {
    case 0x00: return special();
    case 0x01: return regimm();
    case 0x02: return {"j",      immediate26u()};
    case 0x03: return {"jal",    immediate26u()};
    case 0x04: return {"beq",    registers[rs], registers[rt], branchAddress()};
    case 0x05: return {"bne",    registers[rs], registers[rt], branchAddress()};
    case 0x06: return {"blez",   registers[rs], immediate16i()};
    case 0x07: return {"bgtz",   registers[rs], immediate16i()};
    case 0x08: return {"addi",   registers[rt], registers[rs], immediate16i()};
    case 0x09: return {"addiu",  registers[rt], registers[rs], immediate16i()};
    case 0x0a: return {"slti",   registers[rt], registers[rs], immediate16i()};
    case 0x0b: return {"sltiu",  registers[rt], registers[rs], immediate16i()};
    case 0x0c: return {"andi",   registers[rt], registers[rs], immediate16i()};
    case 0x0d: return {"ori",    registers[rt], registers[rs], immediate16i()};
    case 0x0e: return {"xori",   registers[rt], registers[rs], immediate16i()};
    case 0x0f: return {"lui",    registers[rt], immediate32i()};
    case 0x10: return cop0();
    case 0x11: return cop1();
    case 0x12: return cop2();
    case 0x13: return cop3();
    case 0x14: return {"beql",   registers[rs], registers[rt], immediate16i()};
    case 0x15: return {"bnel",   registers[rs], registers[rt], immediate16i()};
    case 0x16: return {"blezl",  registers[rs], immediate16i()};
    case 0x17: return {"bgtzl",  registers[rs], immediate16i()};
    case 0x18: return {"daddi",  registers[rt], registers[rs], immediate16i()};
    case 0x19: return {"daddiu", registers[rt], registers[rs], immediate16i()};
    case 0x1a: return {"ldl",    registers[rt], offsetBase()};
    case 0x1b: return {"ldr",    registers[rt], offsetBase()};
    case 0x1c: break;
    case 0x1d: break;
    case 0x1e: break;
    case 0x1f: break;
    case 0x20: return {"lb",     registers[rt], offsetBase()};
    case 0x21: return {"lh",     registers[rt], offsetBase()};
    case 0x22: return {"lwl",    registers[rt], offsetBase()};
    case 0x23: return {"lw",     registers[rt], offsetBase()};
    case 0x24: return {"lbu",    registers[rt], offsetBase()};
    case 0x25: return {"lhu",    registers[rt], offsetBase()};
    case 0x26: return {"lwr",    registers[rt], offsetBase()};
    case 0x27: return {"lwu",    registers[rt], offsetBase()};
    case 0x28: return {"sb",     registers[rt], offsetBase()};
    case 0x29: return {"sh",     registers[rt], offsetBase()};
    case 0x2a: return {"swl",    registers[rt], offsetBase()};
    case 0x2b: return {"sw",     registers[rt], offsetBase()};
    case 0x2c: return {"sdl",    registers[rt], offsetBase()};
    case 0x2d: return {"sdr",    registers[rt], offsetBase()};
    case 0x2e: return {"swr",    registers[rt], offsetBase()};
    case 0x2f: return {"cache",  rt, offsetBase()};
    case 0x30: return {"ll",     registers[rt], offsetBase()};
    case 0x31: return {"lwc1",   registers[rt], offsetBase()};
    case 0x32: return {"lwc2",   registers[rt], offsetBase()};
    case 0x33: return {"lwc3",   registers[rt], offsetBase()};
    case 0x34: return {"lld",    registers[rt], offsetBase()};
    case 0x35: return {"ldc1",   registers[rt], offsetBase()};
    case 0x36: return {"ldc2",   registers[rt], offsetBase()};
    case 0x37: return {"ld",     registers[rt], offsetBase()};
    case 0x38: return {"sc",     registers[rt], offsetBase()};
    case 0x39: return {"swc1",   registers[rt], offsetBase()};
    case 0x3a: return {"swc2",   registers[rt], offsetBase()};
    case 0x3b: return {"swc3",   registers[rt], offsetBase()};
    case 0x3c: return {"scd",    registers[rt], offsetBase()};
    case 0x3d: return {"sdc1",   registers[rt], offsetBase()};
    case 0x3e: return {"sdc2",   registers[rt], offsetBase()};
    case 0x3f: return {"sd",     registers[rt], offsetBase()};
    }
    return {"invalid"};
  };

  special = [&]() -> vector<string> {
    switch(opcode & 0x3f) {
    case 0x00: return {"sll",    registers[rd], registers[rt], sa};
    case 0x01: break;
    case 0x02: return {"srl",    registers[rd], registers[rt], sa};
    case 0x03: return {"sra",    registers[rd], registers[rt], sa};
    case 0x04: return {"sllv",   registers[rd], registers[rt], rs};
    case 0x05: break;
    case 0x06: return {"srlv",   registers[rd], registers[rt], registers[rs]};
    case 0x07: return {"srav",   registers[rd], registers[rt], registers[rs]};
    case 0x08: return {"jr",     registers[rs]};
    case 0x09: return {"jalr",   registers[rd], registers[rs]};
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
    case 0x14: return {"dsllv",  registers[rd], registers[rt], registers[rs]};
    case 0x15: break;
    case 0x16: return {"dsrlv",  registers[rd], registers[rt], registers[rs]};
    case 0x17: return {"dsrav",  registers[rd], registers[rt], registers[rs]};
    case 0x18: return {"mult",   registers[rs], registers[rt]};
    case 0x19: return {"multu",  registers[rs], registers[rt]};
    case 0x1a: return {"div",    registers[rs], registers[rt]};
    case 0x1b: return {"divu",   registers[rs], registers[rt]};
    case 0x1c: return {"dmult",  registers[rs], registers[rt]};
    case 0x1d: return {"dmultu", registers[rs], registers[rt]};
    case 0x1e: return {"ddiv",   registers[rs], registers[rt]};
    case 0x1f: return {"ddivu",  registers[rs], registers[rt]};
    case 0x20: return {"add",    registers[rd], registers[rs], registers[rt]};
    case 0x21: return {"addu",   registers[rd], registers[rs], registers[rt]};
    case 0x22: return {"sub",    registers[rd], registers[rs], registers[rt]};
    case 0x23: return {"subu",   registers[rd], registers[rs], registers[rt]};
    case 0x24: return {"and",    registers[rd], registers[rs], registers[rt]};
    case 0x25: return {"or",     registers[rd], registers[rs], registers[rt]};
    case 0x26: return {"xor",    registers[rd], registers[rs], registers[rt]};
    case 0x27: return {"nor",    registers[rd], registers[rs], registers[rt]};
    case 0x28: break;
    case 0x29: break;
    case 0x2a: return {"slt",    registers[rd], registers[rs], registers[rt]};
    case 0x2b: return {"sltu",   registers[rd], registers[rs], registers[rt]};
    case 0x2c: return {"dadd",   registers[rd], registers[rs], registers[rt]};
    case 0x2d: return {"daddu",  registers[rd], registers[rs], registers[rt]};
    case 0x2e: return {"dsub",   registers[rd], registers[rs], registers[rt]};
    case 0x2f: return {"dsubu",  registers[rd], registers[rs], registers[rt]};
    case 0x30: return {"tge",    registers[rs], registers[rt]};
    case 0x31: return {"tgeu",   registers[rs], registers[rt]};
    case 0x32: return {"tlt",    registers[rs], registers[rt]};
    case 0x33: return {"tltu",   registers[rs], registers[rt]};
    case 0x34: return {"teq",    registers[rs], registers[rt]};
    case 0x35: break;
    case 0x36: return {"tne",    registers[rs], registers[rt]};
    case 0x37: break;
    case 0x38: return {"dsll",   registers[rd], registers[rt], registers[sa]};
    case 0x39: break;
    case 0x3a: return {"dsrl",   registers[rd], registers[rt], registers[sa]};
    case 0x3b: return {"dsra",   registers[rd], registers[rt], registers[sa]};
    case 0x3c: return {"dsll32", registers[rd], registers[rt], registers[sa]};
    case 0x3d: break;
    case 0x3e: return {"dsrl32", registers[rd], registers[rt], registers[sa]};
    case 0x3f: return {"dsra32", registers[rd], registers[rt], registers[sa]};
    }
    return {"invalid"};
  };

  regimm = [&]() -> vector<string> {
    switch(opcode >> 16 & 0x1f) {
    case 0x00: return {"bltz",    registers[rs], immediate16i()};
    case 0x01: return {"bgez",    registers[rs], immediate16i()};
    case 0x02: return {"bltzl",   registers[rs], immediate16i()};
    case 0x03: return {"bgezl",   registers[rs], immediate16i()};
    case 0x04: break;
    case 0x05: break;
    case 0x06: break;
    case 0x07: break;
    case 0x08: return {"tgei",    registers[rs], immediate16i()};
    case 0x09: return {"tgeiu",   registers[rs], immediate16u()};
    case 0x0a: return {"tlti",    registers[rs], immediate16i()};
    case 0x0b: return {"tltiu",   registers[rs], immediate16u()};
    case 0x0c: return {"teqi",    registers[rs], immediate16i()};
    case 0x0d: break;
    case 0x0e: return {"tnei",    registers[rs], immediate16u()};
    case 0x0f: break;
    case 0x10: return {"bltzal",  registers[rs], immediate16i()};
    case 0x11: return {"bgezal",  registers[rs], immediate16i()};
    case 0x12: return {"bltzall", registers[rs], immediate16i()};
    case 0x13: return {"bgezall", registers[rs], immediate16i()};
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
  };

  cop0 = [&]() -> vector<string> {
    switch(opcode >> 21 & 0x1f) {
    case 0x00: return {"mfc0",  registers[rt], registersCOP0[rd]};
    case 0x01: return {"dmfc0", registers[rt], registersCOP0[rd]};
    case 0x02: return {"cfc0",  registers[rt], registersCOP0[rd]};
    case 0x04: return {"mtc0",  registersCOP0[rd], registers[rt]};
    case 0x05: return {"dmtc0", registersCOP0[rd], registers[rt]};
    case 0x06: return {"ctc0",  registersCOP0[rd], registers[rt]};
    case 0x08: switch(opcode >> 16 & 3) {
      case 0x00: return {"bc0f",  branchAddress()};
      case 0x01: return {"bc0t",  branchAddress()};
      case 0x02: return {"bc0fl", branchAddress()};
      case 0x03: return {"bc0tl", branchAddress()};
      }
    }
    if(!(opcode >> 25 & 1)) return {"cop0"};
    switch(opcode & 0x3f) {
    case 0x01: return {"tlbr"};
    case 0x02: return {"tlbwi"};
    case 0x06: return {"tlbwr"};
    case 0x08: return {"tlbp"};
    }
    return {"cop0"};
  };

  cop1 = [&]() -> vector<string> {
    switch(opcode >> 21 & 0x1f) {
    case 0x00: return {"mfc1",  registers[rt], registersCOP1[rd]};
    case 0x01: return {"dmfc1", registers[rt], registersCOP1[rd]};
    case 0x02: return {"cfc1",  registers[rt], registersCOP1[rd]};
    case 0x04: return {"mtc1",  registersCOP1[rd], registers[rt]};
    case 0x05: return {"dmtc1", registersCOP1[rd], registers[rt]};
    case 0x06: return {"ctc1",  registersCOP1[rd], registers[rt]};
    case 0x08: switch(opcode >> 16 & 3) {
      case 0x00: return {"bc1f",  branchAddress()};
      case 0x01: return {"bc1t",  branchAddress()};
      case 0x02: return {"bc1fl", branchAddress()};
      case 0x03: return {"bc1tl", branchAddress()};
      }
    }
    bool s = (opcode & 1 << 21) == 0;
    bool i = (opcode & 1 << 23) != 0;
    //todo
    switch(opcode & 0x3f) {
    case 0x00: return {s ? "addS"    : "addD"   };
    case 0x01: return {s ? "subS"    : "subD"   };
    case 0x02: return {s ? "mulS"    : "mulD"   };
    case 0x03: return {s ? "divS"    : "divD"   };
    case 0x04: return {s ? "sqrtS"   : "sqrtD"  };
    case 0x05: return {s ? "absS"    : "absD"   };
    case 0x06: return {s ? "movS"    : "movD"   };
    case 0x07: return {s ? "negS"    : "negD"   };
    case 0x08: return {s ? "roundlS" : "roundlD"};
    case 0x09: return {s ? "trunclS" : "trunclD"};
    case 0x0a: return {s ? "ceillS"  : "ceillD" };
    case 0x0b: return {s ? "floorlS" : "floorlD"};
    case 0x0c: return {s ? "roundwS" : "roundwD"};
    case 0x0d: return {s ? "truncwS" : "truncwD"};
    case 0x0e: return {s ? "ceilwS"  : "ceilwD" };
    case 0x0f: return {s ? "floorwS" : "floorwD"};
    case 0x20: return {i ? (s ? "cvtsW" : "cvtsL") : "cvtsD"};
    case 0x21: return {i ? (s ? "cvtdW" : "cvtdL") : "cvtdS"};
    case 0x24: return {s ? "cvtwS"   : "cvtwD"  };
    case 0x25: return {s ? "cvtlS"   : "cvtlD"  };
    case 0x30: case 0x38: return {s ? "cfS"   : "cfD"  };
    case 0x31: case 0x39: return {s ? "cunS"  : "cunD" };
    case 0x32: case 0x3a: return {s ? "ceqS"  : "ceqD" };
    case 0x33: case 0x3b: return {s ? "cueqS" : "cueqD"};
    case 0x34: case 0x3c: return {s ? "coltS" : "coltD"};
    case 0x35: case 0x3d: return {s ? "cultS" : "cultD"};
    case 0x36: case 0x3e: return {s ? "coleS" : "coleD"};
    case 0x37: case 0x3f: return {s ? "culeS" : "culeD"};
    }
    return {"cop1"};
  };

  cop2 = [&]() -> vector<string> {
    return {"cop2"};
  };

  cop3 = [&]() -> vector<string> {
    return {"cop3"};
  };

  vector<string> v = disassemble();
  if(!v) v.append("invalid");
  if(!opcode) v = {"nop"};
  auto instruction = pad(v.takeFirst(), -8L);
  if(v.size() >= 2 && v[0] == v[1]) v.takeFirst();
  return {instruction, v.merge(",")};
}

auto CPU::disassembleContext() -> string {
  string s;
  for(uint n : range(32)) {
    s.append("r", n, ":", hex(GPR[n], 16L), " ");
  }
  s.append("hi:", hex(HI, 16L), " ");
  s.append("lo:", hex(LO, 16L));
  return s;
}
