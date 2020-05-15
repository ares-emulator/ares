auto CPU::Disassembler::disassemble(u32 address, u32 instruction) -> string {
  this->address = address;
  this->instruction = instruction;

  auto v = EXECUTE();
  if(!v) v.append("invalid", string{"$", hex(instruction, 8L)});
  if(!instruction) v = {"nop"};
  auto s = pad(v.takeFirst(), -8L);
  return {s, v.merge(",")};
}

auto CPU::Disassembler::EXECUTE() -> vector<string> {
  auto rd = [&] { return cpuRegister(instruction >> 11 & 31); };
  auto rt = [&] { return cpuRegister(instruction >> 16 & 31); };
  auto ft = [&] { return fpuRegister(instruction >> 16 & 31); };
  auto rs = [&] { return cpuRegister(instruction >> 21 & 31); };
  auto imm16i = [&] { return immediate(i16(instruction)); };
  auto imm32i = [&] { return immediate(i16(instruction) << 16); };
  auto imm32u = [&] { return immediate(u32(i16(instruction) << 16)); };
  auto jump = [&] { return immediate(address & 0xf000'0000 | (instruction & 0x03ff'ffff) << 2); };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };
  auto offsetBase = [&] { return cpuRegister(instruction >> 21 & 31, i16(instruction)); };

  auto ALU = [&](string_view name) -> vector<string> {
    if(rt() == rs()) return {name, shorthand(rt()), immediate(u16(instruction))};
    return {name, rt(), rs(), immediate(u16(instruction))};
  };

  auto ADDI = [&](string_view add, string_view sub) -> vector<string> {
    auto name = i16(instruction) >= 0 ? add : sub;
    if(rt() == rs()) return {name, shorthand(rt()), immediate(abs(i16(instruction)))};
    return {name, rt(), rs(), immediate(abs(i16(instruction)))};
  };

  auto BRANCH1 = [&](string_view name) -> vector<string> {
    return {name, rs(), branch()};
  };

  auto BRANCH2 = [&](string_view name) -> vector<string> {
    return {name, rs(), rt(), branch()};
  };

  auto CACHE = [&](string_view name) -> vector<string> {
    auto cache  = instruction >> 16 & 3;
    auto op     = instruction >> 18 & 7;
    string type = "reserved";
    if(cache == 0) switch(op) {
    case 0: type = "code(IndexInvalidate)"; break;
    case 1: type = "code(IndexLoadTag)"; break;
    case 2: type = "code(IndexStoreTag)"; break;
    case 4: type = "code(HitInvalidate)"; break;
    case 5: type = "code(Fill)"; break;
    case 6: type = "code(HitWriteBack)"; break;
    }
    if(cache == 1) switch(op) {
    case 0: type = "data(IndexWriteBackInvalidate)"; break;
    case 1: type = "data(IndexLoadTag)"; break;
    case 2: type = "data(IndexStoreTag)"; break;
    case 3: type = "data(CreateDirtyExclusive)"; break;
    case 4: type = "data(HitInvalidate)"; break;
    case 5: type = "data(HitWriteBackInvalidate)"; break;
    case 6: type = "data(HitWriteBack)"; break;
    }
    return {name, type, offsetBase()};
  };

  auto JUMP = [&](string_view name) -> vector<string> {
    return {name, jump()};
  };

  auto LS = [&](string_view name) -> vector<string> {
    return {name, rt(), offsetBase()};
  };

  switch(instruction >> 26) {
  case 0x00: return SPECIAL();
  case 0x01: return REGIMM();
  case 0x02: return JUMP("j");
  case 0x03: return JUMP("jal");
  case 0x04: return BRANCH2("beq");
  case 0x05: return BRANCH2("bne");
  case 0x06: return BRANCH1("blez");
  case 0x07: return BRANCH1("bgtz");
  case 0x08: return ADDI("addi", "subi");
  case 0x09: return ADDI("addiu", "subiu");
  case 0x0a: return ALU("slti");
  case 0x0b: return ALU("sltiu");
  case 0x0c: return ALU("andi");
  case 0x0d: return ALU("ori");
  case 0x0e: return ALU("xori");
  case 0x0f: return {"lui", rt(), imm32u()};
  case 0x10: return SCC();
  case 0x11: return FPU();
  case 0x12: break;  //COP2
  case 0x13: break;  //COP3
  case 0x14: return BRANCH2("beql");
  case 0x15: return BRANCH2("bnel");
  case 0x16: return BRANCH1("blezl");
  case 0x17: return BRANCH1("bgtzl");
  case 0x18: return ADDI("daddi", "dsubi");
  case 0x19: return ADDI("daddiu", "dsubiu");
  case 0x1a: return LS("ldl");
  case 0x1b: return LS("ldr");
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
  case 0x20: return LS("lb");
  case 0x21: return LS("lh");
  case 0x22: return LS("lwl");
  case 0x23: return LS("lw");
  case 0x24: return LS("lbu");
  case 0x25: return LS("lhu");
  case 0x26: return LS("lwr");
  case 0x27: return LS("lwu");
  case 0x28: return LS("sb");
  case 0x29: return LS("sh");
  case 0x2a: return LS("swl");
  case 0x2b: return LS("sw");
  case 0x2c: return LS("sdl");
  case 0x2d: return LS("sdr");
  case 0x2e: return LS("swr");
  case 0x2f: return CACHE("cache");
  case 0x30: return LS("ll");
  case 0x31: return {"lwc1", ft(), offsetBase()};
  case 0x32: break;  //LWC2
  case 0x33: break;  //LWC3
  case 0x34: return LS("lld");
  case 0x35: return {"ldc1", ft(), offsetBase()};
  case 0x36: break;  //LDC2
  case 0x37: return LS("ld");
  case 0x38: return LS("sc");
  case 0x39: return {"swc1", ft(), offsetBase()};
  case 0x3a: break;  //SWC2
  case 0x3b: break;  //SWC3
  case 0x3c: return LS("scd");
  case 0x3d: return {"sdc1", ft(), offsetBase()};
  case 0x3e: break;  //SDC2
  case 0x3f: return LS("sd");
  }
  return {};
}

