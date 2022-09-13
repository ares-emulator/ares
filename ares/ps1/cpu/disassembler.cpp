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
  auto rtName  = [&] { return ipuRegisterName (instruction >> 16 & 31); };
  auto rtValue = [&] { return ipuRegisterValue(instruction >> 16 & 31); };
  auto rsValue = [&] { return ipuRegisterValue(instruction >> 21 & 31); };
  auto dtName  = [&] { return gteDataRegisterName (instruction >> 16 & 31); };
  auto dtValue = [&] { return gteDataRegisterValue(instruction >> 16 & 31); };
  auto jump    = [&] { return immediate(address + 4 & 0xf000'0000 | (instruction & 0x03ff'ffff) << 2, 32L); };
  auto branch  = [&] { return immediate(address + 4 + (s16(instruction) << 2), 32L); };
  auto offset  = [&] { return ipuRegisterIndex(instruction >> 21 & 31, s16(instruction)); };

  auto ALU = [&](string_view name) -> vector<string> {
    return {name, rtName(), rsValue(), immediate(u16(instruction))};
  };

  auto ADDI = [&](string_view add, string_view sub, string_view mov) -> vector<string> {
    if(!(instruction >> 21 & 31)) return {mov, rtName(), immediate(s16(instruction))};
    return {s16(instruction) >= 0 ? add : sub, rtName(), rsValue(), immediate(abs(s16(instruction)))};
  };

  auto BRANCH1 = [&](string_view name) -> vector<string> {
    return {name, rsValue(), branch()};
  };

  auto BRANCH2 = [&](string_view name) -> vector<string> {
    auto rs = rsValue(), rt = rtValue(), target = branch();
    if(name == "beq" && rs == "0" && rt == "0") return {"bra", target};
    return {name, rs, rt, target};
  };

  auto JUMP = [&](string_view name) -> vector<string> {
    return {name, jump()};
  };

  auto LOAD = [&](string_view name) -> vector<string> {
    return {name, rtName(), offset()};
  };

  auto STORE = [&](string_view name) -> vector<string> {
    return {name, rtValue(), offset()};
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
  case 0x08: return ADDI("addi",  "subi",  "li");
  case 0x09: return ADDI("addiu", "subiu", "liu");
  case 0x0a: return ALU("slti");
  case 0x0b: return ALU("sltiu");
  case 0x0c: return ALU("andi");
  case 0x0d: return ALU("ori");
  case 0x0e: return ALU("xori");
  case 0x0f: return {"lui", rtName(), immediate(u16(instruction), 16L)};
  case 0x10: return SCC();
  case 0x11: break;  //COP1
  case 0x12: return GTE();
  case 0x13: break;  //COP3
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
  case 0x20: return LOAD("lb");
  case 0x21: return LOAD("lh");
  case 0x22: return LOAD("lwl");
  case 0x23: return LOAD("lw");
  case 0x24: return LOAD("lbu");
  case 0x25: return LOAD("lhu");
  case 0x26: return LOAD("lwr");
  case 0x27: break;
  case 0x28: return STORE("sb");
  case 0x29: return STORE("sh");
  case 0x2a: return STORE("swl");
  case 0x2b: return STORE("sw");
  case 0x2c: break;
  case 0x2d: break;
  case 0x2e: return STORE("swr");
  case 0x2f: break;
  case 0x30: return {"lwc0", instruction >> 16 & 31, offset()};
  case 0x31: return {"lwc1", instruction >> 16 & 31, offset()};
  case 0x32: return {"lwc2", dtName(), offset()};
  case 0x33: return {"lwc3", instruction >> 16 & 31, offset()};
  case 0x38: return {"swc0", instruction >> 16 & 31, offset()};
  case 0x39: return {"swc1", instruction >> 16 & 31, offset()};
  case 0x3a: return {"swc2", dtValue(), offset()};
  case 0x3b: return {"swc3", instruction >> 16 & 31, offset()};
  }

  return {};
}

