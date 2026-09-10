// The register description is for 65816-aware RSP consumers.
// Stock GDB has no current 65816 target backend and the XML does not add a disassembler or an ABI.
namespace {
auto gdbRegisterBytes(u64 value, u32 size) -> string {
  string result;
  for(u32 byte : range(size)) result.append(hex(value >> (byte * 8) & 0xff, 2, '0'));
  return result;
}

auto normalizeGdbRegisters() -> void {
  if(cpu.r.e) {
    cpu.r.p.m = 1;
    cpu.r.p.x = 1;
    cpu.r.s.h = 1;
  }
  if(cpu.r.p.x) {
    cpu.r.x.h = 0;
    cpu.r.y.h = 0;
  }
}

auto installGdbHooks() -> void {
  using nall::GDB::server;
  server.hooks.instructionBoundaryStop = true;
  server.hooks.emuReset = [] { system.power(true); };
  server.hooks.registersLittleEndian = true;
  server.hooks.targetXML = []() -> string {
    return "<target version=\"1.0\">"
      "<feature name=\"dev.ares.wdc65816.core\">"
        "<reg name=\"a\" bitsize=\"16\" regnum=\"0\" group=\"general\"/>"
        "<reg name=\"x\" bitsize=\"16\" regnum=\"1\" group=\"general\"/>"
        "<reg name=\"y\" bitsize=\"16\" regnum=\"2\" group=\"general\"/>"
        "<reg name=\"s\" bitsize=\"16\" regnum=\"3\" group=\"general\"/>"
        "<reg name=\"d\" bitsize=\"16\" regnum=\"4\" group=\"general\"/>"
        "<reg name=\"db\" bitsize=\"8\" regnum=\"5\" group=\"general\"/>"
        "<reg name=\"pb\" bitsize=\"8\" regnum=\"6\" group=\"general\"/>"
        "<reg name=\"pc\" bitsize=\"16\" regnum=\"7\" group=\"general\"/>"
        "<reg name=\"p\" bitsize=\"8\" regnum=\"8\" group=\"general\"/>"
        "<reg name=\"e\" bitsize=\"8\" regnum=\"9\" group=\"general\"/>"
      "</feature>"
    "</target>";
  };
  server.hooks.regRead = [](u32 index) -> string {
    switch(index) {
      case 0: return gdbRegisterBytes(cpu.r.a.w, 2);
      case 1: return gdbRegisterBytes(cpu.r.x.w, 2);
      case 2: return gdbRegisterBytes(cpu.r.y.w, 2);
      case 3: return gdbRegisterBytes(cpu.r.s.w, 2);
      case 4: return gdbRegisterBytes(cpu.r.d.w, 2);
      case 5: return gdbRegisterBytes(cpu.r.b, 1);
      case 6: return gdbRegisterBytes(cpu.r.pc.b, 1);
      case 7: return gdbRegisterBytes(cpu.r.pc.w, 2);
      case 8: return gdbRegisterBytes(cpu.r.p, 1);
      case 9: return gdbRegisterBytes(cpu.r.e, 1);
    }
    return {};
  };
  server.hooks.regReadGeneral = []() -> string {
    string result;
    for(u32 index : range(10)) result.append(server.hooks.regRead(index));
    return result;
  };
  server.hooks.regWrite = [](u32 index, u64 value) -> bool {
    switch(index) {
      case 0: cpu.r.a.w = value; break;
      case 1: cpu.r.x.w = value; break;
      case 2: cpu.r.y.w = value; break;
      case 3: cpu.r.s.w = value; break;
      case 4: cpu.r.d.w = value; break;
      case 5: cpu.r.b = value; break;
      case 6: cpu.r.pc.b = value; break;
      case 7: cpu.r.pc.w = value; break;
      case 8: cpu.r.p = value; break;
      case 9: cpu.r.e = value & 1; break;
      default: return false;
    }
    normalizeGdbRegisters();
    return true;
  };
  server.hooks.regWriteGeneral = [](const string& data) -> void {
    // Decode the whole validated packet first: E/P determine the final widths.
    u64 values[10]{};
    u32 offset = 0;
    for(u32 index : range(10)) {
      u32 size = index < 5 || index == 7 ? 2 : 1;
      for(u32 byte : range(size)) values[index] |= data.slice(offset + byte * 2, 2).hex() << (byte * 8);
      offset += size * 2;
    }
    cpu.r.a.w = values[0]; cpu.r.x.w = values[1]; cpu.r.y.w = values[2];
    cpu.r.s.w = values[3]; cpu.r.d.w = values[4]; cpu.r.b = values[5];
    cpu.r.pc.b = values[6]; cpu.r.pc.w = values[7];
    cpu.r.p = values[8]; cpu.r.e = values[9] & 1;
    normalizeGdbRegisters();
  };
}
}
