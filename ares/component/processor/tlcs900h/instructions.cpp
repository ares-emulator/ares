template<typename Target, typename Source>
auto TLCS900H::instructionAdd(Target target, Source source) -> void {
  store(target, algorithmAdd(load(target), load(source)));
}

template<typename Target, typename Source>
auto TLCS900H::instructionAddCarry(Target target, Source source) -> void {
  store(target, algorithmAdd(load(target), load(source), CF));
}

template<typename Target, typename Source>
auto TLCS900H::instructionAnd(Target target, Source source) -> void {
  store(target, algorithmAnd(load(target), load(source)));
}

template<typename Source, typename Offset>
auto TLCS900H::instructionAndCarry(Source source, Offset offset) -> void {
  if constexpr(Source::bits == 8 && is_same_v<Offset, Register<n8>>) {
    if(load(offset).bit(3)) return (void)Undefined;
  }
  CF &= load(source).bit(load(offset) & Source::bits - 1);
}

template<typename Source, typename Offset>
auto TLCS900H::instructionBit(Source source, Offset offset) -> void {
  NF = 0;
  VF = Undefined;
  HF = 1;
  ZF = !load(source).bit(load(offset));
  SF = Undefined;
}

template<typename Source>
auto TLCS900H::instructionBitSearch1Backward(Source source) -> void {
  auto value = load(source);
  for(u32 index : reverse(range(16))) {
    if(value.bit(index)) return VF = 0, store(A, index);
  }
  VF = 1;
}

template<typename Source>
auto TLCS900H::instructionBitSearch1Forward(Source source) -> void {
  auto value = load(source);
  for(u32 index : range(16)) {
    if(value.bit(index)) return VF = 0, store(A, index);
  }
  VF = 1;
}

template<typename Source>
auto TLCS900H::instructionCall(Source source) -> void {
  auto address = load(source);
  push(PC);
  store(PC, address);
}

template<typename Source>
auto TLCS900H::instructionCallRelative(Source displacement) -> void {
  push(PC);
  store(PC, load(PC) + load(displacement));
}

template<typename Target, typename Offset>
auto TLCS900H::instructionChange(Target target, Offset offset) -> void {
  auto result = load(target);
  result.bit(load(offset) & Target::bits - 1) ^= 1;
  store(target, result);
}

template<typename Size, s32 Adjust, typename Target, typename Source>
auto TLCS900H::instructionCompare(Target target, Source source) -> void {
  auto cf = CF;  //CF is not modified; but algorithmSubtract will modify it
  algorithmSubtract(load(target), load(toMemory<Size>(load(source))));
  store(source, load(source) + Adjust);
  store(BC, load(BC) - 1);
  CF = cf;
  VF = load(BC) != 0;
}

template<typename Size, s32 Adjust, typename Target, typename Source>
auto TLCS900H::instructionCompareRepeat(Target target, Source source) -> void {
  do {
    prefetch(14);
    instructionCompare<Size, Adjust>(target, source);
  } while(load(BC) && !ZF);
}

template<typename Target, typename Source>
auto TLCS900H::instructionCompare(Target target, Source source) -> void {
  algorithmSubtract(load(target), load(source));
}

template<typename Target>
auto TLCS900H::instructionComplement(Target target) -> void {
  store(target, ~load(target));
  NF = 1;
  HF = 1;
}

template<typename Modify>
auto TLCS900H::instructionDecimalAdjustAccumulator(Modify modify) -> void {
  auto input = load(modify), value = input;
  if(CF || (n8)value > 0x99) value += NF ? -0x60 : 0x60;
  if(HF || (n4)value > 0x09) value += NF ? -0x06 : 0x06;
  if(NF == 0) CF |= value < input;
  if(NF == 1) CF |= value > input;
  VF = parity(value);
  HF = n8(value ^ input).bit(4);
  ZF = value == 0;
  SF = value.bit(-1);
  store(modify, value);
}

template<typename Target, typename Source>
auto TLCS900H::instructionDecrement(Target target, Source source) -> void {
  auto immediate = load(source);
  if(!immediate) immediate = 8;
  if constexpr(is_same_v<Target, Register<n16>> || is_same_v<Target, Register<n32>>) {
    //decw #n,r; decl #n,r: does not update flags
    store(target, load(target) - immediate);
  } else {
    //decb #n,r; decb #n,(r); decw #n,(r): does update flags
    store(target, algorithmDecrement(load(target), immediate));
  }
}

template<typename Target, typename Offset>
auto TLCS900H::instructionDecrementJumpNotZero(Target target, Offset offset) -> void {
  auto result = load(target);
  store(target, --result);
  if(!result) return;
  prefetch(2);
  store(PC, load(PC) + load(offset));
  prefetch(2);
}

