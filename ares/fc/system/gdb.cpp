auto System::initGdbDebugHooks() -> void {
  // See: https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
  GDB::server.hooks.targetXML = []() -> string {
    return "<target version=\"1.0\">"
      "<architecture>mos</architecture>"
      "<feature>"
        "<flags id=\"6502_flags\" size=\"1\">"
          "<field name=\"CF\" start=\"0\" end=\"0\" dwarf_regnum=\"7\"/>"
          "<field name=\"ZF\" start=\"1\" end=\"1\" dwarf_regnum=\"10\"/>"
          "<field name=\"IF\" start=\"2\" end=\"2\"/>"
          "<field name=\"DF\" start=\"3\" end=\"3\"/>"
          "<field name=\"VF\" start=\"4\" end=\"4\" dwarf_regnum=\"9\"/>"
          "<field name=\"NF\" start=\"5\" end=\"5\" dwarf_regnum=\"8\"/>"
        "</flags>"
        "<reg name=\"A\" bitsize=\"8\" dwarf_regnum=\"0\"/>"
        "<reg name=\"X\" bitsize=\"8\" dwarf_regnum=\"2\"/>"
        "<reg name=\"Y\" bitsize=\"8\" dwarf_regnum=\"4\"/>"
        "<reg name=\"S\" bitsize=\"8\" dwarf_regnum=\"6\"/>"
        "<reg name=\"P\" bitsize=\"8\" dwarf_regnum=\"12\" type=\"6502_flags\"/>"
        "<reg name=\"PC\" bitsize=\"32\" type=\"code_ptr\" generic=\"pc\"/>"
      "</feature>"
    "</target>";
  };

  GDB::server.hooks.normalizeAddress = [](u64 address) -> u64 {
    return cpu.debugAddress(address & 0xFFFF);
  };

  GDB::server.hooks.read = [](u64 address, u32 byteCount) -> string {
    string res{};
    res.resize(byteCount * 2);
    char* resPtr = res.begin();

    for(u32 i : range(byteCount)) {
      auto val = cpu.readDebugger(address++);
      hexByte(resPtr, val);
      resPtr += 2;
    }

    return res;
  };

  GDB::server.hooks.write = [](u64 address, u32 unitSize, u64 value) {
    for(u32 i : range(unitSize)) {
      cpu.writeBus(address++, value);
      value >>= 8;
    }
  };

  GDB::server.hooks.regRead = [](u32 regIdx) {
    switch(regIdx) {
      case 0: return hex(cpu.A,   2, '0');
      case 1: return hex(cpu.X,   2, '0');
      case 2: return hex(cpu.Y,   2, '0');
      case 3: return hex(cpu.S,   2, '0');
      case 4: return hex(cpu.P|0, 2, '0');
      case 5: return hex(bswap32(cpu.debugAddress(cpu.PC)), 8, '0');
    }
    return string{"00"};
  };

  GDB::server.hooks.regWrite = [](u32 regIdx, u64 regValue) -> bool {
    switch(regIdx) {
      case 0: cpu.A  = regValue; return true;
      case 1: cpu.X  = regValue; return true;
      case 2: cpu.Y  = regValue; return true;
      case 3: cpu.S  = regValue; return true;
      case 4: cpu.P  = regValue | 0x30; return true;
      case 5: cpu.PC = bswap32(regValue) & 0xFFFF; return true;
    }
    return false;
  };

  GDB::server.hooks.regReadGeneral = []() {
    string res{};
    for(auto i : range(6)) {
      res.append(GDB::server.hooks.regRead(i));
    }
    return res;
  };

  GDB::server.hooks.regWriteGeneral = [](const string &regData) {
    u32 regIdx{0};
    GDB::server.hooks.regWrite(0, regData.slice(0,  2).hex());
    GDB::server.hooks.regWrite(1, regData.slice(2,  2).hex());
    GDB::server.hooks.regWrite(2, regData.slice(4,  2).hex());
    GDB::server.hooks.regWrite(3, regData.slice(6,  2).hex());
    GDB::server.hooks.regWrite(4, regData.slice(8,  2).hex());
    GDB::server.hooks.regWrite(5, regData.slice(10, 4).hex()); // 8
  };
}
