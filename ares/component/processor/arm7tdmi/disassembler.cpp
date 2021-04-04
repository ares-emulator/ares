static const string _r[] = {
  "r0", "r1",  "r2",  "r3",  "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc"
};
static const string _conditions[] = {
  "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
  "hi", "ls", "ge", "lt", "gt", "le", "",   "nv",
};
#define _s save ? "s" : ""
#define _move(mode) (mode == 13 || mode == 15)
#define _comp(mode) (mode >=  8 && mode <= 11)
#define _math(mode) (mode <=  7 || mode == 12 || mode == 14)

auto ARM7TDMI::disassembleInstruction(maybe<n32> pc, maybe<boolean> thumb) -> string {
  if(!pc) pc = pipeline.execute.address;
  if(!thumb) thumb = cpsr().t;

  _pc = pc();
  if(!thumb()) {
    n32 opcode = read(Word | Nonsequential, _pc & ~3);
    n12 index = (opcode & 0x0ff00000) >> 16 | (opcode & 0x000000f0) >> 4;
    _c = _conditions[opcode >> 28];
    return pad(armDisassemble[index](opcode), -40);
  } else {
    n16 opcode = read(Half | Nonsequential, _pc & ~1);
    return pad(thumbDisassemble[opcode](), -40);
  }
}

auto ARM7TDMI::disassembleContext() -> string {
  string output;
  for(u32 n : range(16)) {
    output.append(_r[n], ":", hex((u32)r(n), 8L), " ");
  }

  output.append("cpsr:");
  output.append(cpsr().n ? "N" : "n");
  output.append(cpsr().z ? "Z" : "z");
  output.append(cpsr().c ? "C" : "c");
  output.append(cpsr().v ? "V" : "v", "/");
  output.append(cpsr().i ? "I" : "i");
  output.append(cpsr().f ? "F" : "f");
  output.append(cpsr().t ? "T" : "t", "/");
  output.append(hex(cpsr().m, 2L));
  if(cpsr().m == PSR::USR || cpsr().m == PSR::SYS) return output;

  output.append(" spsr:");
  output.append(spsr().n ? "N" : "n");
  output.append(spsr().z ? "Z" : "z");
  output.append(spsr().c ? "C" : "c");
  output.append(spsr().v ? "V" : "v", "/");
  output.append(spsr().i ? "I" : "i");
  output.append(spsr().f ? "F" : "f");
  output.append(spsr().t ? "T" : "t", "/");
  output.append(hex(spsr().m, 2L));
  return output;
}

//

auto ARM7TDMI::armDisassembleBranch
(i24 displacement, n1 link) -> string {
  return {"b", link ? "l" : "", _c, " 0x", hex(_pc + 8 + displacement * 4, 8L)};
}

auto ARM7TDMI::armDisassembleBranchExchangeRegister
(n4 m) -> string {
  return {"bx", _c, " ", _r[m]};
}

auto ARM7TDMI::armDisassembleDataImmediate
(n8 immediate, n4 shift, n4 d, n4 n, n1 save, n4 mode) -> string {
  static const string opcode[] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn",
  };
  n32 data = immediate >> (shift << 1) | immediate << 32 - (shift << 1);
  return {opcode[mode], _c,
    _move(mode) ? string{_s, " ", _r[d]} : string{},
    _comp(mode) ? string{" ", _r[n]} : string{},
    _math(mode) ? string{_s, " ", _r[d], ",", _r[n]} : string{},
    ",#0x", hex(data, 8L)};
}

auto ARM7TDMI::armDisassembleDataImmediateShift
(n4 m, n2 type, n5 shift, n4 d, n4 n, n1 save, n4 mode) -> string {
  static const string opcode[] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn",
  };
  return {opcode[mode], _c,
    _move(mode) ? string{_s, " ", _r[d]} : string{},
    _comp(mode) ? string{" ", _r[n]} : string{},
    _math(mode) ? string{_s, " ", _r[d], ",", _r[n]} : string{},
    ",", _r[m],
    type == 0 && shift ? string{" lsl #", shift} : string{},
    type == 1 ? string{" lsr #", shift ? (u32)shift : 32} : string{},
    type == 2 ? string{" asr #", shift ? (u32)shift : 32} : string{},
    type == 3 && shift ? string{" ror #", shift} : string{},
    type == 3 && !shift ? " rrx" : ""};
}