auto CPU::Disassembler::SPECIAL() -> vector<string> {
  auto shift   = [&] { return string{instruction >> 6 & 31}; };
  auto rdName  = [&] { return ipuRegisterName (instruction >> 11 & 31); };
  auto rdValue = [&] { return ipuRegisterValue(instruction >> 11 & 31); };
  auto rtValue = [&] { return ipuRegisterValue(instruction >> 16 & 31); };
  auto rsValue = [&] { return ipuRegisterValue(instruction >> 21 & 31); };

  auto ALU = [&](string_view name, string_view by) -> vector<string> {
    return {name, rdName(), rtValue(), by};
  };

  auto JALR = [&](string_view name) -> vector<string> {
    if((instruction >> 11 & 31) == 31) return {name, rsValue()};
    return {name, rdName(), rsValue()};
  };

  auto REG = [&](string_view name) -> vector<string> {
    return {name, rdName(), rsValue(), rtValue()};
  };

  auto ST = [&](string_view name) -> vector<string> {
    return {name, rsValue(), rtValue()};
  };

  switch(instruction & 0x3f) {
  case 0x00: return ALU("sll", shift());
  case 0x01: break;
  case 0x02: return ALU("srl", shift());
  case 0x03: return ALU("sra", shift());
  case 0x04: return ALU("sllv", rsValue());
  case 0x05: break;
  case 0x06: return ALU("srlv", rsValue());
  case 0x07: return ALU("srav", rsValue());
  case 0x08: return {"jr", rsValue()};
  case 0x09: return JALR("jalr");
  case 0x0a: break;
  case 0x0b: break;
  case 0x0c: return {"syscall"};
  case 0x0d: return {"break"};
  case 0x0e: break;
  case 0x0f: break;
  case 0x10: return {"mfhi", rdName(), {"hi", hint("{$", hex(self.ipu.hi, 8L), "}")}};
  case 0x11: return {"mthi", rsValue(), "hi"};
  case 0x12: return {"mflo", rdName(), {"lo", hint("{$", hex(self.ipu.lo, 8L), "}")}};
  case 0x13: return {"mtlo", rsValue(), "lo"};
  case 0x14: break;
  case 0x15: break;
  case 0x16: break;
  case 0x17: break;
  case 0x18: return ST("mult");
  case 0x19: return ST("multu");
  case 0x1a: return ST("div");
  case 0x1b: return ST("divu");
  case 0x1c: break;
  case 0x1d: break;
  case 0x1e: break;
  case 0x1f: break;
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
  case 0x2c: break;
  case 0x2d: break;
  case 0x2e: break;
  case 0x2f: break;
  case 0x30: break;
  case 0x31: break;
  case 0x32: break;
  case 0x33: break;
  case 0x34: break;
  case 0x35: break;
  case 0x36: break;
  case 0x37: break;
  case 0x38: break;
  case 0x39: break;
  case 0x3a: break;
  case 0x3b: break;
  case 0x3c: break;
  case 0x3d: break;
  case 0x3e: break;
  case 0x3f: break;
  }

  return {};
}

