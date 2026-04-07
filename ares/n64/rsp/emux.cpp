namespace {

auto readRspByte(u32 address) -> u8 {
  u32 a = address & 0x1fff;
  if(a & 0x1000) return rsp.imem.read<Byte>(a & 0xfff);
  return rsp.dmem.read<Byte>(a & 0xfff);
}

auto toSignedOrHex(u32 value, bool decimal) -> string {
  if(decimal) return string{(s32)value};
  return hex(value, 8L);
}

}

auto RSP::XDETECT(r32& rd, u32 code) -> void {
  if(!system.homebrewMode) return;
  n64 detect = 0;
  n64 ioctl = 0;
  detect.bit(0x20) = 1;  // XDETECT
  detect.bit(0x23) = 1;  // XTRACE-START
  detect.bit(0x24) = 1;  // XTRACE-STOP
  detect.bit(0x25) = 1;  // XLOG
  detect.bit(0x26) = 1;  // XLOGREGS
  detect.bit(0x27) = 1;  // XHEXDUMP
  detect.bit(0x28) = 1;  // XPROF
  detect.bit(0x29) = 1;  // XPROFREAD
  detect.bit(0x2a) = 1;  // XEXCEPTION
  detect.bit(0x2c) = 1;  // XIOCTL
  ioctl.bit(0x01) = 1;   // XIOCTL exit
  ioctl.bit(0x02) = 1;   // XIOCTL fast
  ioctl.bit(0x03) = 1;   // XIOCTL slow
  switch(code) {
  case 0x00: rd.u32 = detect.bit(0x00, 0x1f); break;
  case 0x01: rd.u32 = detect.bit(0x20, 0x3f); break;
  case 0x02: rd.u32 = ioctl.bit(0x00, 0x1f); break;
  default:   rd.u32 = 0; break;
  }
}

auto RSP::XTRACESTART(u32 code) -> void {
  if(!system.homebrewMode) return;
  debugger.tracer.instruction->setEnabled(true);
  debugger.tracer.instructionCountdown = code;
  debugger.tracer.traceStartCycle = pipeline.clocksTotal / 3;
}

auto RSP::XTRACESTOP() -> void {
  if(!system.homebrewMode) return;
  debugger.tracer.instruction->setEnabled(false);
}

auto RSP::XLOG(cr32& rd, cr32& rt, u32 code) -> void {
  if(!system.homebrewMode) return;
  auto& emux = debugger.tracer.emux;
  string out;
  u32 address = rd.u32;
  switch(code) {
  case 0x00:
    for(u32 n : range(1_MiB)) {
      u8 c = readRspByte(address++);
      if(!c) break;
      out.append((char)c);
    }
    break;
  case 0x01:
    for(u32 n : range(rt.u32)) out.append((char)readRspByte(address++));
    break;
  }
  if(out) emux->notify(out);
}