template<typename Target, typename Source>
auto TLCS900H::instructionDivide(Target target, Source source) -> void {
  using T = typename Target::type;
  using E = Natural<2 * T::bits()>;
  auto dividend  = load(expand(target));
  auto divisor   = load(source);
  auto quotient  = divisor ? E(dividend / divisor) : E(T(~(dividend >> T::bits())));
  auto remainder = divisor ? E(dividend % divisor) : E(T(dividend));
  store(expand(target), T(remainder) << T::bits() | T(quotient));
  VF = !divisor || remainder >> T::bits();
}

template<typename Target, typename Source>
auto TLCS900H::instructionDivideSigned(Target target, Source source) -> void {
  using T = typename Target::type;
  using E = Natural<2 * T::bits()>;
  auto dividend  = load(expand(target)).integer();
  auto divisor   = load(source).integer();
  auto quotient  = divisor ? E(dividend / divisor) : E(T(~(dividend >> T::bits())));
  auto remainder = divisor ? E(dividend % divisor) : E(T(dividend));
  store(expand(target), T(remainder) << T::bits() | T(quotient));
  VF = !divisor || remainder >> T::bits();
}

template<typename Target, typename Source>
auto TLCS900H::instructionExchange(Target target, Source source) -> void {
  auto data = load(target);
  store(target, load(source));
  store(source, data);
}

template<typename Target>
auto TLCS900H::instructionExtendSign(Target target) -> void {
  store(target, load(shrink(target)).integer());
}

template<typename Target>
auto TLCS900H::instructionExtendZero(Target target) -> void {
  store(target, load(shrink(target)));
}

auto TLCS900H::instructionHalt() -> void {
  HALT = 1;
}

template<typename Target, typename Source>
auto TLCS900H::instructionIncrement(Target target, Source source) -> void {
  auto immediate = load(source);
  if(!immediate) immediate = 8;
  if constexpr(is_same_v<Target, Register<n16>> || is_same_v<Target, Register<n32>>) {
    //incw #n,r; incl #n,r: does not update flags
    store(target, load(target) + immediate);
  } else {
    //incb #n,r; incb #n,(r); incw #n,(r): does update flags
    store(target, algorithmIncrement(load(target), immediate));
  }
}

template<typename Source>
auto TLCS900H::instructionJump(Source source) -> void {
  auto address = load(source);
  store(PC, address);
}

template<typename Source>
auto TLCS900H::instructionJumpRelative(Source displacement) -> void {
  store(PC, load(PC) + load(displacement));
}

template<typename Target, typename Offset>
auto TLCS900H::instructionLink(Target target, Offset offset) -> void {
  push(target);
  store(target, load(XSP));
  store(XSP, load(XSP) + load(offset));
}

template<typename Target, typename Source>
auto TLCS900H::instructionLoad(Target target, Source source) -> void {
  store(target, load(source));
}

template<typename Source, typename Offset>
auto TLCS900H::instructionLoadCarry(Source source, Offset offset) -> void {
  if constexpr(Source::bits == 8 && is_same_v<Offset, Register<n8>>) {
    if(load(offset).bit(3)) return (void)Undefined;
  }
  CF = load(source).bit(load(offset) & Source::bits - 1);
}

template<typename Size, s32 Adjust, typename Target, typename Source>
auto TLCS900H::instructionLoad(Target target, Source source) -> void {
  store(toMemory<Size>(load(target)), load(toMemory<Size>(load(source))));
  store(source, load(source) + Adjust);
  store(target, load(target) + Adjust);
  store(BC, load(BC) - 1);
  NF = 0;
  VF = load(BC) != 0;
  HF = 0;
}

//note: the TLCS900/H manual states that when BC=0, an extra memory read cycle occurs and its value is discarded
//further, the instruction list indicates LDIR and LDDR take 7n+1 wait states
//however, the manual indicates it's the first address that is read twice
//it further also isn't clear whether this only occurs when *starting* LDIR and LDDR with BC=0,
//or if it always occurs at the end of ever LDIR and LDDR instruction.
//since I'm not sure how to emulate this, I don't try and guess here.
template<typename Size, s32 Adjust, typename Target, typename Source>
auto TLCS900H::instructionLoadRepeat(Target target, Source source) -> void {
  do {
    prefetch(14);
    instructionLoad<Size, Adjust>(target, source);
  } while(load(BC));
}

//reverse all bits in a 16-bit register
template<typename Modify>
auto TLCS900H::instructionMirror(Modify modify) -> void {
  auto data = load(modify);
  store(modify, bit::reverse<u16>(data));
}

template<u32 Modulo, typename Target, typename Source>
auto TLCS900H::instructionModuloDecrement(Target target, Source source) -> void {
  auto result = load(target);
  auto number = load(source);
  if((result & number) == number) {
    result += number;
  } else {
    result -= Modulo;
  }
  store(target, result);
}