auto ARM7TDMI::armDisassembleDataRegisterShift
(n4 m, n2 type, n4 s, n4 d, n4 n, n1 save, n4 mode) -> string {
  static const string opcode[] = {
    "and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc",
    "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn",
  };
  return {opcode[mode], _c,
    _move(mode) ? string{_s, " ", _r[d]} : string{},
    _comp(mode) ? string{" ", _r[n]} : string{},
    _math(mode) ? string{_s, " ", _r[d], ",", _r[n]} : string{},
    ",", _r[m], " ",
    type == 0 ? "lsl" : "",
    type == 1 ? "lsr" : "",
    type == 2 ? "asr" : "",
    type == 3 ? "ror" : "",
    " ", _r[s]};
}

auto ARM7TDMI::armDisassembleLoadImmediate
(n8 immediate, n1 half, n4 d, n4 n, n1 writeback, n1 up, n1 pre) -> string {
  string data;
  if(n == 15) data = {" =0x", hex(read((half ? Half : Byte) | Nonsequential,
    _pc + 8 + (up ? +immediate : -immediate)), half ? 4L : 2L)};

  return {"ldr", _c, half ? "sh" : "sb", " ",
    _r[d], ",[", _r[n],
    pre == 0 ? "]" : "",
    immediate ? string{",", up ? "+" : "-", "0x", hex(immediate, 2L)} : string{},
    pre == 1 ? "]" : "",
    pre == 0 || writeback ? "!" : "", data};
}

auto ARM7TDMI::armDisassembleLoadRegister
(n4 m, n1 half, n4 d, n4 n, n1 writeback, n1 up, n1 pre) -> string {
  return {"ldr", _c, half ? "sh" : "sb", " ",
    _r[d], ",[", _r[n],
    pre == 0 ? "]" : "",
    ",", up ? "+" : "-", _r[m],
    pre == 1 ? "]" : "",
    pre == 0 || writeback ? "!" : ""};
}

auto ARM7TDMI::armDisassembleMemorySwap
(n4 m, n4 d, n4 n, n1 byte) -> string {
  return {"swp", _c, byte ? "b" : "", " ", _r[d], ",", _r[m], ",[", _r[n], "]"};
}

auto ARM7TDMI::armDisassembleMoveHalfImmediate
(n8 immediate, n4 d, n4 n, n1 mode, n1 writeback, n1 up, n1 pre) -> string {
  string data;
  if(n == 15) data = {" =0x", hex(read(Half | Nonsequential, _pc + (up ? +immediate : -immediate)), 4L)};

  return {mode ? "ldr" : "str", _c, "h ",
    _r[d], ",[", _r[n],
    pre == 0 ? "]" : "",
    immediate ? string{",", up ? "+" : "-", "0x", hex(immediate, 2L)} : string{},
    pre == 1 ? "]" : "",
    pre == 0 || writeback ? "!" : "", data};
}

auto ARM7TDMI::armDisassembleMoveHalfRegister
(n4 m, n4 d, n4 n, n1 mode, n1 writeback, n1 up, n1 pre) -> string {
  return {mode ? "ldr" : "str", _c, "h ",
    _r[d], ",[", _r[n],
    pre == 0 ? "]" : "",
    ",", up ? "+" : "-", _r[m],
    pre == 1 ? "]" : "",
    pre == 0 || writeback ? "!" : ""};
}

auto ARM7TDMI::armDisassembleMoveImmediateOffset
(n12 immediate, n4 d, n4 n, n1 mode, n1 writeback, n1 byte, n1 up, n1 pre) -> string {
  string data;
  if(n == 15) data = {" =0x", hex(read((byte ? Byte : Word) | Nonsequential,
    _pc + 8 + (up ? +immediate : -immediate)), byte ? 2L : 4L)};
  return {mode ? "ldr" : "str", _c, byte ? "b" : "", " ", _r[d], ",[", _r[n],
    pre == 0 ? "]" : "",
    immediate ? string{",", up ? "+" : "-", "0x", hex(immediate, 3L)} : string{},
    pre == 1 ? "]" : "",
    pre == 0 || writeback ? "!" : "", data};
}

auto ARM7TDMI::armDisassembleMoveMultiple
(n16 list, n4 n, n1 mode, n1 writeback, n1 type, n1 up, n1 pre) -> string {
  string registers;
  for(u32 index : range(16)) {
    if(list.bit(index)) registers.append(_r[index], ",");
  }
  registers.trimRight(",", 1L);
  return {mode ? "ldm" : "stm", _c,
    up == 0 && pre == 0 ? "da" : "",
    up == 0 && pre == 1 ? "db" : "",
    up == 1 && pre == 0 ? "ia" : "",
    up == 1 && pre == 1 ? "ib" : "",
    " ", _r[n], writeback ? "!" : "",
    ",{", registers, "}", type ? "^" : ""};
}

