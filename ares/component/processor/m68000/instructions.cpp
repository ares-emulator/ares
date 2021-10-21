auto M68000::instructionABCD(EffectiveAddress from, EffectiveAddress with) -> void {
  if(from.mode == DataRegisterDirect) idle(2);
  auto target = read<Byte, Hold, Fast>(with);
  auto source = read<Byte>(from);
  auto result = source + target + r.x;
  bool c = false;
  bool v = false;

  if(((target ^ source ^ result) & 0x10) || (result & 0x0f) >= 0x0a) {
    auto previous = result;
    result += 0x06;
    v |= ((~previous & 0x80) & (result & 0x80));
  }

  if(result >= 0xa0) {
    auto previous = result;
    result += 0x60;
    c  = true;
    v |= ((~previous & 0x80) & (result & 0x80));
  }

  prefetch();
  write<Byte>(with, result);

  r.c = c;
  r.v = v;
  r.z = clip<Byte>(result) ? 0 : r.z;
  r.n = sign<Byte>(result) < 0;
  r.x = r.c;
}

template<u32 Size> auto M68000::instructionADD(EffectiveAddress from, DataRegister with) -> void {
  if constexpr(Size == Long) {
    if(from.mode == DataRegisterDirect || from.mode == AddressRegisterDirect || from.mode == Immediate) {
      idle(4);
    } else {
      idle(2);
    }
  }
  auto source = read<Size>(from);
  auto target = read<Size>(with);
  auto result = ADD<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionADD(DataRegister from, EffectiveAddress with) -> void {
  auto source = read<Size>(from);
  auto target = read<Size, Hold>(with);
  auto result = ADD<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionADDA(EffectiveAddress from, AddressRegister with) -> void {
  if(Size != Long || from.mode == DataRegisterDirect || from.mode == AddressRegisterDirect || from.mode == Immediate) {
    idle(4);
  } else {
    idle(2);
  }
  auto source = sign<Size>(read<Size>(from));
  auto target = read<Long>(with);
  prefetch();
  write<Long>(with, source + target);
}

template<u32 Size> auto M68000::instructionADDI(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = extension<Size>();
  auto target = read<Size, Hold>(with);
  auto result = ADD<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionADDQ(n4 immediate, EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = immediate;
  auto target = read<Size, Hold>(with);
  auto result = ADD<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

//Size is ignored: always uses Long
template<u32 Size> auto M68000::instructionADDQ(n4 immediate, AddressRegister with) -> void {
  idle(4);
  auto result = read<Long>(with) + immediate;
  prefetch();
  write<Long>(with, result);
}

template<u32 Size> auto M68000::instructionADDX(EffectiveAddress from, EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(from.mode == DataRegisterDirect) idle(4);
  }
  auto target = read<Size, Hold, Fast>(with);
  auto source = read<Size>(from);
  auto result = ADD<Size, Extend>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionAND(EffectiveAddress from, DataRegister with) -> void {
  if constexpr(Size == Long) {
    if(from.mode == DataRegisterDirect || from.mode == Immediate) {
      idle(4);
    } else {
      idle(2);
    }
  }
  auto source = read<Size>(from);
  auto target = read<Size>(with);
  auto result = AND<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionAND(DataRegister from, EffectiveAddress with) -> void {
  auto source = read<Size>(from);
  auto target = read<Size, Hold>(with);
  auto result = AND<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionANDI(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    //note: m68000um.pdf erroneously lists ANDI.L #,Dn as 14(3/0), but is in fact 16(3/0)
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = extension<Size>();
  auto target = read<Size, Hold>(with);
  auto result = AND<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionANDI_TO_CCR() -> void {
  auto data = extension<Word>();
  writeCCR(readCCR() & data);
  idle(8);
  read<Word>(r.pc);
  prefetch();
}

auto M68000::instructionANDI_TO_SR() -> void {
  if(supervisor()) {
    auto data = extension<Word>();
    writeSR(readSR() & data);
    idle(8);
    read<Word>(r.pc);
  }
  prefetch();
}

template<u32 Size> auto M68000::instructionASL(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ASL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionASL(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ASL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionASL(EffectiveAddress with) -> void {
  auto result = ASL<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

template<u32 Size> auto M68000::instructionASR(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ASR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionASR(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ASR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionASR(EffectiveAddress with) -> void {
  auto result = ASR<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

auto M68000::instructionBCC(n4 test, n8 displacement) -> void {
  if(!condition(test)) {
    idle(4);
    if(!displacement) prefetch();
  } else {
    idle(2);
    auto offset = displacement ? (s8)displacement : (s16)prefetched() - 2;
    r.pc -= 2;
    r.pc += offset;
    prefetch();
  }
  prefetch();
}

template<u32 Size> auto M68000::instructionBCHG(DataRegister bit, EffectiveAddress with) -> void {
  auto index = read<Size>(bit) & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(index < 16 ? 2 : 4);
  }
  auto test = read<Size, Hold>(with);
  r.z = test.bit(index) == 0;
  test.bit(index) ^= 1;
  prefetch();
  write<Size>(with, test);
}

template<u32 Size> auto M68000::instructionBCHG(EffectiveAddress with) -> void {
  auto index = extension<Word>() & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(index < 16 ? 2 : 4);
  }
  auto test = read<Size, Hold>(with);
  r.z = test.bit(index) == 0;
  test.bit(index) ^= 1;
  prefetch();
  write<Size>(with, test);
}

template<u32 Size> auto M68000::instructionBCLR(DataRegister bit, EffectiveAddress with) -> void {
  auto index = read<Size>(bit) & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(index < 16 ? 4 : 6);
  }
  auto test = read<Size, Hold>(with);
  r.z = test.bit(index) == 0;
  test.bit(index) = 0;
  prefetch();
  write<Size>(with, test);
}

template<u32 Size> auto M68000::instructionBCLR(EffectiveAddress with) -> void {
  auto index = extension<Word>() & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(index < 16 ? 4 : 6);
  }
  auto test = read<Size, Hold>(with);
  r.z = test.bit(index) == 0;
  test.bit(index) = 0;
  prefetch();
  write<Size>(with, test);
}

auto M68000::instructionBRA(n8 displacement) -> void {
  idle(2);
  auto offset = displacement ? (s8)displacement : (s16)prefetched() - 2;
  r.pc -= 2;
  r.pc += offset;
  prefetch();
  prefetch();
}

template<u32 Size> auto M68000::instructionBSET(DataRegister bit, EffectiveAddress with) -> void {
  auto index = read<Size>(bit) & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(index < 16 ? 2 : 4);
  }
  auto test = read<Size, Hold>(with);
  r.z = test.bit(index) == 0;
  test.bit(index) = 1;
  prefetch();
  write<Size>(with, test);
}

template<u32 Size> auto M68000::instructionBSET(EffectiveAddress with) -> void {
  auto index = extension<Word>() & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(index < 16 ? 2 : 4);
  }
  auto test = read<Size, Hold>(with);
  r.z = test.bit(index) == 0;
  test.bit(index) = 1;
  prefetch();
  write<Size>(with, test);
}

auto M68000::instructionBSR(n8 displacement) -> void {
  idle(2);
  auto offset = displacement ? (s8)displacement : (s16)prefetched() - 2;
  r.pc -= 2;
  push<Long>(r.pc);
  r.pc += offset;
  prefetch();
  prefetch();
}

template<u32 Size> auto M68000::instructionBTST(DataRegister bit, EffectiveAddress with) -> void {
  auto index = read<Size>(bit) & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(2);
  }
  auto test = read<Size>(with);
  r.z = test.bit(index) == 0;
  prefetch();
}

template<u32 Size> auto M68000::instructionBTST(EffectiveAddress with) -> void {
  auto index = extension<Word>() & bits<Size>() - 1;
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(2);
  }
  auto test = read<Size>(with);
  r.z = test.bit(index) == 0;
  prefetch();
}

auto M68000::instructionCHK(DataRegister compare, EffectiveAddress maximum) -> void {
  idle(6);
  auto source = read<Word>(maximum);
  auto target = read<Word>(compare);

  r.z = clip<Word>(target) == 0;
  r.n = sign<Word>(target) < 0;
  if(r.n) return exception(Exception::BoundsCheck, Vector::BoundsCheck);

  auto result = (n64)target - source;
  r.c = sign<Word>(result >> 1) < 0;
  r.v = sign<Word>((target ^ source) & (target ^ result)) < 0;
  r.z = clip<Word>(result) == 0;
  r.n = sign<Word>(result) < 0;
  if(r.n == r.v && !r.z) return exception(Exception::BoundsCheck, Vector::BoundsCheck);
  prefetch();
}

template<u32 Size> auto M68000::instructionCLR(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect || with.mode == AddressRegisterDirect) idle(2);
  }
  read<Size, Hold>(with);
  prefetch();
  write<Size>(with, 0);
  r.c = 0;
  r.v = 0;
  r.z = 1;
  r.n = 0;
}

template<u32 Size> auto M68000::instructionCMP(EffectiveAddress from, DataRegister with) -> void {
  if constexpr(Size == Long) idle(2);
  auto source = read<Size>(from);
  auto target = read<Size>(with);
  CMP<Size>(source, target);
  prefetch();
}

template<u32 Size> auto M68000::instructionCMPA(EffectiveAddress from, AddressRegister with) -> void {
  idle(2);
  auto source = sign<Size>(read<Size>(from));
  auto target = read<Long>(with);
  CMP<Long>(source, target);
  prefetch();
}

template<u32 Size> auto M68000::instructionCMPI(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(2);
  }
  auto source = extension<Size>();
  auto target = read<Size>(with);
  CMP<Size>(source, target);
  prefetch();
}

template<u32 Size> auto M68000::instructionCMPM(EffectiveAddress from, EffectiveAddress with) -> void {
  auto source = read<Size>(from);
  auto target = read<Size>(with);
  CMP<Size>(source, target);
  prefetch();
}

auto M68000::instructionDBCC(n4 test, DataRegister with) -> void {
  auto displacement = extension<Word>();
  if(condition(test)) {
    idle(4);
  } else {
    n16 result = read<Word>(with);
    write<Word>(with, result - 1);
    if(result) {
      idle(2);
      r.pc -= 4;
      r.pc += sign<Word>(displacement);
      prefetch();
    } else {
      idle(2);
    }
  }
  prefetch();
}

auto M68000::instructionDIVS(EffectiveAddress from, DataRegister with) -> void {
  n32 dividend = read<Long>(with), odividend = dividend;
  n32 divisor  = read<Word>(from) << 16, odivisor = divisor;

  if(divisor == 0) {
    return exception(Exception::DivisionByZero, Vector::DivisionByZero);
  }

  if(divisor >> 31) {
    divisor = -divisor;
  }

  if(dividend >> 31) {
    dividend = -dividend;
    idle(2);
  }

  r.c = 0;
  if(r.v = dividend >= divisor) {
    r.z = 0;
    r.n = 1;
    idle(14);
    prefetch();
    return;
  }

  n16 quotient = 0;
  bool carry = 0;
  u32 ticks = 12;
  for(u32 index : range(15)) {
    dividend = dividend << 1;
    quotient = quotient << 1 | carry;
    if(carry = dividend >= divisor) dividend -= divisor;
    ticks += !carry ? 8 : 6;
  }
  quotient = quotient << 1 | carry;
  dividend = dividend << 1;
  if(carry = dividend >= divisor) dividend -= divisor;
  quotient = quotient << 1 | carry;
  ticks += 4;

  if(odivisor >> 31) {
    ticks += 16;
    if(odividend >> 31) {
      if(quotient >> 15) r.v = 1;
      dividend = -dividend;
    } else {
      quotient = -quotient;
      if(quotient && !(quotient >> 15)) r.v = 1;
    }
  } else if(odividend >> 31) {
    ticks += 18;
    quotient = -quotient;
    if(quotient && !(quotient >> 15)) r.v = 1;
    dividend = -dividend;
  } else {
    ticks += 14;
    if(quotient >> 15) r.v = 1;
  }

  if(r.v) {
    r.z = 0;
    r.n = 1;
    idle(ticks);
    prefetch();
    return;
  }

  r.z = quotient == 0;
  r.n = quotient < 0;

  idle(ticks);
  write<Long>(with, dividend | quotient);
  prefetch();
}

auto M68000::instructionDIVU(EffectiveAddress from, DataRegister with) -> void {
  n32 dividend = read<Long>(with);
  n32 divisor  = read<Word>(from) << 16;

  if(divisor == 0) {
    return exception(Exception::DivisionByZero, Vector::DivisionByZero);
  }

  r.c = 0;
  if(r.v = dividend >= divisor) {
    r.z = 0;
    r.n = 1;
    idle(10);
    prefetch();
    return;
  }

  n16 quotient = 0;
  bool force = 0;
  bool carry = 0;
  u32 ticks = 6;
  for(u32 index : range(16)) {
    force = dividend >> 31;
    dividend = dividend << 1;
    quotient = quotient << 1 | carry;
    if(carry = force || dividend >= divisor) dividend -= divisor;
    ticks += !carry ? 8 : !force ? 6 : 4;
  }
  ticks += force ? 6 : carry ? 4 : 2;
  quotient = quotient << 1 | carry;

  r.z = quotient == 0;
  r.n = quotient < 0;

  idle(ticks);
  write<Long>(with, dividend | quotient);
  prefetch();
}

template<u32 Size> auto M68000::instructionEOR(DataRegister from, EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = read<Size>(from);
  auto target = read<Size, Hold>(with);
  auto result = EOR<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionEORI(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = extension<Size>();
  auto target = read<Size, Hold>(with);
  auto result = EOR<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionEORI_TO_CCR() -> void {
  auto data = extension<Word>();
  writeCCR(readCCR() ^ data);
  idle(8);
  read<Word>(r.pc);
  prefetch();
}

auto M68000::instructionEORI_TO_SR() -> void {
  if(supervisor()) {
    auto data = extension<Word>();
    writeSR(readSR() ^ data);
    idle(8);
    read<Word>(r.pc);
  }
  prefetch();
}

auto M68000::instructionEXG(DataRegister x, DataRegister y) -> void {
  idle(2);
  auto z = read<Long>(x);
  write<Long>(x, read<Long>(y));
  write<Long>(y, z);
  prefetch();
}

auto M68000::instructionEXG(AddressRegister x, AddressRegister y) -> void {
  idle(2);
  auto z = read<Long>(x);
  write<Long>(x, read<Long>(y));
  write<Long>(y, z);
  prefetch();
}

auto M68000::instructionEXG(DataRegister x, AddressRegister y) -> void {
  idle(2);
  auto z = read<Long>(x);
  write<Long>(x, read<Long>(y));
  write<Long>(y, z);
  prefetch();
}

template<> auto M68000::instructionEXT<Word>(DataRegister with) -> void {
  auto result = (i8)read<Byte>(with);
  write<Word>(with, result);

  r.c = 0;
  r.v = 0;
  r.z = clip<Word>(result) == 0;
  r.n = sign<Word>(result) < 0;
  prefetch();
}

template<> auto M68000::instructionEXT<Long>(DataRegister with) -> void {
  auto result = (i16)read<Word>(with);
  write<Long>(with, result);

  r.c = 0;
  r.v = 0;
  r.z = clip<Long>(result) == 0;
  r.n = sign<Long>(result) < 0;
  prefetch();
}

auto M68000::instructionILLEGAL(n16 code) -> void {
  idle(6);
  if(code.bit(12,15) == 0xa) return exception(Exception::Illegal, Vector::IllegalLineA);
  if(code.bit(12,15) == 0xf) return exception(Exception::Illegal, Vector::IllegalLineF);
  return exception(Exception::Illegal, Vector::IllegalInstruction);
}

auto M68000::instructionJMP(EffectiveAddress from) -> void {
  r.pc = prefetched(from);
  prefetch();
  prefetch();
}

auto M68000::instructionJSR(EffectiveAddress from) -> void {
  auto ir = prefetched(from);
  auto pc = r.pc;
  r.pc = ir;
  prefetch();
  push<Long>(pc - 2);
  prefetch();
}

auto M68000::instructionLEA(EffectiveAddress from, AddressRegister to) -> void {
  if(from.mode == AddressRegisterIndirectWithIndex) idle(2);
  write<Long>(to, fetch<Long>(from));
  prefetch();
}

auto M68000::instructionLINK(AddressRegister with) -> void {
  auto displacement = (i16)extension<Word>();
  auto sp = AddressRegister{7};
  push<Long>(read<Long>(with));
  write<Long>(with, read<Long>(sp));
  write<Long>(sp, read<Long>(sp) + displacement);
  prefetch();
}

template<u32 Size> auto M68000::instructionLSL(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = LSL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionLSL(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = LSL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionLSL(EffectiveAddress with) -> void {
  auto result = LSL<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

template<u32 Size> auto M68000::instructionLSR(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = LSR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionLSR(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = LSR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionLSR(EffectiveAddress with) -> void {
  auto result = LSR<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

//todo: move memory,(xxx).l should interleave extension fetches with writes
template<u32 Size> auto M68000::instructionMOVE(EffectiveAddress from, EffectiveAddress to) -> void {
  auto data = read<Size>(from);
  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(data) == 0;
  r.n = sign<Size>(data) < 0;

  if(to.mode == AddressRegisterIndirectWithPreDecrement) {
    prefetch();
    write<Size>(to, data);
  } else {
    write<Size>(to, data);
    prefetch();
  }
}

template<u32 Size> auto M68000::instructionMOVEA(EffectiveAddress from, AddressRegister to) -> void {
  auto data = sign<Size>(read<Size>(from));
  write<Long>(to, data);
  prefetch();
}

template<u32 Size> auto M68000::instructionMOVEM_TO_MEM(EffectiveAddress to) -> void {
  auto list = extension<Word>();
  auto addr = fetch<Long>(to);

  for(u32 n : range(16)) {
    if(!list.bit(n)) continue;
    //pre-decrement mode traverses registers in reverse order {A7-A0, D7-D0}
    u32 index = to.mode == AddressRegisterIndirectWithPreDecrement ? 15 - n : n;

    if(to.mode == AddressRegisterIndirectWithPreDecrement) addr -= bytes<Size>();
    auto data = index < 8 ? read<Size>(DataRegister{index}) : read<Size>(AddressRegister{index});
    write<Size>(addr, data);
    if(to.mode != AddressRegisterIndirectWithPreDecrement) addr += bytes<Size>();
  }

  AddressRegister with{to.reg};
  if(to.mode == AddressRegisterIndirectWithPreDecrement ) write<Long>(with, addr);
  if(to.mode == AddressRegisterIndirectWithPostIncrement) write<Long>(with, addr);
  prefetch();
}

template<u32 Size> auto M68000::instructionMOVEM_TO_REG(EffectiveAddress from) -> void {
  auto list = extension<Word>();
  auto addr = fetch<Long>(from);

  for(u32 n : range(16)) {
    if(!list.bit(n)) continue;
    u32 index = from.mode == AddressRegisterIndirectWithPreDecrement ? 15 - n : n;

    if(from.mode == AddressRegisterIndirectWithPreDecrement) addr -= bytes<Size>();
    auto data = read<Size>(addr);
    data = sign<Size>(data);
    index < 8 ? write<Long>(DataRegister{index}, data) : write<Long>(AddressRegister{index}, data);
    if(from.mode != AddressRegisterIndirectWithPreDecrement) addr += bytes<Size>();
  }

  //spurious extra word read cycle exclusive to MOVEM memory->register
  if(from.mode == AddressRegisterIndirectWithPreDecrement) addr -= 2;
  read<Word>(addr);

  AddressRegister with{from.reg};
  if(from.mode == AddressRegisterIndirectWithPreDecrement ) write<Long>(with, addr);
  if(from.mode == AddressRegisterIndirectWithPostIncrement) write<Long>(with, addr);
  prefetch();
}

template<u32 Size> auto M68000::instructionMOVEP(DataRegister from, EffectiveAddress to) -> void {
  auto address = fetch<Size>(to);
  auto data = read<Long>(from);
  u32 shift = bits<Size>();
  for(auto _ : range(bytes<Size>())) {
    shift -= 8;
    write<Byte>(address, data >> shift);
    address += 2;
  }
  prefetch();
}

template<u32 Size> auto M68000::instructionMOVEP(EffectiveAddress from, DataRegister to) -> void {
  auto address = fetch<Size>(from);
  auto data = read<Long>(to);
  u32 shift = bits<Size>();
  for(auto _ : range(bytes<Size>())) {
    shift -= 8;
    data &= ~(0xff << shift);
    data |= read<Byte>(address) << shift;
    address += 2;
  }
  write<Long>(to, data);
  prefetch();
}

auto M68000::instructionMOVEQ(n8 immediate, DataRegister to) -> void {
  write<Long>(to, sign<Byte>(immediate));

  r.c = 0;
  r.v = 0;
  r.z = clip<Byte>(immediate) == 0;
  r.n = sign<Byte>(immediate) < 0;
  prefetch();
}

auto M68000::instructionMOVE_FROM_SR(EffectiveAddress to) -> void {
  if(to.mode == DataRegisterDirect) idle(2);
  if(to.mode == AddressRegisterIndirectWithPreDecrement) idle(2);
  if(to.mode != DataRegisterDirect) read<Word>(r.pc);
  auto data = readSR();
  fetch<Word>(to);
  prefetch();
  write<Word>(to, data);
}

auto M68000::instructionMOVE_TO_CCR(EffectiveAddress from) -> void {
  idle(8);
  auto data = read<Word>(from);
  writeCCR(data);
  prefetch();
}

auto M68000::instructionMOVE_TO_SR(EffectiveAddress from) -> void {
  if(supervisor()) {
    idle(8);
    auto data = read<Word>(from);
    writeSR(data);
  }
  prefetch();
}

auto M68000::instructionMOVE_FROM_USP(AddressRegister to) -> void {
  if(supervisor()) {
    write<Long>(to, r.sp);
  }
  prefetch();
}

auto M68000::instructionMOVE_TO_USP(AddressRegister from) -> void {
  if(supervisor()) {
    r.sp = read<Long>(from);
  }
  prefetch();
}

auto M68000::instructionMULS(EffectiveAddress from, DataRegister with) -> void {
  auto source = read<Word>(from);
  auto target = read<Word>(with);
  auto result = (i16)source * (i16)target;
  //+2 cycles per 0<>1 bit transition
  auto cycles = bit::count(n16(source << 1) ^ source);
  idle(34 + cycles * 2);
  write<Long>(with, result);

  r.c = 0;
  r.v = 0;
  r.z = clip<Long>(result) == 0;
  r.n = sign<Long>(result) < 0;
  prefetch();
}

auto M68000::instructionMULU(EffectiveAddress from, DataRegister with) -> void {
  auto source = read<Word>(from);
  auto target = read<Word>(with);
  auto result = source * target;
  //+2 cycles per bit set
  auto cycles = bit::count(source);
  idle(34 + cycles * 2);
  write<Long>(with, result);

  r.c = 0;
  r.v = 0;
  r.z = clip<Long>(result) == 0;
  r.n = sign<Long>(result) < 0;
  prefetch();
}

auto M68000::instructionNBCD(EffectiveAddress with) -> void {
  if(with.mode == DataRegisterDirect || with.mode == AddressRegisterDirect) idle(2);
  auto source = read<Byte, Hold>(with);
  auto target = 0u;
  auto result = target - source - r.x;
  bool c = false;
  bool v = false;

  const bool adjustLo = (target ^ source ^ result) & 0x10;
  const bool adjustHi = result & 0x100;

  if(adjustLo) {
    auto previous = result;
    result -= 0x06;
    c  = (~previous & 0x80) & ( result & 0x80);
    v |= ( previous & 0x80) & (~result & 0x80);
  }

  if(adjustHi) {
    auto previous = result;
    result -= 0x60;
    c  = true;
    v |= (previous & 0x80) & (~result & 0x80);
  }

  write<Byte>(with, result);

  r.c = c;
  r.v = v;
  r.z = clip<Byte>(result) ? 0 : r.z;
  r.n = sign<Byte>(result) < 0;
  r.x = r.c;
  prefetch();
}

template<u32 Size> auto M68000::instructionNEG(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect || with.mode == AddressRegisterDirect) idle(2);
  }
  auto result = SUB<Size>(read<Size, Hold>(with), 0);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionNEGX(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect || with.mode == AddressRegisterDirect) idle(2);
  }
  auto result = SUB<Size, Extend>(read<Size, Hold>(with), 0);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionNOP() -> void {
  prefetch();
}

template<u32 Size> auto M68000::instructionNOT(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect || with.mode == AddressRegisterDirect) idle(2);
  }
  auto result = ~read<Size, Hold>(with);
  prefetch();
  write<Size>(with, result);

  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(result) == 0;
  r.n = sign<Size>(result) < 0;
}

template<u32 Size> auto M68000::instructionOR(EffectiveAddress from, DataRegister with) -> void {
  if constexpr(Size == Long) {
    if(from.mode == DataRegisterDirect || from.mode == Immediate) {
      idle(4);
    } else {
      idle(2);
    }
  }
  auto source = read<Size>(from);
  auto target = read<Size>(with);
  auto result = OR<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionOR(DataRegister from, EffectiveAddress with) -> void {
  auto source = read<Size>(from);
  auto target = read<Size, Hold>(with);
  auto result = OR<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionORI(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = extension<Size>();
  auto target = read<Size, Hold>(with);
  auto result = OR<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionORI_TO_CCR() -> void {
  auto data = extension<Word>();
  writeCCR(readCCR() | data);
  idle(8);
  read<Word>(r.pc);
  prefetch();
}

auto M68000::instructionORI_TO_SR() -> void {
  if(supervisor()) {
    auto data = extension<Word>();
    writeSR(readSR() | data);
    idle(8);
    read<Word>(r.pc);
  }
  prefetch();
}

auto M68000::instructionPEA(EffectiveAddress from) -> void {
  if(from.mode == AddressRegisterIndirectWithIndex) idle(2);
  auto data = fetch<Long>(from);
  if(from.mode == AbsoluteShortIndirect || from.mode == AbsoluteLongIndirect) {
    push<Long>(data);
    prefetch();
  } else {
    prefetch();
    push<Long>(data);
  }
}

auto M68000::instructionRESET() -> void {
  if(supervisor()) {
    r.reset = 1;
    idle(128);
    r.reset = 0;
  }
  prefetch();
}

template<u32 Size> auto M68000::instructionROL(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionROL(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionROL(EffectiveAddress with) -> void {
  auto result = ROL<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

template<u32 Size> auto M68000::instructionROR(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionROR(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionROR(EffectiveAddress with) -> void {
  auto result = ROR<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

template<u32 Size> auto M68000::instructionROXL(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROXL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionROXL(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROXL<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionROXL(EffectiveAddress with) -> void {
  auto result = ROXL<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

template<u32 Size> auto M68000::instructionROXR(n4 count, DataRegister with) -> void {
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROXR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionROXR(DataRegister from, DataRegister with) -> void {
  auto count = read<Long>(from) & 63;
  idle((Size != Long ? 2 : 4) + count * 2);
  auto result = ROXR<Size>(read<Size>(with), count);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionROXR(EffectiveAddress with) -> void {
  auto result = ROXR<Word>(read<Word, Hold>(with), 1);
  prefetch();
  write<Word>(with, result);
}

auto M68000::instructionRTE() -> void {
  if(supervisor()) {
    auto sr = pop<Word>();
    r.pc = pop<Long>();
    writeSR(sr);
    prefetch();
  }
  prefetch();
}

auto M68000::instructionRTR() -> void {
  writeCCR(pop<Word>());
  r.pc = pop<Long>();
  prefetch();
  prefetch();
}

auto M68000::instructionRTS() -> void {
  r.pc = pop<Long>();
  prefetch();
  prefetch();
}

auto M68000::instructionSBCD(EffectiveAddress from, EffectiveAddress with) -> void {
  if(from.mode == DataRegisterDirect) idle(2);
  auto target = read<Byte, Hold, Fast>(with);
  auto source = read<Byte>(from);
  auto result = target - source - r.x;
  bool c = false;
  bool v = false;

  const bool adjustLo = (target ^ source ^ result) & 0x10;
  const bool adjustHi = result & 0x100;

  if(adjustLo) {
    auto previous = result;
    result -= 0x06;
    c  = (~previous & 0x80) & ( result & 0x80);
    v |= ( previous & 0x80) & (~result & 0x80);
  }

  if(adjustHi) {
    auto previous = result;
    result -= 0x60;
    c  = true;
    v |= (previous & 0x80) & (~result & 0x80);
  }

  prefetch();
  write<Byte>(with, result);

  r.c = c;
  r.v = v;
  r.z = clip<Byte>(result) ? 0 : r.z;
  r.n = sign<Byte>(result) < 0;
  r.x = r.c;
}

auto M68000::instructionSCC(n4 test, EffectiveAddress to) -> void {
  fetch<Byte>(to);
  prefetch();
  if(!condition(test)) {
    write<Byte>(to, 0);
  } else {
    write<Byte>(to, ~0);
    if(to.mode == DataRegisterDirect) idle(2);
  }
}

auto M68000::instructionSTOP() -> void {
  if(supervisor()) {
    auto sr = extension<Word>();
    writeSR(sr);
    r.stop = true;
  }
  prefetch();
}

template<u32 Size> auto M68000::instructionSUB(EffectiveAddress from, DataRegister with) -> void {
  if constexpr(Size == Long) {
    if(from.mode == DataRegisterDirect || from.mode == AddressRegisterDirect || from.mode == Immediate) {
      idle(4);
    } else {
      idle(2);
    }
  }
  auto source = read<Size>(from);
  auto target = read<Size>(with);
  auto result = SUB<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionSUB(DataRegister from, EffectiveAddress with) -> void {
  auto source = read<Size>(from);
  auto target = read<Size, Hold>(with);
  auto result = SUB<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionSUBA(EffectiveAddress from, AddressRegister to) -> void {
  if(Size != Long || from.mode == DataRegisterDirect || from.mode == AddressRegisterDirect || from.mode == Immediate) {
    idle(4);
  } else {
    idle(2);
  }
  auto source = sign<Size>(read<Size>(from));
  auto target = read<Long>(to);
  prefetch();
  write<Long>(to, target - source);
}

template<u32 Size> auto M68000::instructionSUBI(EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = extension<Size>();
  auto target = read<Size, Hold>(with);
  auto result = SUB<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

template<u32 Size> auto M68000::instructionSUBQ(n4 immediate, EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(with.mode == DataRegisterDirect) idle(4);
  }
  auto source = immediate;
  auto target = read<Size, Hold>(with);
  auto result = SUB<Size>(source, target);
  prefetch();
  write<Size>(with, result);
}

//Size is ignored: always uses Long
template<u32 Size> auto M68000::instructionSUBQ(n4 immediate, AddressRegister with) -> void {
  idle(4);
  auto result = read<Long>(with) - immediate;
  prefetch();
  write<Long>(with, result);
}

template<u32 Size> auto M68000::instructionSUBX(EffectiveAddress from, EffectiveAddress with) -> void {
  if constexpr(Size == Long) {
    if(from.mode == DataRegisterDirect) idle(4);
  }
  auto target = read<Size, Hold, Fast>(with);
  auto source = read<Size>(from);
  auto result = SUB<Size, Extend>(source, target);
  prefetch();
  write<Size>(with, result);
}

auto M68000::instructionSWAP(DataRegister with) -> void {
  auto result = read<Long>(with);
  result = result >> 16 | result << 16;
  write<Long>(with, result);

  r.c = 0;
  r.v = 0;
  r.z = clip<Long>(result) == 0;
  r.n = sign<Long>(result) < 0;
  prefetch();
}

auto M68000::instructionTAS(EffectiveAddress with) -> void {
  n32 data;

  if(lockable() || with.mode == DataRegisterDirect) {
    data = read<Byte, Hold>(with);
    prefetch();
    write<Byte>(with, data | 0x80);
  } else {
    //Mega Drive models 1&2 have a bug that prevents TAS write from taking effect
    //this bugged behavior is required for certain software to function correctly
    data = read<Byte>(with);
    prefetch();
    idle(6);
  }

  r.c = 0;
  r.v = 0;
  r.z = clip<Byte>(data) == 0;
  r.n = sign<Byte>(data) < 0;
}

auto M68000::instructionTRAP(n4 vector) -> void {
  idle(6);
  prefetched();
  return exception(Exception::Trap, 32 + vector, r.i);
}

auto M68000::instructionTRAPV() -> void {
  if(r.v) {
    idle(6);
    prefetched();
    return exception(Exception::Overflow, Vector::Overflow);
  }
  prefetch();
}

template<u32 Size> auto M68000::instructionTST(EffectiveAddress from) -> void {
  auto data = read<Size>(from);
  r.c = 0;
  r.v = 0;
  r.z = clip<Size>(data) == 0;
  r.n = sign<Size>(data) < 0;
  prefetch();
}

auto M68000::instructionUNLK(AddressRegister with) -> void {
  auto sp = AddressRegister{7};
  write<Long>(sp, read<Long>(with));
  write<Long>(with, pop<Long>());
  prefetch();
}