template<u32 Modulo, typename Target, typename Source>
auto TLCS900H::instructionModuloIncrement(Target target, Source source) -> void {
  auto result = load(target);
  auto number = load(source);
  if((result & number) == number) {
    result -= number;
  } else {
    result += Modulo;
  }
  store(target, result);
}

template<typename Target, typename Source>
auto TLCS900H::instructionMultiply(Target target, Source source) -> void {
  store(expand(target), load(target) * load(source));
}

template<typename Modify>
auto TLCS900H::instructionMultiplyAdd(Modify modify) -> void {
  auto xde = toMemory<i16>(load(XDE));
  auto xhl = toMemory<i16>(load(XHL));

  auto source = load(expand(modify));
  auto target = load(xde) * load(xhl);
  store(expand(modify), source + target);
  store(XHL, load(XHL) - 2);

  auto result = load(expand(modify));
  VF = n32((target ^ result) & (source ^ result)).bit(-1);
  ZF = result == 0;
  SF = result.bit(-1);
}

template<typename Target, typename Source>
auto TLCS900H::instructionMultiplySigned(Target target, Source source) -> void {
  store(expand(target), load(target).integer() * load(source).integer());
}

template<typename Target>
auto TLCS900H::instructionNegate(Target target) -> void {
  store(target, algorithmSubtract(typename Target::type{0}, load(target)));
}

auto TLCS900H::instructionNoOperation() -> void {
}

template<typename Target, typename Source>
auto TLCS900H::instructionOr(Target target, Source source) -> void {
  store(target, algorithmOr(load(target), load(source)));
}

template<typename Source, typename Offset>
auto TLCS900H::instructionOrCarry(Source source, Offset offset) -> void {
  if constexpr(Source::bits == 8 && is_same_v<Offset, Register<n8>>) {
    if(load(offset).bit(3)) return (void)Undefined;
  }
  CF |= load(source).bit(load(offset) & Source::bits - 1);
}

//increments odd addresses only to ensure they are even (16-bit aligned)
template<typename Target>
auto TLCS900H::instructionPointerAdjustAccumulator(Target target) -> void {
  auto result = load(target);
  store(target, result + result.bit(0));
}

template<typename Target>
auto TLCS900H::instructionPop(Target target) -> void {
  pop(target);
}

template<typename Source>
auto TLCS900H::instructionPush(Source source) -> void {
  push(source);
}

template<typename Target, typename Offset>
auto TLCS900H::instructionReset(Target target, Offset offset) -> void {
  auto result = load(target);
  result.bit(load(offset) & Target::bits - 1) = 0;
  store(target, result);
}

auto TLCS900H::instructionReturn() -> void {
  pop(PC);
}

template<typename Source>
auto TLCS900H::instructionReturnDeallocate(Source displacement) -> void {
  pop(PC);
  store(XSP, load(XSP) + load(displacement));
}

auto TLCS900H::instructionReturnInterrupt() -> void {
  pop(SR);
  pop(PC);
  store(INTNEST, load(INTNEST) - 1);
}

template<typename LHS, typename RHS>
auto TLCS900H::instructionRotateLeftDigit(LHS lhs, RHS rhs) -> void {
  auto lvalue = load(lhs);
  auto rvalue = load(rhs);
  auto Lvalue = lvalue;
  lvalue.bit(0,3) = rvalue.bit(4,7);
  rvalue.bit(4,7) = rvalue.bit(0,3);
  rvalue.bit(0,3) = Lvalue.bit(0,3);
  store(lhs, lvalue);
  store(rhs, rvalue);
  NF = 0;
  VF = parity(lvalue);
  HF = 0;
  ZF = lvalue == 0;
  SF = lvalue.bit(-1);
}

template<typename Target, typename Amount>
auto TLCS900H::instructionRotateLeft(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    u32 cf = result.bit(-1);
    result = result << 1 | CF;
    CF = cf;
  }
  store(target, algorithmRotated(result));
}

template<typename Target, typename Amount>
auto TLCS900H::instructionRotateLeftWithoutCarry(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    CF = result.bit(-1);
    result = result << 1 | CF;
  }
  store(target, algorithmRotated(result));
}

template<typename LHS, typename RHS>
auto TLCS900H::instructionRotateRightDigit(LHS lhs, RHS rhs) -> void {
  auto lvalue = load(lhs);
  auto rvalue = load(rhs);
  auto Rvalue = rvalue;
  rvalue.bit(0,3) = rvalue.bit(4,7);
  rvalue.bit(4,7) = lvalue.bit(0,3);
  lvalue.bit(0,3) = Rvalue.bit(0,3);
  store(lhs, lvalue);
  store(rhs, rvalue);
  NF = 0;
  VF = parity(lvalue);
  HF = 0;
  ZF = lvalue == 0;
  SF = lvalue.bit(-1);
}