auto ARM7TDMI::armDisassembleMoveRegisterOffset
(n4 m, n2 type, n5 shift, n4 d, n4 n, n1 mode, n1 writeback, n1 byte, n1 up, n1 pre) -> string {
  return {mode ? "ldr" : "str", _c, byte ? "b" : "", " ", _r[d], ",[", _r[n],
    pre == 0 ? "]" : "",
    ",", up ? "+" : "-", _r[m],
    type == 0 && shift ? string{" lsl #", shift} : string{},
    type == 1 ? string{" lsr #", shift ? (u32)shift : 32} : string{},
    type == 2 ? string{" asr #", shift ? (u32)shift : 32} : string{},
    type == 3 && shift ? string{" ror #", shift} : string{},
    type == 3 && !shift ? " rrx" : "",
    pre == 1 ? "]" : "",
    pre == 0 || writeback ? "!" : ""};
}

auto ARM7TDMI::armDisassembleMoveToRegisterFromStatus
(n4 d, n1 mode) -> string {
  return {"mrs", _c, " ", _r[d], ",", mode ? "spsr" : "cpsr"};
}

auto ARM7TDMI::armDisassembleMoveToStatusFromImmediate
(n8 immediate, n4 rotate, n4 field, n1 mode) -> string {
  n32 data = immediate >> (rotate << 1) | immediate << 32 - (rotate << 1);
  return {"msr", _c, " ", mode ? "spsr:" : "cpsr:",
    field.bit(0) ? "c" : "",
    field.bit(1) ? "x" : "",
    field.bit(2) ? "s" : "",
    field.bit(3) ? "f" : "",
    ",#0x", hex(data, 8L)};
}

auto ARM7TDMI::armDisassembleMoveToStatusFromRegister
(n4 m, n4 field, n1 mode) -> string {
  return {"msr", _c, " ", mode ? "spsr:" : "cpsr:",
    field.bit(0) ? "c" : "",
    field.bit(1) ? "x" : "",
    field.bit(2) ? "s" : "",
    field.bit(3) ? "f" : "",
    ",", _r[m]};
}

auto ARM7TDMI::armDisassembleMultiply
(n4 m, n4 s, n4 n, n4 d, n1 save, n1 accumulate) -> string {
  if(accumulate) {
    return {"mla", _c, _s, " ", _r[d], ",", _r[m], ",", _r[s], ",", _r[n]};
  } else {
    return {"mul", _c, _s, " ", _r[d], ",", _r[m], ",", _r[s]};
  }
}

auto ARM7TDMI::armDisassembleMultiplyLong
(n4 m, n4 s, n4 l, n4 h, n1 save, n1 accumulate, n1 sign) -> string {
  return {sign ? "s" : "u", accumulate ? "mlal" : "mull", _c, _s, " ",
    _r[l], ",", _r[h], ",", _r[m], ",", _r[s]};
}

auto ARM7TDMI::armDisassembleSoftwareInterrupt
(n24 immediate) -> string {
  return {"swi #0x", hex(immediate, 6L)};
}

auto ARM7TDMI::armDisassembleUndefined
() -> string {
  return {"undefined"};
}

//

auto ARM7TDMI::thumbDisassembleALU
(n3 d, n3 m, n4 mode) -> string {
  static const string opcode[] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
    "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn",
  };
  return {opcode[mode], " ", _r[d], ",", _r[m]};
}

auto ARM7TDMI::thumbDisassembleALUExtended
(n4 d, n4 m, n2 mode) -> string {
  static const string opcode[] = {"add", "sub", "mov"};
  if(d == 8 && m == 8 && mode == 2) return {"nop"};
  return {opcode[mode], " ", _r[d], ",", _r[m]};
}

auto ARM7TDMI::thumbDisassembleAddRegister
(n8 immediate, n3 d, n1 mode) -> string {
  return {"add ", _r[d], ",", mode ? "sp" : "pc", ",#0x", hex(immediate, 2L)};
}

auto ARM7TDMI::thumbDisassembleAdjustImmediate
(n3 d, n3 n, n3 immediate, n1 mode) -> string {
  return {!mode ? "add" : "sub", " ", _r[d], ",", _r[n], ",#", immediate};
}

auto ARM7TDMI::thumbDisassembleAdjustRegister
(n3 d, n3 n, n3 m, n1 mode) -> string {
  return {!mode ? "add" : "sub", " ", _r[d], ",", _r[n], ",", _r[m]};
}

auto ARM7TDMI::thumbDisassembleAdjustStack
(n7 immediate, n1 mode) -> string {
  return {!mode ? "add" : "sub", " sp,#0x", hex(immediate * 4, 3L)};
}

auto ARM7TDMI::thumbDisassembleBranchExchange
(n4 m) -> string {
  return {"bx ", _r[m]};
}