auto CPU::Disassembler::SPECIAL() -> vector<string> {
  auto sa = [&] { return string{instruction >> 6 & 31}; };
  auto rd = [&] { return cpuRegister(instruction >> 11 & 31); };
  auto rt = [&] { return cpuRegister(instruction >> 16 & 31); };
  auto rs = [&] { return cpuRegister(instruction >> 21 & 31); };

  auto ALU = [&](string_view name, string_view by) -> vector<string> {
    if(rd() == rt()) return {name, shorthand(rd()), by};
    return {name, rd(), rt(), by};
  };

  auto JALR = [&](string_view name) -> vector<string> {
    if((instruction >> 11 & 31) == 31) return {name, rs()};
    return {name, rd(), rs()};
  };

  auto REG = [&](string_view name) -> vector<string> {
    if(rd() == rs()) return {name, shorthand(rd()), rt()};
    return {name, rd(), rs(), rt()};
  };

  auto ST = [&](string_view name) -> vector<string> {
    return {name, rs(), rt()};
  };

  switch(instruction & 0x3f) {
  case 0x00: return ALU("sll", sa());
  case 0x01: break;
  case 0x02: return ALU("srl", sa());
  case 0x03: return ALU("sra", sa());
  case 0x04: return ALU("sllv", rs());
  case 0x05: break;
  case 0x06: return ALU("srlv", rs());
  case 0x07: return ALU("srav", rs());
  case 0x08: return {"jr", rs()};
  case 0x09: return JALR("jalr");
  case 0x0a: break;
  case 0x0b: break;
  case 0x0c: return {"syscall"};
  case 0x0d: return {"break"};
  case 0x0e: break;
  case 0x0f: return {"sync"};
  case 0x10: return {"mfhi", rd()};
  case 0x11: return {"mthi", rs()};
  case 0x12: return {"mflo", rd()};
  case 0x13: return {"mtlo", rs()};
  case 0x14: return ALU("dsllv", rs());
  case 0x15: break;
  case 0x16: return ALU("dsrlv", rs());
  case 0x17: return ALU("dsrav", rs());
  case 0x18: return ST("mult");
  case 0x19: return ST("multu");
  case 0x1a: return ST("div");
  case 0x1b: return ST("divu");
  case 0x1c: return ST("dmult");
  case 0x1d: return ST("dmultu");
  case 0x1e: return ST("ddiv");
  case 0x1f: return ST("ddivu");
  case 0x20: return REG("add");
  case 0x21: return REG("addu");
  case 0x22: return REG("sub");
  case 0x23: return REG("subu");
  case 0x24: return REG("and");
  case 0x25: return REG("or");
  case 0x26: return REG("xor");
  case 0x27: return REG("nor");
  case 0x28: break;
  case 0x29: break;
  case 0x2a: return REG("slt");
  case 0x2b: return REG("sltu");
  case 0x2c: return REG("dadd");
  case 0x2d: return REG("daddu");
  case 0x2e: return REG("dsub");
  case 0x2f: return REG("dsubu");
  case 0x30: return ST("tge");
  case 0x31: return ST("tgeu");
  case 0x32: return ST("tlt");
  case 0x33: return ST("tltu");
  case 0x34: return ST("teq");
  case 0x35: break;
  case 0x36: return ST("tne");
  case 0x37: break;
  case 0x38: return ALU("dsll", sa());
  case 0x39: break;
  case 0x3a: return ALU("dsrl", sa());
  case 0x3b: return ALU("dsra", sa());
  case 0x3c: return ALU("dsll32", sa());
  case 0x3d: break;
  case 0x3e: return ALU("dsrl32", sa());
  case 0x3f: return ALU("dsra32", sa());
  }

  return {};
}

