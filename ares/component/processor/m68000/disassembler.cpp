template<> auto M68000::_read<Byte>(n32 address) -> n32 {
  if(address & 1) {
    return read(0, 1, address & ~1).byte(0);
  } else {
    return read(1, 0, address & ~1).byte(1);
  }
}

template<> auto M68000::_read<Word>(n32 address) -> n32 {
  return read(1, 1, address & ~1);
}

template<> auto M68000::_read<Long>(n32 address) -> n32 {
  n32    data = _read<Word>(address + 0) << 16;
  return data | _read<Word>(address + 2) <<  0;
}

template<u32 Size> auto M68000::_readPC() -> n32 {
  auto data = _read<Size == Byte ? Word : Size>(_pc);
  _pc += Size == Long ? 4 : 2;
  return clip<Size>(data);
}

auto M68000::_readDisplacement(n32 base) -> n32 {
  return base + (i16)_readPC<Word>();
}

auto M68000::_readIndex(n32 base) -> n32 {
  auto extension = _readPC<Word>();
  auto index = extension & 0x8000
  ? read(AddressRegister{extension >> 12})
  : read(DataRegister{extension >> 12});
  if(!(extension & 0x800)) index = (i16)index;
  return base + index + (i8)extension;
}

auto M68000::_dataRegister(DataRegister dr) -> string {
  return {"d", dr.number};
}

auto M68000::_addressRegister(AddressRegister ar) -> string {
  return {"a", ar.number};
}

template<u32 Size> auto M68000::_immediate() -> string {
  return {"#$", hex(_readPC<Size>(), 2 << Size)};
}

template<u32 Size> auto M68000::_address(EffectiveAddress& ea) -> string {
  if(ea.mode ==  2) return {_addressRegister(AddressRegister{ea.reg})};
  if(ea.mode ==  5) return {"$", hex(_readDisplacement(read(AddressRegister{ea.reg})), 6L)};
  if(ea.mode ==  6) return {"$", hex(_readIndex(read(AddressRegister{ea.reg})), 6L)};
  if(ea.mode ==  7) { auto imm = _readPC<Word>(); return {"$", imm >= 0x8000 ? hex((i16)imm, 6L, 'f') : hex((i16)imm, 6L, '0') }; }
  if(ea.mode ==  8) return {"$", hex(_readPC<Long>(), 6L)};
  if(ea.mode ==  9) return {"$", hex(_pc + (i16)_readPC(), 6L)};
  if(ea.mode == 10) return {"$", hex(_readIndex(_pc), 6L)};
  return "???";  //should never occur (modes 0, 1, 3, 4, 11 are not valid for LEA)
}

template<u32 Size> auto M68000::_effectiveAddress(EffectiveAddress& ea) -> string {
  if(ea.mode ==  0) return {_dataRegister(DataRegister{ea.reg})};
  if(ea.mode ==  1) return {_addressRegister(AddressRegister{ea.reg})};
  if(ea.mode ==  2) return {"(", _addressRegister(AddressRegister{ea.reg}), ")"};
  if(ea.mode ==  3) return {"(", _addressRegister(AddressRegister{ea.reg}), ")+"};
  if(ea.mode ==  4) return {"-(", _addressRegister(AddressRegister{ea.reg}), ")"};
  if(ea.mode ==  5) return {"($", hex(_readDisplacement(read(AddressRegister{ea.reg})), 6L), ")"};
  if(ea.mode ==  6) return {"($", hex(_readIndex(read(AddressRegister{ea.reg})), 6L), ")"};
  if(ea.mode ==  7) { auto imm = _readPC<Word>(); return {"($", imm >= 0x8000 ? hex((i16)imm, 6L, 'f') : hex((i16)imm, 6L, '0'), ")"}; }
  if(ea.mode ==  8) return {"($", hex(_readPC<Long>(), 6L), ")"};
  if(ea.mode ==  9) return {"($", hex(_readDisplacement(_pc), 6L), ")"};
  if(ea.mode == 10) return {"($", hex(_readIndex(_pc), 6L), ")"};
  if(ea.mode == 11) return {"#$", hex(_readPC<Size>(), 2 << Size)};
  return "???";  //should never occur
}