auto ARM7TDMI::thumbDisassembleBranchFarPrefix
(i11 displacementHi) -> string {
  n11 displacementLo = read(Half | Nonsequential, (_pc & ~1) + 2);
  i22 displacement = displacementHi << 11 | displacementLo << 0;
  n32 address = _pc + 4 + displacement * 2;
  return {"bl 0x", hex(address, 8L)};
}

auto ARM7TDMI::thumbDisassembleBranchFarSuffix
(n11 displacement) -> string {
  return {"bl (suffix)"};
}

auto ARM7TDMI::thumbDisassembleBranchNear
(i11 displacement) -> string {
  n32 address = _pc + 4 + displacement * 2;
  return {"b 0x", hex(address, 8L)};
}

auto ARM7TDMI::thumbDisassembleBranchTest
(i8 displacement, n4 condition) -> string {
  n32 address = _pc + 4 + displacement * 2;
  return {"b", _conditions[condition], " 0x", hex(address, 8L)};
}

auto ARM7TDMI::thumbDisassembleImmediate
(n8 immediate, n3 d, n2 mode) -> string {
  static const string opcode[] = {"mov", "cmp", "add", "sub"};
  return {opcode[mode], " ", _r[d], ",#0x", hex(immediate, 2L)};
}

auto ARM7TDMI::thumbDisassembleLoadLiteral
(n8 displacement, n3 d) -> string {
  n32 address = ((_pc + 4) & ~3) + (displacement << 2);
  n32 data = read(Word | Nonsequential, address);
  return {"ldr ", _r[d], ",[pc,#0x", hex(address, 8L), "] =0x", hex(data, 8L)};
}

auto ARM7TDMI::thumbDisassembleMoveByteImmediate
(n3 d, n3 n, n5 offset, n1 mode) -> string {
  return {mode ? "ldrb" : "strb", " ", _r[d], ",[", _r[n], ",#0x", hex(offset, 2L), "]"};
}

auto ARM7TDMI::thumbDisassembleMoveHalfImmediate
(n3 d, n3 n, n5 offset, n1 mode) -> string {
  return {mode ? "ldrh" : "strh", " ", _r[d], ",[", _r[n], ",#0x", hex(offset * 2, 2L), "]"};
}

auto ARM7TDMI::thumbDisassembleMoveMultiple
(n8 list, n3 n, n1 mode) -> string {
  string registers;
  for(u32 m : range(8)) {
    if(list.bit(m)) registers.append(_r[m], ",");
  }
  registers.trimRight(",", 1L);
  return {mode ? "ldmia" : "stmia", " ", _r[n], "!,{", registers, "}"};
}

auto ARM7TDMI::thumbDisassembleMoveRegisterOffset
(n3 d, n3 n, n3 m, n3 mode) -> string {
  static const string opcode[] = {"str", "strh", "strb", "ldsb", "ldr", "ldrh", "ldrb", "ldsh"};
  return {opcode[mode], " ", _r[d], ",[", _r[n], ",", _r[m], "]"};
}

auto ARM7TDMI::thumbDisassembleMoveStack
(n8 immediate, n3 d, n1 mode) -> string {
  return {mode ? "ldr" : "str", " ", _r[d], ",[sp,#0x", hex(immediate * 4, 3L), "]"};
}

auto ARM7TDMI::thumbDisassembleMoveWordImmediate
(n3 d, n3 n, n5 offset, n1 mode) -> string {
  return {mode ? "ldr" : "str", " ", _r[d], ",[", _r[n], ",#0x", hex(offset * 4, 2L), "]"};
}

auto ARM7TDMI::thumbDisassembleShiftImmediate
(n3 d, n3 m, n5 immediate, n2 mode) -> string {
  static const string opcode[] = {"lsl", "lsr", "asr"};
  return {opcode[mode], " ", _r[d], ",", _r[m], ",#", immediate};
}

auto ARM7TDMI::thumbDisassembleSoftwareInterrupt
(n8 immediate) -> string {
  return {"swi #0x", hex(immediate, 2L)};
}

auto ARM7TDMI::thumbDisassembleStackMultiple
(n8 list, n1 lrpc, n1 mode) -> string {
  string registers;
  for(u32 m : range(8)) {
    if(list.bit(m)) registers.append(_r[m], ",");
  }
  if(lrpc) registers.append(!mode ? "lr," : "pc,");
  registers.trimRight(",", 1L);
  return {!mode ? "push" : "pop", " {", registers, "}"};
}

auto ARM7TDMI::thumbDisassembleUndefined
() -> string {
  return {"undefined"};
}

#undef _s
#undef _move
#undef _comp
#undef _save