auto CPU::Disassembler::REGIMM() -> vector<string> {
  auto rs = [&] { return cpuRegister(instruction >> 21 & 31); };
  auto imm16i = [&] { return immediate(i16(instruction)); };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };

  auto BRANCH = [&](string_view name) -> vector<string> {
    return {name, rs(), branch()};
  };

  auto TRAP = [&](string_view name) -> vector<string> {
    return {name, rs(), imm16i()};
  };

  switch(instruction >> 16 & 0x1f) {
  case 0x00: return BRANCH("bltz");
  case 0x01: return BRANCH("bgez");
  case 0x02: return BRANCH("bltzl");
  case 0x03: return BRANCH("bgezl");
  case 0x04: break;
  case 0x05: break;
  case 0x06: break;
  case 0x07: break;
  case 0x08: return TRAP("tgei");
  case 0x09: return TRAP("tgeiu");
  case 0x0a: return TRAP("tlti");
  case 0x0b: return TRAP("tltiu");
  case 0x0c: return TRAP("teqi");
  case 0x0d: break;
  case 0x0e: return TRAP("tnei");
  case 0x0f: break;
  case 0x10: return BRANCH("bltzal");
  case 0x11: return BRANCH("bgezal");
  case 0x12: return BRANCH("bltzall");
  case 0x13: return BRANCH("bgezall");
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

  return {};
}

