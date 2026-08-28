auto ARMv6M::instruction() -> Fault {
  auto sx = [](u32 value, u32 bits) -> i32 {
    auto shift = 32 - bits;
    return (i32)(value << shift) >> shift;
  };
  auto reg = [&](u32 index) -> u32 {
    return index == 15 ? (u32)instructionAddress + 4 : (u32)r(index);
  };
  auto writeReg = [&](u32 index, u32 value) -> Fault {
    //High-register ADD/MOV use ALUWritePC: remain in Thumb state and discard
    //bit zero.  BX/BLX use branch() below and still validate the Thumb bit.
    if(index == 15) {
      r(15) = value & ~1u;
      return Fault::None;
    }
    r(index) = value;
    return Fault::None;
  };

  instructionAddress = r(15);
  n32 fetched = 0;
  if(auto error = get(Prefetch | Half, instructionAddress, fetched); error != Fault::None) return error;
  opcode = fetched;
  r(15) += 2;
  step(1);
  auto decoded = decodeInstruction(opcode);
  using Family = InstructionFamily;

  //ARMv6-M keeps the IT condition/mask in xPSR.  Advance it before executing
  //the current slot so all of the early-return decode paths share one rule.
  auto activeIT = psr().it != 0;
  auto execute = !activeIT || condition(psr().it >> 4);
  if(activeIT) {
    if((psr().it & 7) == 0) psr().it = 0;
    else psr().it = (psr().it & 0xe0) | ((psr().it << 1) & 0x1f);
  }
  if(!execute) {
    //BL is the only 32-bit ARMv6-M instruction implemented by this core.
    if(decoded.width == 4) {
      n32 suffix = 0;
      if(auto error = get(Prefetch | Half, r(15), suffix); error != Fault::None) return error;
      r(15) += 2;
      step(1);
    }
    return Fault::None;
  }

  //Shift by immediate.
  if(decoded.family == Family::ShiftImmediate) {
    auto operation = opcode >> 11 & 3;
    auto amount = opcode >> 6 & 31;
    auto source = r(opcode >> 3 & 7);
    auto target = opcode & 7;
    if(operation == 0) r(target) = shiftLSL(source, amount);
    if(operation == 1) r(target) = shiftLSR(source, amount);
    if(operation == 2) r(target) = shiftASR(source, amount);
    return Fault::None;
  }

  //Add/subtract register or three-bit immediate.
  if(decoded.family == Family::AddSubtract) {
    auto immediate = opcode >> 10 & 1;
    auto subtract = opcode >> 9 & 1;
    auto operand = immediate ? (u32)(opcode >> 6 & 7) : (u32)r(opcode >> 6 & 7);
    auto source = r(opcode >> 3 & 7);
    auto target = opcode & 7;
    r(target) = subtract ? sub(source, operand, 1) : add(source, operand, 0);
    return Fault::None;
  }

  //Move/compare/add/subtract immediate.
  if(decoded.family == Family::Immediate) {
    auto operation = opcode >> 11 & 3;
    auto target = opcode >> 8 & 7;
    auto immediate = opcode & 0xff;
    if(operation == 0) { r(target) = immediate; setNZ(r(target)); }
    if(operation == 1) sub(r(target), immediate, 1);
    if(operation == 2) r(target) = add(r(target), immediate, 0);
    if(operation == 3) r(target) = sub(r(target), immediate, 1);
    return Fault::None;
  }

  //Data-processing register.
  if(decoded.family == Family::DataProcessing) {
    auto operation = opcode >> 6 & 15;
    auto source = r(opcode >> 3 & 7);
    auto target = opcode & 7;
    auto value = (u32)r(target);
    switch(operation) {
    case 0x0: r(target) = value & source; setNZ(r(target)); break;
    case 0x1: r(target) = value ^ source; setNZ(r(target)); break;
    case 0x2: r(target) = shiftLSL(value, source & 0xff); break;
    case 0x3: {
      auto amount = source & 0xff;
      r(target) = amount ? shiftLSR(value, amount) : value;
      if(!amount) setNZ(value);
      break;
    }
    case 0x4: {
      auto amount = source & 0xff;
      r(target) = amount ? shiftASR(value, amount) : value;
      if(!amount) setNZ(value);
      break;
    }
    case 0x5: r(target) = add(value, source, psr().c); break;
    case 0x6: r(target) = sub(value, source, psr().c); break;
    case 0x7: r(target) = shiftROR(value, source & 0xff); break;
    case 0x8: setNZ(value & source); break;
    case 0x9: r(target) = sub(0, source, 1); break;
    case 0xa: sub(value, source, 1); break;
    case 0xb: add(value, source, 0); break;
    case 0xc: r(target) = value | source; setNZ(r(target)); break;
    case 0xd: r(target) = value * source; setNZ(r(target)); break;
    case 0xe: r(target) = value & ~source; setNZ(r(target)); break;
    case 0xf: r(target) = ~source; setNZ(r(target)); break;
    }
    return Fault::None;
  }

  //High-register operations and branch/exchange.
  if(decoded.family == Family::HighRegister) {
    auto operation = opcode >> 8 & 3;
    auto target = (opcode & 7) | (opcode >> 4 & 8);
    auto source = opcode >> 3 & 15;
    if(operation == 0) return writeReg(target, reg(target) + reg(source));
    if(operation == 1) { sub(reg(target), reg(source), 1); return Fault::None; }
    if(operation == 2) return writeReg(target, reg(source));
    auto destination = reg(source);
    if(opcode & 0x0080) r(14) = (instructionAddress + 2) | 1;
    return branch(destination);
  }

  //PC-relative literal load.
  if(decoded.family == Family::LoadLiteral) {
    auto target = opcode >> 8 & 7;
    auto address = (instructionAddress + 4 & ~3u) + ((opcode & 0xff) << 2);
    return load(Word, address, r(target));
  }

  //Register-offset loads and stores.
  if(decoded.family == Family::LoadStoreRegister) {
    auto operation = opcode >> 9 & 7;
    auto source = opcode >> 6 & 7;
    auto base = opcode >> 3 & 7;
    auto target = opcode & 7;
    auto address = r(base) + r(source);
    n32 value = 0;
    if(operation == 0) return store(Word, address, r(target));
    if(operation == 1) return store(Half, address, r(target));
    if(operation == 2) return store(Byte, address, r(target));
    if(operation == 3) {
      if(auto error = load(Byte | Signed, address, value); error != Fault::None) return error;
      r(target) = (i8)value;
      return Fault::None;
    }
    if(operation == 4) return load(Word, address, r(target));
    if(operation == 5) return load(Half, address, r(target));
    if(operation == 6) return load(Byte, address, r(target));
    if(auto error = load(Half | Signed, address, value); error != Fault::None) return error;
    r(target) = (i16)value;
    return Fault::None;
  }

  //Immediate word/byte load/store.
  if(decoded.family == Family::LoadStoreImmediate) {
    auto byte = opcode >> 12 & 1;
    auto loading = opcode >> 11 & 1;
    auto immediate = opcode >> 6 & 31;
    auto base = opcode >> 3 & 7;
    auto target = opcode & 7;
    auto mode = byte ? Byte : Word;
    auto address = r(base) + (byte ? immediate : immediate << 2);
    if(loading) return load(mode, address, r(target));
    return store(mode, address, r(target));
  }

  //Immediate halfword load/store.
  if(decoded.family == Family::LoadStoreHalf) {
    auto loading = opcode >> 11 & 1;
    auto address = r(opcode >> 3 & 7) + ((opcode >> 6 & 31) << 1);
    auto target = opcode & 7;
    if(loading) return load(Half, address, r(target));
    return store(Half, address, r(target));
  }

  //SP-relative word load/store.
  if(decoded.family == Family::LoadStoreStack) {
    auto target = opcode >> 8 & 7;
    auto address = r(13) + ((opcode & 0xff) << 2);
    if(opcode & 0x0800) return load(Word, address, r(target));
    return store(Word, address, r(target));
  }

  //Form address from PC or SP.
  if(decoded.family == Family::AddAddress) {
    auto target = opcode >> 8 & 7;
    auto base = opcode & 0x0800 ? (u32)r(13) : (u32)(instructionAddress + 4) & ~3u;
    r(target) = base + ((opcode & 0xff) << 2);
    return Fault::None;
  }

  //Adjust SP.
  if(decoded.family == Family::AdjustStack) {
    auto amount = (opcode & 0x7f) << 2;
    r(13) = opcode & 0x0080 ? r(13) - amount : r(13) + amount;
    return Fault::None;
  }

  //CPSIE/CPSID.  The isolated cartridge runtime has no architectural
  //interrupt sources, but compiler startup code is allowed to change PRIMASK.
  if(decoded.family == Family::ChangeInterrupt) return Fault::None;

  //The executable format is fixed little-endian.  SETEND LE is therefore a
  //no-op; accepting SETEND BE would make all mapped data semantics ambiguous.
  if(decoded.family == Family::SetEndian) return decoded.valid ? Fault::None : Fault::Undefined;

  //Sign/zero extension.
  if(decoded.family == Family::Extend) {
    auto operation = opcode >> 6 & 3;
    auto source = r(opcode >> 3 & 7);
    auto target = opcode & 7;
    if(operation == 0) r(target) = (i16)source;
    if(operation == 1) r(target) = (i8)source;
    if(operation == 2) r(target) = (u16)source;
    if(operation == 3) r(target) = (u8)source;
    return Fault::None;
  }

  //Push registers.
  if(decoded.family == Family::Push) {
    auto count = (u32)bit::count(opcode & 0xff) + (opcode >> 8 & 1);
    auto address = r(13) - count * 4;
    auto cursor = address;
    for(u32 index = 0; index < 8; index++) if(opcode & 1 << index) {
      if(auto error = store(Word, cursor, r(index)); error != Fault::None) return error;
      cursor += 4;
    }
    if(opcode & 0x0100) {
      if(auto error = store(Word, cursor, r(14)); error != Fault::None) return error;
    }
    r(13) = address;
    return Fault::None;
  }

  //Byte reversal.
  if(decoded.family == Family::Reverse) {
    if(!decoded.valid) return Fault::Undefined;
    auto operation = opcode >> 6 & 3;
    auto value = r(opcode >> 3 & 7);
    auto target = opcode & 7;
    if(operation == 0) r(target) = bswap32(value);
    else if(operation == 1) r(target) = (value >> 8 & 0x00ff'00ff) | (value << 8 & 0xff00'ff00);
    else if(operation == 3) r(target) = (i16)bswap16((u16)value);
    else return Fault::Undefined;
    return Fault::None;
  }

  if(decoded.family == Family::Breakpoint) return Fault::Breakpoint;
  if(decoded.family == Family::Hint) {
    if(opcode & 15) {
      auto code = opcode >> 4 & 15;
      if(activeIT || !decoded.valid) return Fault::Undefined;
      psr().it = opcode;
    }
    //NOP/YIELD/WFE/WFI/SEV are deterministic no-ops for this isolated runtime.
    return Fault::None;
  }

  //Pop registers.
  if(decoded.family == Family::Pop) {
    auto address = r(13);
    for(u32 index = 0; index < 8; index++) if(opcode & 1 << index) {
      if(auto error = load(Word, address, r(index)); error != Fault::None) return error;
      address += 4;
    }
    if(opcode & 0x0100) {
      n32 target = 0;
      if(auto error = load(Word, address, target); error != Fault::None) return error;
      address += 4;
      r(13) = address;
      return branch(target);
    }
    r(13) = address;
    return Fault::None;
  }

  //Store/load multiple.
  if(decoded.family == Family::LoadStoreMultiple) {
    auto loading = opcode >> 11 & 1;
    auto base = opcode >> 8 & 7;
    auto list = opcode & 0xff;
    if(!decoded.valid) return Fault::Undefined;
    auto address = r(base);
    for(u32 index = 0; index < 8; index++) if(list & 1 << index) {
      if(loading) {
        if(auto error = load(Word, address, r(index)); error != Fault::None) return error;
      } else {
        if(auto error = store(Word, address, r(index)); error != Fault::None) return error;
      }
      address += 4;
    }
    if(!loading || !(list & 1 << base)) r(base) = address;
    return Fault::None;
  }

  //Conditional branch and supervisor call.
  if(decoded.family == Family::BranchConditional) {
    auto code = opcode >> 8 & 15;
    if(!decoded.valid) return Fault::Undefined;
    if(condition(code)) return branch((instructionAddress + 4 + sx((opcode & 0xff) << 1, 9)) | 1);
    return Fault::None;
  }

  //Unconditional branch.
  if(decoded.family == Family::Branch) {
    return branch((instructionAddress + 4 + sx((opcode & 0x7ff) << 1, 12)) | 1);
  }

  //BL immediate (ARMv6-M Thumb-1 pair).
  if(decoded.family == Family::BranchLink) {
    n32 fetchedSuffix = 0;
    if(auto error = get(Prefetch | Half, r(15), fetchedSuffix); error != Fault::None) return error;
    n16 suffix = fetchedSuffix;
    if(!decodeInstruction(opcode, suffix).valid) return Fault::Undefined;
    r(15) += 2;
    step(1);
    auto sign = opcode >> 10 & 1;
    auto j1 = suffix >> 13 & 1;
    auto j2 = suffix >> 11 & 1;
    auto i1 = !(j1 ^ sign);
    auto i2 = !(j2 ^ sign);
    auto encoded = sign << 24 | i1 << 23 | i2 << 22
      | (opcode & 0x03ff) << 12 | (suffix & 0x07ff) << 1;
    auto target = instructionAddress + 4 + sx(encoded, 25);
    r(14) = (instructionAddress + 4) | 1;
    return branch(target | 1);
  }

  return Fault::Undefined;
}