auto CPU::Disassembler::REGIMM() -> vector<string> {
  auto rsValue = [&] { return ipuRegisterValue(instruction >> 21 & 31); };
  auto branch  = [&] { return immediate(address + 4 + (s16(instruction) << 2)); };

  auto BRANCH = [&](string_view name) -> vector<string> {
    return {name, rsValue(), branch()};
  };

  switch(instruction >> 16 & 0x1f) {
  case 0x00: return BRANCH("bltz");
  case 0x01: return BRANCH("bgez");
  case 0x02: break;
  case 0x03: break;
  case 0x04: break;
  case 0x05: break;
  case 0x06: break;
  case 0x07: break;
  case 0x08: break;
  case 0x09: break;
  case 0x0a: break;
  case 0x0b: break;
  case 0x0c: break;
  case 0x0d: break;
  case 0x0e: break;
  case 0x0f: break;
  case 0x10: return BRANCH("bltzal");
  case 0x11: return BRANCH("bgezal");
  case 0x12: break;
  case 0x13: break;
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
  auto rtName  = [&] { return ipuRegisterName (instruction >> 16 & 31); };
  auto rtValue = [&] { return ipuRegisterValue(instruction >> 16 & 31); };
  auto sdName  = [&] { return sccRegisterName (instruction >> 11 & 31); };
  auto sdValue = [&] { return sccRegisterValue(instruction >> 11 & 31); };

  switch(instruction >> 21 & 0x1f) {
  case 0x00: return {"mfc0", rtName(), sdValue()};
  case 0x04: return {"mtc0", rtValue(), sdName()};
  }
  if(!(instruction >> 25 & 1)) return {};
  switch(instruction & 0x3f) {
  case 0x10: return {"rfe"};
  }

  return {};
}

auto CPU::Disassembler::GTE() -> vector<string> {
  auto rtName  = [&] { return ipuRegisterName (instruction >> 16 & 31); };
  auto rtValue = [&] { return ipuRegisterValue(instruction >> 16 & 31); };
  auto drName  = [&] { return gteDataRegisterName (instruction >> 11 & 31); };
  auto drValue = [&] { return gteDataRegisterValue(instruction >> 11 & 31); };
  auto crName  = [&] { return gteControlRegisterName (instruction >> 11 & 31); };
  auto crValue = [&] { return gteControlRegisterValue(instruction >> 11 & 31); };

  switch(instruction >> 21 & 0x1f) {
  case 0x00: return {"mfc2", rtName(), drValue()};
  case 0x02: return {"cfc2", rtName(), crValue()};
  case 0x04: return {"mtc2", rtValue(), drName()};
  case 0x06: return {"ctc2", rtValue(), crName()};
  }
  if(!(instruction >> 25 & 1)) return {};
  u32 lm = instruction >> 10 & 1;
  u32 tv = instruction >> 13 & 3;
  u32 mv = instruction >> 15 & 3;
  u32 mm = instruction >> 17 & 3;
  u32 sf = instruction >> 19 & 1;
  switch(instruction & 0x3f) {
  case 0x00: return {"rtps", lm, sf};  //0x01 mirror?
  case 0x01: return {"rtps", lm, sf};
  case 0x06: return {"nclip"};
  case 0x0c: return {"op", lm, sf};
  case 0x10: return {"dpcs", lm, sf};
  case 0x11: return {"intpl", lm, sf};
  case 0x12: return {"mvmva", lm, tv, mv, mm, sf};
  case 0x13: return {"ncds", lm, sf};
  case 0x14: return {"cdp", lm, sf};
  case 0x16: return {"ncdt", lm, sf};
  case 0x1a: return {"dcpl", lm, sf};  //0x29 mirror?
  case 0x1b: return {"nccs", lm, sf};
  case 0x1c: return {"cc", lm, sf};
  case 0x1e: return {"ncs", lm, sf};
  case 0x20: return {"nct", lm, sf};
  case 0x28: return {"sqr", lm, sf};
  case 0x29: return {"dpcl", lm, sf};
  case 0x2a: return {"dpct", lm, sf};
  case 0x2d: return {"avsz3"};
  case 0x2e: return {"avsz4"};
  case 0x30: return {"rtpt", lm, sf};
  case 0x3d: return {"gpf", lm, sf};
  case 0x3e: return {"gpl", lm, sf};
  case 0x3f: return {"ncct", lm, sf};
  }

  return {};
}

auto CPU::Disassembler::immediate(s64 value, u8 bits) const -> string {
  if(value < 0) return {"-$", hex(-value, bits >> 2)};
  return {"$", hex(value, bits >> 2)};
}

auto CPU::Disassembler::ipuRegisterName(u8 index) const -> string {
  static const string registers[32] = {
     "0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
  };
  return registers[index];
}

auto CPU::Disassembler::ipuRegisterValue(u8 index) const -> string {
  if(index && showValues) return {ipuRegisterName(index), hint("{$", hex(self.ipu.r[index], 8L), "}")};
  return ipuRegisterName(index);
}

auto CPU::Disassembler::ipuRegisterIndex(u8 index, s16 offset) const -> string {
  string adjust;
  if(offset >= 0) adjust = {"+$", hex( offset)};
  if(offset <  0) adjust = {"-$", hex(-offset)};
  if(index && showValues) return {ipuRegisterName(index), adjust, hint("{$", hex(self.ipu.r[index] + offset, 8L), "}")};
  return {ipuRegisterName(index), adjust};
}

auto CPU::Disassembler::sccRegisterName(u8 index) const -> string {
  static const string registers[32] = {
    "scc0", "scc1", "scc2",  "bpc",  "scc4", "bda",   "tar", "dcic",
    "bada", "bdam", "scc10", "bpcm", "sr",   "cause", "epc", "prid",
    "scc16", "scc17", "scc18", "scc19", "scc20", "scc21", "scc22", "scc23",
    "scc24", "scc25", "scc26", "scc27", "scc28", "scc29", "scc30", "scc31",
  };
  return registers[index];
}

auto CPU::Disassembler::sccRegisterValue(u8 index) const -> string {
  if(showValues) return {sccRegisterName(index), hint("{$", hex(self.getControlRegisterSCC(index), 8L), "}")};
  return sccRegisterName(index);
}

auto CPU::Disassembler::gteDataRegisterName(u8 index) const -> string {
  static const string registers[32] = {
    "vxy0", "vz0", "vxy1", "vz2", "vxy2", "vz2", "rgbc", "otz",
    "ir0", "ir1", "ir2", "ir3", "sxy0", "sxy1", "sxy2", "sxyp",
    "sz0", "sz1", "sz2", "sz3", "rgb0", "rgb1", "rgb2", "res1",
    "mac0", "mac1", "mac2", "mac3", "irgb", "orgb", "lzcs", "lzcr",
  };
  return registers[index];
}

auto CPU::Disassembler::gteDataRegisterValue(u8 index) const -> string {
  if(showValues) return {gteDataRegisterName(index)};  //todo
  return gteDataRegisterName(index);
}

auto CPU::Disassembler::gteControlRegisterName(u8 index) const -> string {
  static const string registers[32] = {
    "rt11+rt12", "rt13+rt21", "rt22+rt23", "rt31+rt32", "rt33", "trx", "try", "trz",
    "l11+l12", "l13+l21", "l22+l23", "l31+l32", "l33", "rbk", "gbk", "bbk",
    "lr1+lr2", "lr3+lg1", "lg2+lg3", "lb1+lb2", "lb3", "rfc", "gfc", "bfc",
    "ofx", "ofy", "h", "dqa", "dqb", "zsf3", "zsf4", "flag",
  };
  return registers[index];
}

auto CPU::Disassembler::gteControlRegisterValue(u8 index) const -> string {
  if(showValues) return {gteControlRegisterName(index)};  //todo
  return gteControlRegisterName(index);
}

template<typename... P>
auto CPU::Disassembler::hint(P&&... p) const -> string {
  if(showColors) return {"\e[0m\e[37m", std::forward<P>(p)..., "\e[0m"};
  return {std::forward<P>(p)...};
}