auto M68000::_branch(n8 displacement) -> string {
  n16 extension = _readPC();
  _pc -= 2;
  i32 offset = displacement ? sign<Byte>(displacement) : sign<Word>(extension);
  return {"$", hex(_pc + offset, 6L)};
}

template<u32 Size> auto M68000::_suffix() -> string {
  return Size == Byte ? ".b" : Size == Word ? ".w" : ".l";
}

auto M68000::_condition(n4 condition) -> string {
  static const string conditions[16] = {
    "t ", "f ", "hi", "ls", "cc", "cs", "ne", "eq",
    "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le",
  };
  return conditions[condition];
}

auto M68000::disassembleInstruction(n32 pc) -> string {
  _pc = pc;
  return {hex(_read<Word>(_pc), 4L), "  ", pad(disassembleTable[_readPC()](), -49)};
}

auto M68000::disassembleContext() -> string {
  return {
    "d0:", hex(r.d[0], 8L), " ",
    "d1:", hex(r.d[1], 8L), " ",
    "d2:", hex(r.d[2], 8L), " ",
    "d3:", hex(r.d[3], 8L), " ",
    "d4:", hex(r.d[4], 8L), " ",
    "d5:", hex(r.d[5], 8L), " ",
    "d6:", hex(r.d[6], 8L), " ",
    "d7:", hex(r.d[7], 8L), " ",
    "a0:", hex(r.a[0], 8L), " ",
    "a1:", hex(r.a[1], 8L), " ",
    "a2:", hex(r.a[2], 8L), " ",
    "a3:", hex(r.a[3], 8L), " ",
    "a4:", hex(r.a[4], 8L), " ",
    "a5:", hex(r.a[5], 8L), " ",
    "a6:", hex(r.a[6], 8L), " ",
    "a7:", hex(r.a[7], 8L), " ",
    "sp:", hex(r.sp,   8L), " ",
    r.t ? "T" : "t",
    r.s ? "S" : "s",
    (u32)r.i,
    r.c ? "C" : "c",
    r.v ? "V" : "v",
    r.z ? "Z" : "z",
    r.n ? "N" : "n",
    r.x ? "X" : "x", " ",
  };
}

//

auto M68000::disassembleABCD(EffectiveAddress from, EffectiveAddress with) -> string {
  return {"abcd    ", _effectiveAddress<Byte>(from), ",", _effectiveAddress<Byte>(with)};
}