template<typename Target, typename Amount>
auto TLCS900H::instructionRotateRight(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    u32 cf = result.bit(0);
    result = CF << Target::bits - 1 | result >> 1;
    CF = cf;
  }
  store(target, algorithmRotated(result));
}

template<typename Target, typename Amount>
auto TLCS900H::instructionRotateRightWithoutCarry(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    CF = result.bit(0);
    result = CF << Target::bits - 1 | result >> 1;
  }
  store(target, algorithmRotated(result));
}

template<typename Target, typename Offset>
auto TLCS900H::instructionSet(Target target, Offset offset) -> void {
  auto result = load(target);
  result.bit(load(offset) & Target::bits - 1) = 1;
  store(target, result);
}

//RCF: reset carry flag
//SCF: set carry flag
auto TLCS900H::instructionSetCarryFlag(n1 value) -> void {
  CF = value;
  NF = 0;
  HF = 0;
}

//CCF: complement carry flag
//ZCF: complement zero flag to carry flag
auto TLCS900H::instructionSetCarryFlagComplement(n1 value) -> void {
  CF = !value;
  NF = 0;
  HF = Undefined;
}

template<typename Target>
auto TLCS900H::instructionSetConditionCode(n4 code, Target target) -> void {
  store(target, condition(code));
}

auto TLCS900H::instructionSetInterruptFlipFlop(n3 value) -> void {
  IFF = value;
}

auto TLCS900H::instructionSetRegisterFilePointer(n2 value) -> void {
  RFP = value;
}

template<typename Target, typename Amount>
auto TLCS900H::instructionShiftLeftArithmetic(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    CF = result.bit(-1);
    result = result << 1;
  }
  store(target, algorithmShifted(result));
}

template<typename Target, typename Amount>
auto TLCS900H::instructionShiftLeftLogical(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    CF = result.bit(-1);
    result = result << 1;
  }
  store(target, algorithmShifted(result));
}

template<typename Target, typename Amount>
auto TLCS900H::instructionShiftRightArithmetic(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    CF = result.bit(0);
    result = result >> 1;
    result.bit(-1) = result.bit(-2);
  }
  store(target, algorithmShifted(result));
}

template<typename Target, typename Amount>
auto TLCS900H::instructionShiftRightLogical(Target target, Amount amount) -> void {
  auto result = load(target);
  auto length = load(amount).clip(4);
  if(!length) length = 16;
  prefetch(length >> 2 << 1);
  for(u32 n : range(length)) {
    CF = result.bit(0);
    result = result >> 1;
  }
  store(target, algorithmShifted(result));
}

template<typename Target, typename Offset>
auto TLCS900H::instructionStoreCarry(Target target, Offset offset) -> void {
  if constexpr(Target::bits == 8) {
    if(load(offset).bit(3)) return;  //unlike other *CF instructions, STCF behavior is defined
  }
  auto result = load(target);
  result.bit(load(offset)) = CF;
  store(target, result);
}

auto TLCS900H::instructionSoftwareInterrupt(n3 vector) -> void {
  interrupt(vector << 2);
}

template<typename Target, typename Source>
auto TLCS900H::instructionSubtract(Target target, Source source) -> void {
  store(target, algorithmSubtract(load(target), load(source)));
}

//note: the TLCS900/H manual states this is subtract-with-carry, but it isn't
template<typename Target, typename Source>
auto TLCS900H::instructionSubtractBorrow(Target target, Source source) -> void {
  store(target, algorithmSubtract(load(target), load(source), CF));
}

template<typename Target, typename Offset>
auto TLCS900H::instructionTestSet(Target target, Offset offset) -> void {
  auto result = load(target);
  NF = 0;
  VF = Undefined;
  HF = 1;
  ZF = result.bit(load(offset) & Target::bits - 1);
  SF = Undefined;
  result.bit(load(offset) & Target::bits - 1) = 1;
  store(target, result);
}

template<typename Target>
auto TLCS900H::instructionUnlink(Target target) -> void {
  store(XSP, load(target));
  pop(target);
}

template<typename Target, typename Source>
auto TLCS900H::instructionXor(Target target, Source source) -> void {
  store(target, algorithmXor(load(target), load(source)));
}

template<typename Source, typename Offset>
auto TLCS900H::instructionXorCarry(Source source, Offset offset) -> void {
  if constexpr(Source::bits == 8 && is_same_v<Offset, Register<n8>>) {
    if(load(offset).bit(3)) return (void)Undefined;
  }
  CF ^= load(source).bit(load(offset) & Source::bits - 1);
}