auto RSP::XLOGREGS(cr32& rd, u32 code) -> void {
  if(!system.homebrewMode) return;
  static const char* gprNames[32] = {
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
  };
  static const char* cop0Names[16] = {
    "dma_spaddr", "dma_ramaddr", "dma_read", "dma_write",
    "sp_status", "dma_full", "dma_busy", "semaphore",
    "dp_start", "dp_end", "dp_current", "dp_status",
    "dp_clock", "dp_busy", "dp_pipe_busy", "dp_tmem_busy"
  };
  auto& emux = debugger.tracer.emux;
  u32 mask = rd.u32;
  u32 mode = code & 3;
  bool decimal = code & (1 << 2);
  bool extra = code & (1 << 4);
  string out;

  if(mode == 3) {
    u32 columns = 0;
    for(u32 i : range(32)) {
      if(mask && !(mask & (1u << i))) continue;
      out.append(gprNames[i], ": ", toSignedOrHex(ipu.r[i].u32, decimal));
      columns++;
      if(columns == 4) { out.append("\n"); columns = 0; }
      else out.append("   ");
    }
    if(extra) out.append("pc: ", toSignedOrHex(ipu.pc, decimal), "\n");
    else if(columns) out.append("\n");
  }

  if(mode == 0) {
    u32 columns = 0;
    for(u32 i : range(16)) {
      if(mask && !(mask & (1u << i))) continue;
      u32 value = i == 7 ? (u32)status.semaphore : (i & 8 ? rdp.readWord(i & 7, *this) : ioRead(i & 7, *this));
      out.append(cop0Names[i], ": ", toSignedOrHex(value, decimal));
      columns++;
      if(columns == 4) { out.append("\n"); columns = 0; }
      else out.append("   ");
    }
    if(columns) out.append("\n");
  }

  if(mode == 2) {
    auto dumpVector = [&](cr128& value) -> string {
      string line;
      for(u32 i : range(8)) {
        if(decimal) line.append(pad((s32)value.s16(i), -6L, ' '));
        else        line.append(hex(value.u16(i), 4L));
        if(i != 7) line.append(" ");
        if(i == 3) line.append(" ");
      }
      return line;
    };

    auto dumpFlags = [&](cr128& high, cr128& low) -> string {
      string line;
      for(u32 i : range(8)) {
        line.append(high.get(i) ? "1" : "-");
        if(i == 3) line.append(" ");
      }
      line.append(" ");
      for(u32 i : range(8)) {
        line.append(low.get(i) ? "1" : "-");
        if(i == 3) line.append(" ");
      }
      return line;
    };

    u32 columns = 0;
    for(u32 i : range(32)) {
      if(mask && !(mask & (1u << i))) continue;
      out.append("v", pad(i, 2L, '0'), ": ", dumpVector(vpu.r[i]));
      columns++;
      if(columns == 2) { out.append("\n"); columns = 0; }
      else out.append("   ");
    }
    if(columns) out.append("\n");
    if(extra) {
      out.append("acch: ", dumpVector(vpu.acch), "   vco: ", dumpFlags(vpu.vcoh, vpu.vcol), "\n");
      out.append("accm: ", dumpVector(vpu.accm), "   vcc: ", dumpFlags(vpu.vcch, vpu.vccl), "\n");
      string vce;
      for(u32 i : range(8)) {
        vce.append(vpu.vce.get(i) ? "1" : "-");
        if(i == 3) vce.append(" ");
      }
      out.append("accl: ", dumpVector(vpu.accl), "   vce: ", vce, "\n");
    }
  }

  if(out) emux->notify(out);
}

auto RSP::XHEXDUMP(cr32& rd, cr32& rt, u32 code) -> void {
  if(!system.homebrewMode) return;
  auto& emux = debugger.tracer.emux;
  u32 address = rd.u32;
  u32 length = rt.u32;
  string dump;
  (void)code;

  for(u32 n = 0; n < length; n += 16) {
    dump.append(hex(address + n, 8L), " ", hex(n, 4L), ": ");
    u8 data[16] = {};
    u32 line = min(16u, length - n);
    for(u32 m : range(line)) data[m] = readRspByte(address + n + m);
    for(u32 m : range(16)) {
      if(m < line) dump.append(hex(data[m], 2L), " ");
      else         dump.append("   ");
      if(m == 7) dump.append(" ");
    }
    dump.append(" |");
    for(u32 m : range(line)) {
      u8 c = data[m];
      if(c >= 32 && c <= 126) dump.append(string{(char)c});
      else                    dump.append(".");
    }
    for(u32 m = line; m < 16; m++) dump.append(" ");
    dump.append("|\n");
  }

  if(dump) emux->notify(dump);
}

auto RSP::XPROF(cr32& rd, u32 code) -> void {
  if(!system.homebrewMode) return;
  CPU::r64 slot;
  slot.u64 = rd.u32;
  cpu.XPROF(slot, code);
}

auto RSP::XPROFREAD(cr32& rd, r32& rt) -> void {
  if(!system.homebrewMode) return;
  CPU::r64 slot;
  CPU::r64 metric;
  slot.s64 = s32(rd.u32);
  metric.u64 = rt.u32;
  cpu.XPROFREAD(slot, metric);
  rt.u32 = metric.u64;
}

auto RSP::XEXCEPTION(cr32& rt) -> void {
  if(!system.homebrewMode) return;
  CPU::r64 mask;
  mask.u64 = rt.u32;
  cpu.XEXCEPTION(mask);
}

auto RSP::XIOCTL(u32 code) -> void {
  if(!system.homebrewMode) return;
  cpu.XIOCTL(code);
}