auto CPU::Disassembler::SCC() -> vector<string> {
  auto rt = [&] { return cpuRegister(instruction >> 16 & 31); };
  auto rd = [&] { return sccRegister(instruction >> 11 & 31); };
  auto cd = [&] { return string{"ccr", instruction >> 11 & 31}; };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };

  switch(instruction >> 21 & 0x1f) {
  case 0x00: return {"mfc0",  rt(), rd()};
  case 0x01: return {"dmfc0", rt(), rd()};
  case 0x02: return {"cfc0",  rt(), cd()};
  case 0x04: return {"mtc0",  rt(), rd()};
  case 0x05: return {"dmtc0", rt(), rd()};
  case 0x06: return {"ctc0",  rt(), cd()};
  case 0x08: switch(instruction >> 16 & 3) {
    case 0x00: return {"bc0f",  branch()};
    case 0x01: return {"bc0t",  branch()};
    case 0x02: return {"bc0fl", branch()};
    case 0x03: return {"bc0tl", branch()};
    }
  }
  if(!(instruction >> 25 & 1)) return {};
  switch(instruction & 0x3f) {
  case 0x01: return {"tlbr"};
  case 0x02: return {"tlbwi"};
  case 0x06: return {"tlbwr"};
  case 0x08: return {"tlbp"};
  case 0x18: return {"eret"};
  }

  return {};
}

auto CPU::Disassembler::FPU() -> vector<string> {
  auto rt = [&] { return cpuRegister(instruction >> 16 & 31); };
  auto rd = [&] { return fpuRegister(instruction >> 11 & 31); };
  auto cd = [&] { return string{"ccr", instruction >> 11 & 31}; };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };

  switch(instruction >> 21 & 0x1f) {
  case 0x00: return {"mfc1",  rt(), rd()};
  case 0x01: return {"dmfc1", rt(), rd()};
  case 0x02: return {"cfc1",  rt(), cd()};
  case 0x04: return {"mtc1",  rt(), rd()};
  case 0x05: return {"dmtc1", rt(), rd()};
  case 0x06: return {"ctc1",  rt(), cd()};
  case 0x08: switch(instruction >> 16 & 3) {
    case 0x00: return {"bc1f",  branch()};
    case 0x01: return {"bc1t",  branch()};
    case 0x02: return {"bc1fl", branch()};
    case 0x03: return {"bc1tl", branch()};
    }
  }
  if(!(instruction >> 25 & 1)) return {};

  auto fd = [&] { return fpuRegister(instruction >>  6 & 31); };
  auto fs = [&] { return fpuRegister(instruction >> 11 & 31); };
  auto ft = [&] { return fpuRegister(instruction >> 16 & 31); };

  auto DS = [&](string_view name) -> vector<string> {
    if(fd() == fs()) return {name, shorthand(fd())};
    return {name, fd(), fs()};
  };

  auto DST = [&](string_view name) -> vector<string> {
    if(fd() == fs()) return {name, shorthand(fd()), ft()};
    return {name, fd(), fs(), ft()};
  };

  auto ST = [&](string_view name) -> vector<string> {
    return {name, fs(), ft()};
  };

  bool s = (instruction & 1 << 21) == 0;
  bool i = (instruction & 1 << 23) != 0;

  switch(instruction & 0x3f) {
  case 0x00: return DST(s ? "adds"    : "addd"   );
  case 0x01: return DST(s ? "subs"    : "subd"   );
  case 0x02: return DST(s ? "muls"    : "muld"   );
  case 0x03: return DST(s ? "divs"    : "divd"   );
  case 0x04: return DS (s ? "sqrts"   : "sqrtd"  );
  case 0x05: return DS (s ? "abss"    : "absd"   );
  case 0x06: return DS (s ? "movs"    : "movd"   );
  case 0x07: return DS (s ? "negs"    : "negd"   );
  case 0x08: return DS (s ? "roundls" : "roundld");
  case 0x09: return DS (s ? "truncls" : "truncld");
  case 0x0a: return DS (s ? "ceills"  : "ceilld" );
  case 0x0b: return DS (s ? "floorls" : "floorld");
  case 0x0c: return DS (s ? "roundws" : "roundwd");
  case 0x0d: return DS (s ? "truncws" : "truncwd");
  case 0x0e: return DS (s ? "ceilws"  : "ceilwd" );
  case 0x0f: return DS (s ? "floorws" : "floorwd");
  case 0x20: return DS (i ? (s ? "cvtsw" : "cvtsl") : "cvtsd");
  case 0x21: return DS (i ? (s ? "cvtdw" : "cvtdl") : "cvtds");
  case 0x24: return DS (s ? "cvtws" : "cvtwd");
  case 0x25: return DS (s ? "cvtls" : "cvtld");
  case 0x30: case 0x38: return ST(s ? "cfs"   : "cfd"  );
  case 0x31: case 0x39: return ST(s ? "cuns"  : "cund" );
  case 0x32: case 0x3a: return ST(s ? "ceqs"  : "ceqd" );
  case 0x33: case 0x3b: return ST(s ? "cueqs" : "cueqd");
  case 0x34: case 0x3c: return ST(s ? "colts" : "coltd");
  case 0x35: case 0x3d: return ST(s ? "cults" : "cultd");
  case 0x36: case 0x3e: return ST(s ? "coles" : "coled");
  case 0x37: case 0x3f: return ST(s ? "cules" : "culed");
  }

  return {};
}

