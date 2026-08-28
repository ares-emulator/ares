namespace {

static const string armv6mRegisterNames[] = {
  "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
  "r8", "r9", "r10", "r11", "r12", "sp", "lr", "pc",
};

static const string armv6mConditionNames[] = {
  "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
  "hi", "ls", "ge", "lt", "gt", "le", "", "",
};

auto armv6mSignExtend(u32 value, u32 bits) -> i32 {
  auto shift = 32 - bits;
  return (i32)(value << shift) >> shift;
}

auto armv6mRegisterList(u32 list, maybe<u32> extra = {}) -> string {
  string output;
  for(u32 index : range(8)) {
    if(list & 1 << index) output.append(armv6mRegisterNames[index], ",");
  }
  if(extra) output.append(armv6mRegisterNames[*extra], ",");
  output.trimRight(",", 1L);
  return output;
}

}

auto ARMv6M::disassembleInstruction(maybe<n32> pc) -> string {
  auto address = pc ? (u32)*pc : (u32)r(15);
  address &= ~1u;

  n32 fetchedPrefix = 0;
  if(getDebugger(Half, address, fetchedPrefix) != Fault::None) return pad("unavailable", -40L);
  auto prefix = (u16)fetchedPrefix;
  auto decoded = decodeInstruction(prefix);

  maybe<n16> suffix;
  if(decoded.width == 4) {
    n32 fetchedSuffix = 0;
    if(getDebugger(Half, address + 2, fetchedSuffix) != Fault::None) return pad("truncated", -40L);
    suffix = (u16)fetchedSuffix;
    decoded = decodeInstruction(prefix, suffix);
  }
  if(!decoded.valid) return pad("undefined", -40L);

  string output;
  switch(decoded.family) {
  case InstructionFamily::ShiftImmediate: {
    static const string operationNames[] = {"lsl", "lsr", "asr"};
    auto operation = prefix >> 11 & 3;
    auto amount = prefix >> 6 & 31;
    if(operation) amount = amount ? amount : 32;
    auto source = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {
      operationNames[operation], " ", armv6mRegisterNames[target],
      ",", armv6mRegisterNames[source], ",#", amount
    };
    break;
  }

  case InstructionFamily::AddSubtract: {
    auto immediate = prefix >> 10 & 1;
    auto subtract = prefix >> 9 & 1;
    auto operand = prefix >> 6 & 7;
    auto source = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {
      subtract ? "sub " : "add ", armv6mRegisterNames[target], ",", armv6mRegisterNames[source], ",",
      immediate ? string{"#", operand} : armv6mRegisterNames[operand]
    };
    break;
  }

  case InstructionFamily::Immediate: {
    static const string operationNames[] = {"mov", "cmp", "add", "sub"};
    auto operation = prefix >> 11 & 3;
    auto target = prefix >> 8 & 7;
    output = {operationNames[operation], " ", armv6mRegisterNames[target], ",#0x", hex(prefix & 0xff, 2L)};
    break;
  }

  case InstructionFamily::DataProcessing: {
    static const string operationNames[] = {
      "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
      "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn",
    };
    auto operation = prefix >> 6 & 15;
    auto source = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {operationNames[operation], " ", armv6mRegisterNames[target], ",", armv6mRegisterNames[source]};
    break;
  }

  case InstructionFamily::HighRegister: {
    auto operation = prefix >> 8 & 3;
    auto target = (prefix & 7) | (prefix >> 4 & 8);
    auto source = prefix >> 3 & 15;
    if(operation == 3) {
      output = {prefix & 0x0080 ? "blx " : "bx ", armv6mRegisterNames[source]};
    } else {
      static const string operationNames[] = {"add", "cmp", "mov"};
      output = {operationNames[operation], " ", armv6mRegisterNames[target], ",", armv6mRegisterNames[source]};
    }
    break;
  }

  case InstructionFamily::LoadLiteral: {
    auto target = prefix >> 8 & 7;
    auto literalAddress = (address + 4 & ~3u) + ((prefix & 0xff) << 2);
    output = {"ldr ", armv6mRegisterNames[target], ",[pc,#0x", hex(literalAddress, 8L), "]"};
    n32 data = 0;
    if(getDebugger(Word, literalAddress, data) == Fault::None) output.append(" =0x", hex((u32)data, 8L));
    break;
  }

  case InstructionFamily::LoadStoreRegister: {
    static const string operationNames[] = {"str", "strh", "strb", "ldsb", "ldr", "ldrh", "ldrb", "ldsh"};
    auto operation = prefix >> 9 & 7;
    auto offset = prefix >> 6 & 7;
    auto base = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {
      operationNames[operation], " ", armv6mRegisterNames[target],
      ",[", armv6mRegisterNames[base], ",", armv6mRegisterNames[offset], "]"
    };
    break;
  }

  case InstructionFamily::LoadStoreImmediate: {
    auto byte = prefix >> 12 & 1;
    auto loading = prefix >> 11 & 1;
    auto immediate = prefix >> 6 & 31;
    auto base = prefix >> 3 & 7;
    auto target = prefix & 7;
    auto offset = byte ? immediate : immediate << 2;
    output = {
      loading ? "ldr" : "str", byte ? "b " : " ", armv6mRegisterNames[target],
      ",[", armv6mRegisterNames[base], ",#0x", hex(offset, 2L), "]"
    };
    break;
  }

  case InstructionFamily::LoadStoreHalf: {
    auto loading = prefix >> 11 & 1;
    auto offset = (prefix >> 6 & 31) << 1;
    auto base = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {
      loading ? "ldrh " : "strh ", armv6mRegisterNames[target],
      ",[", armv6mRegisterNames[base], ",#0x", hex(offset, 2L), "]"
    };
    break;
  }

  case InstructionFamily::LoadStoreStack: {
    auto loading = prefix >> 11 & 1;
    auto target = prefix >> 8 & 7;
    auto offset = (prefix & 0xff) << 2;
    output = {loading ? "ldr " : "str ", armv6mRegisterNames[target], ",[sp,#0x", hex(offset, 3L), "]"};
    break;
  }

  case InstructionFamily::AddAddress: {
    auto useStack = prefix >> 11 & 1;
    auto target = prefix >> 8 & 7;
    auto offset = (prefix & 0xff) << 2;
    if(useStack) output = {"add ", armv6mRegisterNames[target], ",sp,#0x", hex(offset, 3L)};
    else output = {"adr ", armv6mRegisterNames[target], ",0x", hex((address + 4 & ~3u) + offset, 8L)};
    break;
  }

  case InstructionFamily::AdjustStack: {
    auto subtract = prefix >> 7 & 1;
    auto amount = (prefix & 0x7f) << 2;
    output = {subtract ? "sub" : "add", " sp,#0x", hex(amount, 3L)};
    break;
  }

  case InstructionFamily::ChangeInterrupt: {
    string flags;
    if(prefix & 2) flags.append("i");
    if(prefix & 1) flags.append("f");
    output = {prefix & 0x0010 ? "cpsid" : "cpsie", flags ? string{" ", flags} : string{}};
    break;
  }

  case InstructionFamily::SetEndian:
    output = "setend le";
    break;

  case InstructionFamily::Extend: {
    static const string operationNames[] = {"sxth", "sxtb", "uxth", "uxtb"};
    auto operation = prefix >> 6 & 3;
    auto source = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {operationNames[operation], " ", armv6mRegisterNames[target], ",", armv6mRegisterNames[source]};
    break;
  }

  case InstructionFamily::Push: {
    maybe<u32> extra;
    if(prefix & 0x0100) extra = 14;
    output = {"push {", armv6mRegisterList(prefix & 0xff, extra), "}"};
    break;
  }

  case InstructionFamily::Reverse: {
    static const string operationNames[] = {"rev", "rev16", "", "revsh"};
    auto operation = prefix >> 6 & 3;
    auto source = prefix >> 3 & 7;
    auto target = prefix & 7;
    output = {operationNames[operation], " ", armv6mRegisterNames[target], ",", armv6mRegisterNames[source]};
    break;
  }

  case InstructionFamily::Breakpoint:
    output = {"bkpt #0x", hex(prefix & 0xff, 2L)};
    break;

  case InstructionFamily::Hint: {
    auto mask = prefix & 15;
    auto condition = prefix >> 4 & 15;
    if(!mask) {
      static const string hintNames[] = {"nop", "yield", "wfe", "wfi", "sev"};
      output = condition < 5 ? hintNames[condition] : string{"hint #0x", hex(condition, 2L)};
      break;
    }

    string mnemonic = "it";
    auto state = (u32)(prefix & 0xff);
    while(state) {
      if((state & 7) == 0) state = 0;
      else state = (state & 0xe0) | ((state << 1) & 0x1f);
      if(state) mnemonic.append((state >> 4) == condition ? "t" : "e");
    }
    output = {mnemonic, " ", armv6mConditionNames[condition]};
    break;
  }

  case InstructionFamily::Pop: {
    maybe<u32> extra;
    if(prefix & 0x0100) extra = 15;
    output = {"pop {", armv6mRegisterList(prefix & 0xff, extra), "}"};
    break;
  }

  case InstructionFamily::LoadStoreMultiple: {
    auto loading = prefix >> 11 & 1;
    auto base = prefix >> 8 & 7;
    output = {loading ? "ldmia " : "stmia ", armv6mRegisterNames[base], "!,{", armv6mRegisterList(prefix & 0xff), "}"};
    break;
  }

  case InstructionFamily::BranchConditional: {
    auto condition = prefix >> 8 & 15;
    auto displacement = armv6mSignExtend((prefix & 0xff) << 1, 9);
    output = {"b", armv6mConditionNames[condition], " 0x", hex(address + 4 + displacement, 8L)};
    break;
  }

  case InstructionFamily::Branch: {
    auto displacement = armv6mSignExtend((prefix & 0x7ff) << 1, 12);
    output = {"b 0x", hex(address + 4 + displacement, 8L)};
    break;
  }

  case InstructionFamily::BranchLink: {
    auto suffixValue = (u16)*suffix;
    auto sign = prefix >> 10 & 1;
    auto j1 = suffixValue >> 13 & 1;
    auto j2 = suffixValue >> 11 & 1;
    auto i1 = !(j1 ^ sign);
    auto i2 = !(j2 ^ sign);
    auto encoded = sign << 24 | i1 << 23 | i2 << 22
      | (prefix & 0x03ff) << 12 | (suffixValue & 0x07ff) << 1;
    auto target = address + 4 + armv6mSignExtend(encoded, 25);
    output = {"bl 0x", hex(target, 8L)};
    break;
  }

  case InstructionFamily::Undefined:
    return pad("undefined", -40L);
  }

  return pad(output, -40L);
}

auto ARMv6M::disassembleContext() -> string {
  string output;
  for(u32 index : range(16)) {
    output.append(armv6mRegisterNames[index], ":", hex((u32)r(index), 8L), " ");
  }
  output.append("xpsr:");
  output.append(psr().n ? "N" : "n");
  output.append(psr().z ? "Z" : "z");
  output.append(psr().c ? "C" : "c");
  output.append(psr().v ? "V" : "v");
  output.append("/it:", hex((u32)psr().it, 2L));
  return output;
}