template<u32 Size> auto M68000::disassembleADD(EffectiveAddress from, DataRegister with) -> string {
  return {"add", _suffix<Size>(), "   ", _effectiveAddress<Size>(from), ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleADD(DataRegister from, EffectiveAddress with) -> string {
  return {"add", _suffix<Size>(), "   ", _dataRegister(from), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleADDA(EffectiveAddress from, AddressRegister with) -> string {
  return {"adda", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _addressRegister(with)};
}

template<u32 Size> auto M68000::disassembleADDI(EffectiveAddress with) -> string {
  return {"addi", _suffix<Size>(), "  ", _immediate<Size>(), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleADDQ(n4 immediate, EffectiveAddress with) -> string {
  return {"addq", _suffix<Size>(), "  #", immediate, ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleADDQ(n4 immediate, AddressRegister with) -> string {
  return {"addq", _suffix<Size>(), "  #", immediate, ",", _addressRegister(with)};
}

template<u32 Size> auto M68000::disassembleADDX(EffectiveAddress from, EffectiveAddress with) -> string {
  return {"addx", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleAND(EffectiveAddress from, DataRegister with) -> string {
  return {"and", _suffix<Size>(), "   ", _effectiveAddress<Size>(from), ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleAND(DataRegister from, EffectiveAddress with) -> string {
  return {"and", _suffix<Size>(), "   ", _dataRegister(from), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleANDI(EffectiveAddress ea) -> string {
  return {"andi", _suffix<Size>(), "  ", _immediate<Size>(), ",", _effectiveAddress<Size>(ea)};
}

auto M68000::disassembleANDI_TO_CCR() -> string {
  return {"andi    ", _immediate<Byte>(), ",ccr"};
}

auto M68000::disassembleANDI_TO_SR() -> string {
  return {"andi    ", _immediate<Word>(), ",sr"};
}

template<u32 Size> auto M68000::disassembleASL(n4 count, DataRegister with) -> string {
  return {"asl", _suffix<Size>(), "   #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleASL(DataRegister from, DataRegister with) -> string {
  return {"asl", _suffix<Size>(), "   ", _dataRegister(from), ",", _dataRegister(with)};
}

auto M68000::disassembleASL(EffectiveAddress with) -> string {
  return {"asl", _suffix<Word>(), "   ", _effectiveAddress<Word>(with)};
}

template<u32 Size> auto M68000::disassembleASR(n4 count, DataRegister modify) -> string {
  return {"asr", _suffix<Size>(), "   #", count, ",", _dataRegister(modify)};
}

template<u32 Size> auto M68000::disassembleASR(DataRegister from, DataRegister modify) -> string {
  return {"asr", _suffix<Size>(), "   ", _dataRegister(from), ",", _dataRegister(modify)};
}

auto M68000::disassembleASR(EffectiveAddress with) -> string {
  return {"asr", _suffix<Word>(), "   ", _effectiveAddress<Word>(with)};
}

auto M68000::disassembleBCC(n4 test, n8 displacement) -> string {
  auto cc = _condition(test);
  return {"b", cc, "     ", _branch(displacement)};
}

template<u32 Size> auto M68000::disassembleBCHG(DataRegister bit, EffectiveAddress with) -> string {
  return {"bchg", _suffix<Size>(), "  ", _dataRegister(bit), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleBCHG(EffectiveAddress with) -> string {
  return {"bchg", _suffix<Size>(), "  ", _immediate<Byte>(), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleBCLR(DataRegister bit, EffectiveAddress with) -> string {
  return {"bclr", _suffix<Size>(), "  ", _dataRegister(bit), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleBCLR(EffectiveAddress with) -> string {
  return {"bclr", _suffix<Size>(), "  ", _immediate<Byte>(), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleBRA(n8 displacement) -> string {
  return {"bra     ", _branch(displacement)};
}

template<u32 Size> auto M68000::disassembleBSET(DataRegister bit, EffectiveAddress with) -> string {
  return {"bset", _suffix<Size>(), "  ", _dataRegister(bit), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleBSET(EffectiveAddress with) -> string {
  return {"bset", _suffix<Size>(), "  ", _immediate<Byte>(), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleBSR(n8 displacement) -> string {
  return {"bsr     ", _branch(displacement)};
}

template<u32 Size> auto M68000::disassembleBTST(DataRegister bit, EffectiveAddress with) -> string {
  return {"btst", _suffix<Size>(), "  ", _dataRegister(bit), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleBTST(EffectiveAddress with) -> string {
  return {"btst", _suffix<Size>(), "  ", _immediate<Byte>(), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleCHK(DataRegister compare, EffectiveAddress maximum) -> string {
  return {"chk", _suffix<Word>(), "   ", _effectiveAddress<Word>(maximum), ",", _dataRegister(compare)};
}

template<u32 Size> auto M68000::disassembleCLR(EffectiveAddress ea) -> string {
  return {"clr", _suffix<Size>(), "   ", _effectiveAddress<Size>(ea)};
}

template<u32 Size> auto M68000::disassembleCMP(EffectiveAddress from, DataRegister with) -> string {
  return {"cmp", _suffix<Size>(), "   ", _effectiveAddress<Size>(from), ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleCMPA(EffectiveAddress from, AddressRegister with) -> string {
  return {"cmpa", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _addressRegister(with)};
}

template<u32 Size> auto M68000::disassembleCMPI(EffectiveAddress with) -> string {
  return {"cmpi", _suffix<Size>(), "  ", _immediate<Size>(), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleCMPM(EffectiveAddress from, EffectiveAddress with) -> string {
  return {"cmpm", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleDBCC(n4 condition, DataRegister with) -> string {
  auto base = _pc;
  auto displacement = (i16)_readPC();
  return {"db", _condition(condition), "    ", _dataRegister(with), ",$", hex(base + displacement, 6L)};
}

auto M68000::disassembleDIVS(EffectiveAddress from, DataRegister with) -> string {
  return {"divs", _suffix<Word>(), "  ", _effectiveAddress<Word>(from), ",", _dataRegister(with)};
}

auto M68000::disassembleDIVU(EffectiveAddress from, DataRegister with) -> string {
  return {"divu", _suffix<Word>(), "  ", _effectiveAddress<Word>(from), ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleEOR(DataRegister from, EffectiveAddress with) -> string {
  return {"eor", _suffix<Size>(), "   ", _dataRegister(from), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleEORI(EffectiveAddress with) -> string {
  return {"eori", _suffix<Size>(), "  ", _immediate<Size>(), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleEORI_TO_CCR() -> string {
  return {"eori    ", _immediate<Byte>(), ",ccr"};
}

auto M68000::disassembleEORI_TO_SR() -> string {
  return {"eori    ", _immediate<Word>(), ",sr"};
}

auto M68000::disassembleEXG(DataRegister x, DataRegister y) -> string {
  return {"exg     ", _dataRegister(x), ",", _dataRegister(y)};
}

auto M68000::disassembleEXG(AddressRegister x, AddressRegister y) -> string {
  return {"exg     ", _addressRegister(x), ",", _addressRegister(y)};
}

auto M68000::disassembleEXG(DataRegister x, AddressRegister y) -> string {
  return {"exg     ", _dataRegister(x), ",", _addressRegister(y)};
}

template<u32 Size> auto M68000::disassembleEXT(DataRegister with) -> string {
  return {"ext", _suffix<Size>(), "   ", _dataRegister(with)};
}

auto M68000::disassembleILLEGAL(n16 code) -> string {
  if(code.bit(12,15) == 0xa) return {"linea   $", hex((n12)code, 3L)};
  if(code.bit(12,15) == 0xf) return {"linef   $", hex((n12)code, 3L)};
  return {"illegal "};
}

auto M68000::disassembleJMP(EffectiveAddress from) -> string {
  return {"jmp     ", _effectiveAddress<Long>(from)};
}

auto M68000::disassembleJSR(EffectiveAddress from) -> string {
  return {"jsr     ", _effectiveAddress<Long>(from)};
}

auto M68000::disassembleLEA(EffectiveAddress from, AddressRegister with) -> string {
  return {"lea     ", _address<Long>(from), ",", _addressRegister(with)};
}

auto M68000::disassembleLINK(AddressRegister with) -> string {
  return {"link    ", _addressRegister(with), ",", _immediate<Word>()};
}

template<u32 Size> auto M68000::disassembleLSL(n4 count, DataRegister with) -> string {
  return {"lsl", _suffix<Size>(), "   #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleLSL(DataRegister from, DataRegister with) -> string {
  return {"lsl", _suffix<Size>(), "   ", _dataRegister(from), ",", _dataRegister(with)};
}

auto M68000::disassembleLSL(EffectiveAddress with) -> string {
  return {"lsl", _suffix<Word>(), "   ", _effectiveAddress<Word>(with)};
}

template<u32 Size> auto M68000::disassembleLSR(n4 count, DataRegister with) -> string {
  return {"lsr", _suffix<Size>(), "   #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleLSR(DataRegister from, DataRegister with) -> string {
  return {"lsr", _suffix<Size>(), "   ", _dataRegister(from), ",", _dataRegister(with)};
}

auto M68000::disassembleLSR(EffectiveAddress with) -> string {
  return {"lsr", _suffix<Word>(), "   ", _effectiveAddress<Word>(with)};
}

template<u32 Size> auto M68000::disassembleMOVE(EffectiveAddress from, EffectiveAddress to) -> string {
  return {"move", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _effectiveAddress<Size>(to)};
}

template<u32 Size> auto M68000::disassembleMOVEA(EffectiveAddress from, AddressRegister to) -> string {
  return {"movea   ", _effectiveAddress<Size>(from), ",", _addressRegister(to)};
}

template<u32 Size> auto M68000::disassembleMOVEM_TO_MEM(EffectiveAddress to) -> string {
  string op{"movem", _suffix<Size>(), " "};

  n16 list = _readPC();
  string regs;
  for(u32 lhs = 0; lhs < 8; lhs++) {
    if(!list.bit(0 + lhs)) continue;
    regs.append(_dataRegister(DataRegister{lhs}));
    if(lhs == 7 || !list.bit(1 + lhs)) { regs.append(","); continue; }
    for(u32 rhs = lhs; rhs < 8; rhs++) {
      if(rhs == 7 || !list.bit(1 + rhs)) { regs.append("-", _dataRegister(DataRegister{rhs}), ","); lhs = rhs; break; }
    }
  }
  regs.trimRight(",");
  if(regs && list >> 8) regs.append("/");
  for(u32 lhs = 0; lhs < 8; lhs++) {
    if(!list.bit(8 + lhs)) continue;
    regs.append(_addressRegister(AddressRegister{lhs}));
    if(lhs == 7 || !list.bit(9 + lhs)) { regs.append(","); continue; }
    for(u32 rhs = lhs; rhs < 8; rhs++) {
      if(rhs == 7 || !list.bit(9 + rhs)) { regs.append("-", _addressRegister(AddressRegister{rhs}), ","); lhs = rhs; break; }
    }
  }
  regs.trimRight(",");
  if(!regs) regs = "-";

  return {op, regs, ",", _effectiveAddress<Size>(to)};
}

template<u32 Size> auto M68000::disassembleMOVEM_TO_REG(EffectiveAddress from) -> string {
  string op{"movem", _suffix<Size>(), " "};

  n16 list = _readPC();
  string regs;
  for(u32 lhs = 0; lhs < 8; lhs++) {
    if(!list.bit(0 + lhs)) continue;
    regs.append(_dataRegister(DataRegister{lhs}));
    if(lhs == 7 || !list.bit(1 + lhs)) { regs.append(","); continue; }
    for(u32 rhs = lhs; rhs < 8; rhs++) {
      if(rhs == 7 || !list.bit(1 + rhs)) { regs.append("-", _dataRegister(DataRegister{rhs}), ","); lhs = rhs; break; }
    }
  }
  regs.trimRight(",");
  if(regs && list >> 8) regs.append("/");
  for(u32 lhs = 0; lhs < 8; lhs++) {
    if(!list.bit(8 + lhs)) continue;
    regs.append(_addressRegister(AddressRegister{lhs}));
    if(lhs == 7 || !list.bit(9 + lhs)) { regs.append(","); continue; }
    for(u32 rhs = lhs; rhs < 8; rhs++) {
      if(rhs == 7 || !list.bit(9 + rhs)) { regs.append("-", _addressRegister(AddressRegister{rhs}), ","); lhs = rhs; break; }
    }
  }
  regs.trimRight(",");
  if(!regs) regs = "-";

  return {op, _effectiveAddress<Size>(from), ",", regs};
}

template<u32 Size> auto M68000::disassembleMOVEP(DataRegister from, EffectiveAddress to) -> string {
  return {"movep", _suffix<Size>(), " ", _dataRegister(from), ",", _effectiveAddress<Size>(to)};
}

template<u32 Size> auto M68000::disassembleMOVEP(EffectiveAddress from, DataRegister to) -> string {
  return {"movep", _suffix<Size>(), " ", _effectiveAddress<Size>(from), ",", _dataRegister(to)};
}

auto M68000::disassembleMOVEQ(n8 immediate, DataRegister to) -> string {
  return {"moveq   #$", hex(immediate, 2L), ",", _dataRegister(to)};
}

auto M68000::disassembleMOVE_FROM_SR(EffectiveAddress to) -> string {
  return {"move    sr,", _effectiveAddress<Word>(to)};
}

auto M68000::disassembleMOVE_TO_CCR(EffectiveAddress from) -> string {
  return {"move    ", _effectiveAddress<Byte>(from), ",ccr"};
}

auto M68000::disassembleMOVE_TO_SR(EffectiveAddress from) -> string {
  return {"move    ", _effectiveAddress<Word>(from), ",sr"};
}

auto M68000::disassembleMOVE_FROM_USP(AddressRegister to) -> string {
  return {"move    usp,", _addressRegister(to)};
}

auto M68000::disassembleMOVE_TO_USP(AddressRegister from) -> string {
  return {"move    ", _addressRegister(from), ",usp"};
}

auto M68000::disassembleMULS(EffectiveAddress from, DataRegister with) -> string {
  return {"muls", _suffix<Word>(), "  ", _effectiveAddress<Word>(from), ",", _dataRegister(with)};
}

auto M68000::disassembleMULU(EffectiveAddress from, DataRegister with) -> string {
  return {"mulu", _suffix<Word>(), "  ", _effectiveAddress<Word>(from), ",", _dataRegister(with)};
}

auto M68000::disassembleNBCD(EffectiveAddress with) -> string {
  return {"nbcd    ", _effectiveAddress<Byte>(with)};
}

template<u32 Size> auto M68000::disassembleNEG(EffectiveAddress with) -> string {
  return {"neg", _suffix<Size>(), "   ", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleNEGX(EffectiveAddress with) -> string {
  return {"negx", _suffix<Size>(), "  ", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleNOP() -> string {
  return {"nop     "};
}

template<u32 Size> auto M68000::disassembleNOT(EffectiveAddress with) -> string {
  return {"not", _suffix<Size>(), "   ", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleOR(EffectiveAddress from, DataRegister with) -> string {
  return {"or", _suffix<Size>(), "    ", _effectiveAddress<Size>(from), ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleOR(DataRegister from, EffectiveAddress with) -> string {
  return {"or", _suffix<Size>(), "    ", _dataRegister(from), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleORI(EffectiveAddress with) -> string {
  return {"ori", _suffix<Size>(), "   ", _immediate<Size>(), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleORI_TO_CCR() -> string {
  return {"ori     ", _immediate<Byte>(), ",ccr"};
}

auto M68000::disassembleORI_TO_SR() -> string {
  return {"ori     ", _immediate<Word>(), ",sr"};
}

auto M68000::disassemblePEA(EffectiveAddress from) -> string {
  return {"pea     ", _effectiveAddress<Long>(from)};
}

auto M68000::disassembleRESET() -> string {
  return {"reset   "};
}

template<u32 Size> auto M68000::disassembleROL(n4 count, DataRegister with) -> string {
  return {"rol", _suffix<Size>(), "   #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleROL(DataRegister from, DataRegister with) -> string {
  return {"rol", _suffix<Size>(), "   ", _dataRegister(from), ",", _dataRegister(with)};
}

auto M68000::disassembleROL(EffectiveAddress with) -> string {
  return {"rol", _suffix<Word>(), "   ", _effectiveAddress<Word>(with)};
}

template<u32 Size> auto M68000::disassembleROR(n4 count, DataRegister with) -> string {
  return {"ror", _suffix<Size>(), "   #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleROR(DataRegister from, DataRegister with) -> string {
  return {"ror", _suffix<Size>(), "   ", _dataRegister(from) ,",", _dataRegister(with)};
}

auto M68000::disassembleROR(EffectiveAddress with) -> string {
  return {"ror", _suffix<Word>(), "   ", _effectiveAddress<Word>(with)};
}

template<u32 Size> auto M68000::disassembleROXL(n4 count, DataRegister with) -> string {
  return {"roxl", _suffix<Size>(), "  #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleROXL(DataRegister from, DataRegister with) -> string {
  return {"roxl", _suffix<Size>(), "  ", _dataRegister(from), ",", _dataRegister(with)};
}

auto M68000::disassembleROXL(EffectiveAddress with) -> string {
  return {"roxl", _suffix<Word>(), "  ", _effectiveAddress<Word>(with)};
}

template<u32 Size> auto M68000::disassembleROXR(n4 count, DataRegister with) -> string {
  return {"roxr", _suffix<Size>(), "  #", count, ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleROXR(DataRegister from, DataRegister with) -> string {
  return {"roxr", _suffix<Size>(), "  ", _dataRegister(from), ",", _dataRegister(with)};
}

auto M68000::disassembleROXR(EffectiveAddress with) -> string {
  return {"roxr", _suffix<Word>(), "  ", _effectiveAddress<Word>(with)};
}

auto M68000::disassembleRTE() -> string {
  return {"rte     "};
}

auto M68000::disassembleRTR() -> string {
  return {"rtr     "};
}

auto M68000::disassembleRTS() -> string {
  return {"rts     "};
}

auto M68000::disassembleSBCD(EffectiveAddress from, EffectiveAddress with) -> string {
  return {"sbcd    ", _effectiveAddress<Byte>(from), ",", _effectiveAddress<Byte>(with)};
}

auto M68000::disassembleSCC(n4 test, EffectiveAddress to) -> string {
  return {"s", _condition(test), "     ", _effectiveAddress<Byte>(to)};
}

auto M68000::disassembleSTOP() -> string {
  return {"stop    ", _immediate<Word>()};
}

template<u32 Size> auto M68000::disassembleSUB(EffectiveAddress from, DataRegister with) -> string {
  return {"sub", _suffix<Size>(), "   ", _effectiveAddress<Size>(from), ",", _dataRegister(with)};
}

template<u32 Size> auto M68000::disassembleSUB(DataRegister from, EffectiveAddress with) -> string {
  return {"sub", _suffix<Size>(), "   ", _dataRegister(from), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleSUBA(EffectiveAddress from, AddressRegister with) -> string {
  return {"suba", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _addressRegister(with)};
}

template<u32 Size> auto M68000::disassembleSUBI(EffectiveAddress with) -> string {
  return {"subi", _suffix<Size>(), "  ", _immediate<Size>(), ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleSUBQ(n4 immediate, EffectiveAddress with) -> string {
  return {"subq", _suffix<Size>(), "  #", immediate, ",", _effectiveAddress<Size>(with)};
}

template<u32 Size> auto M68000::disassembleSUBQ(n4 immediate, AddressRegister with) -> string {
  return {"subq", _suffix<Size>(), "  #", immediate, ",", _addressRegister(with)};
}

template<u32 Size> auto M68000::disassembleSUBX(EffectiveAddress from, EffectiveAddress with) -> string {
  return {"subx", _suffix<Size>(), "  ", _effectiveAddress<Size>(from), ",", _effectiveAddress<Size>(with)};
}

auto M68000::disassembleSWAP(DataRegister with) -> string {
  return {"swap    ", _dataRegister(with)};
}

auto M68000::disassembleTAS(EffectiveAddress with) -> string {
  return {"tas     ", _effectiveAddress<Byte>(with)};
}

auto M68000::disassembleTRAP(n4 vector) -> string {
  return {"trap    #", vector};
}

auto M68000::disassembleTRAPV() -> string {
  return {"trapv   "};
}

template<u32 Size> auto M68000::disassembleTST(EffectiveAddress from) -> string {
  return {"tst", _suffix<Size>(), "   ", _effectiveAddress<Size>(from)};
}

auto M68000::disassembleUNLK(AddressRegister with) -> string {
  return {"unlk    ", _addressRegister(with)};
}