auto CPU::Disassembler::immediate(i64 value, uint bits) const -> string {
  if(value < 0) return {"-$", hex(-value, bits >> 2)};
  return {"$", hex(value, bits >> 2)};
};

auto CPU::Disassembler::cpuRegister(uint index, i16 offset) const -> string {
  static const string registers[32] = {
     "0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
  };
  string adjust;
  if(offset >= 0) adjust = {"+$", hex( offset)};
  if(offset <  0) adjust = {"-$", hex(-offset)};
  if(index && showValues) return {registers[index], adjust, hint("{$", hex(self.core.r[index].u64 + offset, 8L), "}")};
  return {registers[index], adjust};
}

auto CPU::Disassembler::cpuRegister(uint index) const -> string {
  static const string registers[32] = {
     "0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra",
  };
  if(index && showValues) return {registers[index], hint("{$", hex(self.core.r[index].u64, 8L), "}")};
  return registers[index];
}

auto CPU::Disassembler::sccRegister(uint index) const -> string {
  static const string registers[32] = {
    "Index",    "Random",   "EntryLo0",    "EntryLo1",
    "Context",  "PageMask", "Wired",       "Unused7",
    "BadVAddr", "Count",    "EntryHi",     "Compare",
    "Status",   "Cause",    "EPC",         "PrID",
    "Config",   "LLAddr",   "WatchLo",     "WatchHi",
    "XContext", "Unused21", "Unused22",    "Unused23",
    "Unused24", "Unused25", "ParityError", "CacheError",
    "TagLo",    "TagHi",    "ErrorEPC",    "Unused31",
  };
  if(showValues) return {registers[index], hint("{$", hex(self.getControlRegister(index)), "}")};
  return registers[index];
}

auto CPU::Disassembler::fpuRegister(uint index) const -> string {
  static const string registers[32] = {
     "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
     "f8",  "f9", "f10", "f11", "f12", "f13", "f14", "f15",
    "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
    "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
  };
  bool f32 = (instruction & 1 << 21) == 0;
  bool f64 = (instruction & 1 << 21) != 0;
  if(f32 && showValues) return {registers[index], hint("{", self.fpu.r[index].f32, "}")};
  if(f64 && showValues) return {registers[index], hint("{", self.fpu.r[index].f64, "}")};
  return registers[index];
}

template<typename... P>
auto CPU::Disassembler::hint(P&&... p) const -> string {
  if(showColors) return {"\e[0m\e[37m", forward<P>(p)..., "\e[0m"};
  return {forward<P>(p)...};
}

template<typename... P>
auto CPU::Disassembler::shorthand(P&&... p) const -> string {
  if(showColors) return {"\e[0m\e[1;92m", forward<P>(p)..., "\e[0m"};
  return {forward<P>(p)...};
}
