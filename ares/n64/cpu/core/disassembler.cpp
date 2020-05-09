auto CPU::Disassembler::disassemble(u32 address, u32 instruction) -> string {
  this->address = address;
  this->instruction = instruction;

  auto v = EXECUTE();
  if(!v) v.append("invalid");
  if(!instruction) v = {"nop"};
  auto s = pad(v.takeFirst(), -8L);
  return {s, v.merge(",")};
}

auto CPU::Disassembler::EXECUTE() -> vector<string> {
  auto rd = [&] { return cpuRegister(instruction >> 11 & 31); };
  auto rt = [&] { return cpuRegister(instruction >> 16 & 31); };
  auto rs = [&] { return cpuRegister(instruction >> 21 & 31); };
  auto imm16i = [&] { return immediate(i16(instruction)); };
  auto imm32i = [&] { return immediate(i16(instruction) << 16); };
  auto imm32u = [&] { return immediate(u32(i16(instruction) << 16)); };
  auto jump = [&] { return immediate(address & 0xf000'0000 | (instruction & 0x03ff'ffff) << 2); };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };
  auto offsetBase = [&] { return string{immediate(u32(i16(instruction))), "(", rs(), ")"}; };

  auto ALU = [&](string_view name) -> vector<string> {
    string target = rt();
    string source = rs();
    string constant = immediate(abs(i16(instruction)));
    if(source == "0" || source == target) return {name, target, constant};
    return {name, target, source, constant};
  };

  auto ADDI = [&](string_view add, string_view sub) -> vector<string> {
    return ALU(i16(instruction) >= 0 ? add : sub);
  };

  auto BRANCH1 = [&](string_view name) -> vector<string> {
    return {name, rs(), branch()};
  };

  auto BRANCH2 = [&](string_view name) -> vector<string> {
    return {name, rs(), rt(), branch()};
  };

  auto JUMP = [&](string_view name) -> vector<string> {
    return {name, jump()};
  };

  auto LS = [&](string_view name) -> vector<string> {
    return {name, offsetBase()};
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
  case 0x2f: return {"cache", instruction >> 16 & 31, offsetBase()};
  case 0x30: return LS("ll");
  case 0x31: return LS("lwc1");
  case 0x32: return LS("lwc2");
  case 0x33: return LS("lwc3");
  case 0x34: return LS("lld");
  case 0x35: return LS("ldc1");
  case 0x36: return LS("ldc2");
  case 0x37: return LS("ld");
  case 0x38: return LS("sc");
  case 0x39: return LS("swc1");
  case 0x3a: return LS("swc2");
  case 0x3b: return LS("swc3");
  case 0x3c: return LS("scd");
  case 0x3d: return LS("sdc1");
  case 0x3e: return LS("sdc2");
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
    string target = rd();
    string source = rt();
    if(source == "0" || source == target) return {name, target, by};
    return {name, target, source, by};
  };

  auto REG = [&](string_view name) -> vector<string> {
    string target = rd();
    string source = rs();
    string extend = rt();
    if(source == "0" || source == target) return {name, target, extend};
    return {name, target, source, extend};
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
  case 0x09: return {"jalr", rd(), rs()};
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
  auto imm16u = [&] { return immediate(u16(instruction)); };

  auto I = [&](string_view name) -> vector<string> {
    return {name, rs(), imm16i()};
  };

  auto U = [&](string_view name) -> vector<string> {
    return {name, rs(), imm16u()};
  };

  switch(instruction >> 16 & 0x1f) {
  case 0x00: return I("bltz");
  case 0x01: return I("bgez");
  case 0x02: return I("bltzl");
  case 0x03: return I("bgezl");
  case 0x04: break;
  case 0x05: break;
  case 0x06: break;
  case 0x07: break;
  case 0x08: return I("tgei");
  case 0x09: return U("tgeiu");
  case 0x0a: return I("tlti");
  case 0x0b: return U("tltiu");
  case 0x0c: return I("teqi");
  case 0x0d: break;
  case 0x0e: return I("tnei");
  case 0x0f: break;
  case 0x10: return I("bltzal");
  case 0x11: return I("bgezal");
  case 0x12: return I("bltzall");
  case 0x13: return I("bgezall");
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
  auto sd = [&] { return sccRegister(instruction >> 11 & 31); };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };

  switch(instruction >> 21 & 0x1f) {
  case 0x00: return {"mfc0",  rt(), sd()};
  case 0x01: return {"dmfc0", rt(), sd()};
  case 0x02: return {"cfc0",  rt(), sd()};
  case 0x04: return {"mtc0",  sd(), rt()};
  case 0x05: return {"dmtc0", sd(), rt()};
  case 0x06: return {"ctc0",  sd(), rt()};
  case 0x08: switch(instruction >> 16 & 3) {
    case 0x00: return {"bc0f", branch()};
    case 0x01: return {"bc0t", branch()};
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
  auto fd = [&] { return fpuRegister(instruction >> 11 & 31); };
  auto branch = [&] { return immediate(address + 4 + (i16(instruction) << 2)); };

  switch(instruction >> 21 & 0x1f) {
  case 0x00: return {"mfc1",  rt(), fd()};
  case 0x01: return {"dmfc1", rt(), fd()};
  case 0x02: return {"cfc1",  rt(), fd()};
  case 0x04: return {"mtc1",  fd(), rt()};
  case 0x05: return {"dmtc1", fd(), rt()};
  case 0x06: return {"ctc1",  fd(), rt()};
  case 0x08: switch(instruction >> 16 & 3) {
    case 0x00: return {"bc0f", branch()};
    case 0x01: return {"bc0t", branch()};
    case 0x02: return {"bc0fl", branch()};
    case 0x03: return {"bc0tl", branch()};
    }
  }
  if(!(instruction >> 25 & 1)) return {};
  bool s = (instruction & 1 << 21) == 0;
  bool i = (instruction & 1 << 23) != 0;
  //todo: FPU arguments
  switch(instruction & 0x3f) {
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

  return {};
}

auto CPU::Disassembler::immediate(i64 value, uint bits) const -> string {
  if(value < 0) return {"-$", hex(value, bits >> 2)};
  return {"$", hex(value, bits >> 2)};
};

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
  if(showColors) return {"\e[37m", forward<P>(p)..., "\e[0m"};
  return {forward<P>(p)...};
}

//

auto CPU::disassembleInstruction() -> string {
  static const string registers[32] = {
     "0", "at", "v0", "v1", "a0", "a1", "a2", "a3",
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
    u32 pc = pipeline.address + 4 + (i16(opcode) << 2);
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

  //

  auto op_rt_rs_i16abs = [&](string op1, string op2) -> vector<string> {
    i16 imm = i16(opcode);
    string _op = imm >= 0 ? op1 : op2;
    string _rt = {registers[rt], "\e[37m{$", hex(GPR[rt].u64, 8L), "}\e[0m"};
    string _rs = {registers[rs], "\e[37m{$", hex(GPR[rs].u64, 8L), "}\e[0m"};
    string _imm = {"$", hex(abs(imm))};
    if(rs ==  0) return {_op, _rt, _imm};
    if(rt == rs) return {_op, _rt, _imm};
    return {_op, _rt, _rs, _imm};
  };

  //

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
    case 0x08: return op_rt_rs_i16abs("addi",  "subi");
    case 0x09: return op_rt_rs_i16abs("addiu", "subiu");
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
  return {instruction, v.merge(",")};
}

auto CPU::disassembleContext() -> string {
  //there are too many registers for this to be usable
  return {};
}
