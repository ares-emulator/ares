template<> auto TLCS900H::toRegister3<n8>(n3 code) const -> Register<n8 > {
  static const Register<n8 > lookup[] = {W, A, B, C, D, E, H, L};
  return lookup[code];
}

template<> auto TLCS900H::toRegister3<n16>(n3 code) const -> Register<n16> {
  static const Register<n16> lookup[] = {WA, BC, DE, HL, IX, IY, IZ, SP};
  return lookup[code];
}

template<> auto TLCS900H::toRegister3<n32>(n3 code) const -> Register<n32> {
  static const Register<n32> lookup[] = {XWA, XBC, XDE, XHL, XIX, XIY, XIZ, XSP};
  return lookup[code];
}

template<typename T> auto TLCS900H::toRegister8(n8 code) const -> Register<T> {
  return {code};
}

template<typename T> auto TLCS900H::toControlRegister(n8 code) const -> ControlRegister<T> {
  return {code};
}

template<typename T> auto TLCS900H::toMemory(n32 address) const -> Memory<T> {
  return {address};
}

template<typename T> auto TLCS900H::toImmediate(n32 constant) const -> Immediate<T> {
  return {constant};
}

auto TLCS900H::undefined() -> void {
  debug(unusual, "[TLCS900H::undefined]");
  instructionSoftwareInterrupt(2);
}

auto TLCS900H::instruction() -> void {
  auto data = fetch();

  switch(OP = data) {

  case 0x00: {  //NOP
    prefetch(4);
    return instructionNoOperation();
  }
  case 0x01: {  //NORMAL (not present on 900/H)
    return undefined();
  }
  case 0x02: {  //PUSH.W SR
    prefetch(4);
    return instructionPush(SR);
  }
  case 0x03: {  //POP.W SR
    prefetch(6);
    return instructionPop(SR);
  }
  case 0x04: {  //MAX or MIN (not present on 900/H)
    return undefined();
  }
  case 0x05: {  //HALT
    prefetch(12);
    return instructionHalt();
  }
  case 0x06: {  //EI n
    data = fetch();
    prefetch(6);
    if(n3(data) == 7) prefetch(2);  //DI
    return instructionSetInterruptFlipFlop((n3)data);
  }
  case 0x07: {  //RETI
    prefetch(18);
    instructionReturnInterrupt();
    return prefetch(2);
  }
  case 0x08: {  //LD.B (n),n
    auto memory = fetchMemory<n8, n8>();
    auto immediate = fetchImmediate<n8>();
    prefetch(8);
    return instructionLoad(memory, immediate);
  }
  case 0x09: {  //PUSH.B n
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionPush(immediate);
  }
  case 0x0a: {  //LD.W (n),nn
    auto memory = fetchMemory<n16, n8>();
    auto immediate = fetchImmediate<n16>();
    prefetch(10);
    return instructionLoad(memory, immediate);
  }
  case 0x0b: {  //PUSH.W nn
    auto immediate = fetchImmediate<n16>();
    prefetch(8);
    return instructionPush(immediate);
  }
  case 0x0c: {  //INCF
    prefetch(4);
    return instructionSetRegisterFilePointer(RFP + 1);
  }
  case 0x0d: {  ///DECF
    prefetch(4);
    return instructionSetRegisterFilePointer(RFP - 1);
  }
  case 0x0e: {  //RET
    prefetch(14);
    instructionReturn();
    return prefetch(2);
  }
  case 0x0f: {  //RETD dd
    auto immediate = fetchImmediate<i16>();
    prefetch(18);
    instructionReturnDeallocate(immediate);
    return prefetch(2);
  }
  case 0x10: {  //RCF
    prefetch(4);
    return instructionSetCarryFlag(0);
  }
  case 0x11: {  //SCF
    prefetch(4);
    return instructionSetCarryFlag(1);
  }
  case 0x12: {  //CCF
    prefetch(4);
    return instructionSetCarryFlagComplement(CF);
  }
  case 0x13: {  //ZCF
    prefetch(4);
    return instructionSetCarryFlagComplement(ZF);
  }
  case 0x14: {  //PUSH.B A
    prefetch(4);
    return instructionPush(A);
  }
  case 0x15: {  //POP.B A
    prefetch(6);
    return instructionPop(A);
  }
  case 0x16: {  //EX F,F'
    prefetch(4);
    return instructionExchange(F, FP);
  }
  case 0x17: {  //LDF n
    data = fetch();
    prefetch(4);
    return instructionSetRegisterFilePointer((n2)data);
  }
  case 0x18: {  //PUSH.B F
    prefetch(4);
    return instructionPush(F);
  }
  case 0x19: {  //POP.B F
    prefetch(6);
    return instructionPop(F);
  }
  case 0x1a: {  //JP nn
    auto immediate = fetchImmediate<n16>();
    prefetch(8);
    instructionJump(immediate);
    return prefetch(2);
  }
  case 0x1b: {  //JP nnn
    auto immediate = fetchImmediate<n24>();
    prefetch(8);
    instructionJump(immediate);
    return prefetch(2);
  }
  case 0x1c: {  //CALL nn
    auto immediate = fetchImmediate<n16>();
    prefetch(14);
    instructionCall(immediate);
    return prefetch(2);
  }
  case 0x1d: {  //CALL nnn
    auto immediate = fetchImmediate<n24>();
    prefetch(16);
    instructionCall(immediate);
    return prefetch(2);
  }
  case 0x1e: {  //CALR PC+dd
    auto immediate = fetchImmediate<i16>();
    prefetch(16);
    instructionCallRelative(immediate);
    return prefetch(2);
  }
  case 0x1f: {
    return undefined();
  }
  case 0x20 ... 0x27: {  //LD R,n
    auto immediate = fetchImmediate<n8>();
    prefetch(4);
    return instructionLoad(toRegister3<n8>(data), immediate);
  }
  case 0x28 ... 0x2f: {  //PUSH.W rr
    prefetch(4);
    return instructionPush(toRegister3<n16>(data));
  }
  case 0x30 ... 0x37: {  //LD RR,nn
    auto immediate = fetchImmediate<n16>();
    prefetch(6);
    return instructionLoad(toRegister3<n16>(data), immediate);
  }
  case 0x38 ... 0x3f: {  //PUSH.L XRR
    prefetch(6);
    return instructionPush(toRegister3<n32>(data));
  }
  case 0x40 ... 0x47: {  //LD XRR,nnnn
    auto immediate = fetchImmediate<n32>();
    prefetch(10);
    return instructionLoad(toRegister3<n32>(data), immediate);
  }
  case 0x48 ... 0x4f: {  //POP.W RR
    prefetch(6);
    return instructionPop(toRegister3<n16>(data));
  }
  case 0x50 ... 0x57: {
    return undefined();
  }
  case 0x58 ... 0x5f: {  //POP.L XRR
    prefetch(8);
    return instructionPop(toRegister3<n32>(data));
  }
  case 0x60 ... 0x6f: {  //JR cc,PC+d
    auto immediate = fetchImmediate<i8>();
    prefetch(4);
    if(!condition((n4)data)) return;
    prefetch(4);
    instructionJumpRelative(immediate);
    return prefetch(2);
  }
  case 0x70 ... 0x7f: {  //JRL cc,PC+dd
    auto immediate = fetchImmediate<i16>();
    prefetch(4);
    if(!condition((n4)data)) return;
    prefetch(4);
    instructionJumpRelative(immediate);
    return prefetch(2);
  }
  case 0x80 ... 0x87: {  //src.B
    return instructionSourceMemory(toMemory<n8>(load(toRegister3<n32>(data))));
  }
  case 0x88 ... 0x8f: {  //src.B
    auto immediate = fetch<i8>();
    prefetch(2);
    return instructionSourceMemory(toMemory<n8>(load(toRegister3<n32>(data)) + immediate));
  }
  case 0x90 ... 0x97: {  //src.W
    return instructionSourceMemory(toMemory<n16>(load(toRegister3<n32>(data))));
  }
  case 0x98 ... 0x9f: {  //src.W
    auto immediate = fetch<i8>();
    prefetch(2);
    return instructionSourceMemory(toMemory<n16>(load(toRegister3<n32>(data)) + immediate));
  }
  case 0xa0 ... 0xa7: {  //src.L
    return instructionSourceMemory(toMemory<n32>(load(toRegister3<n32>(data))));
  }
  case 0xa8 ... 0xaf: {  //src.L
    auto immediate = fetch<i8>();
    prefetch(2);
    return instructionSourceMemory(toMemory<n32>(load(toRegister3<n32>(data)) + immediate));
  }
  case 0xb0 ... 0xb7: {  //dst
    return instructionTargetMemory(load(toRegister3<n32>(data)));
  }
  case 0xb8 ... 0xbf: {  //dst
    auto immediate = fetch<i8>();
    prefetch(2);
    return instructionTargetMemory(load(toRegister3<n32>(data)) + immediate);
  }
  case 0xc0: {  //src.B
    auto memory = fetchMemory<n8, n8>();
    prefetch(2);
    return instructionSourceMemory(memory);
  }
  case 0xc1: {  //src.B
    auto memory = fetchMemory<n8, n16>();
    prefetch(4);
    return instructionSourceMemory(memory);
  }
  case 0xc2: {  //src.B
    auto memory = fetchMemory<n8, n24>();
    prefetch(6);
    return instructionSourceMemory(memory);
  }
  case 0xc3: {  //src.B
    data = fetch();
    if((data & 3) == 0) {
      auto register = load(toRegister8<n32>(data));
      prefetch(2);
      return instructionSourceMemory(toMemory<n8>(register));
    }
    if((data & 3) == 1) {
      auto register = load(toRegister8<n32>(data));
      auto displacement = fetch<i16>();
      prefetch(6);
      return instructionSourceMemory(toMemory<n8>(register + displacement));
    }
    if(data == 0x03) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n8>());
      prefetch(6);
      return instructionSourceMemory(toMemory<n8>(register + (i8)displacement));
    }
    if(data == 0x07) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n16>());
      prefetch(6);
      return instructionSourceMemory(toMemory<n8>(register + (i16)displacement));
    }
    debug(unusual, "[TLCS900H::instruction] 0xc3 0x", hex(data, 2L));
    return undefined();
  }
  case 0xc4: {  //src.B
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionSourceMemory(toMemory<n8>(location));
  }
  case 0xc5: {  //src.B
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionSourceMemory(toMemory<n8>(location));
  }
  case 0xc6: {
    return undefined();
  }
  case 0xc7: {  //reg.B
    auto register = fetchRegister<n8>();
    prefetch(2);
    return instructionRegister(register);
  }
  case 0xc8 ... 0xcf: {  //reg.B
    return instructionRegister(toRegister3<n8>(data));
  }
  case 0xd0: {  //src.W
    auto memory = fetchMemory<n16, n8>();
    prefetch(2);
    return instructionSourceMemory(memory);
  }
  case 0xd1: {  //src.W
    auto memory = fetchMemory<n16, n16>();
    prefetch(4);
    return instructionSourceMemory(memory);
  }
  case 0xd2: {  //src.W
    auto memory = fetchMemory<n16, n24>();
    prefetch(6);
    return instructionSourceMemory(memory);
  }
  case 0xd3: {  //src.W
    data = fetch();
    if((data & 3) == 0) {
      auto register = load(toRegister8<n32>(data));
      prefetch(2);
      return instructionSourceMemory(toMemory<n16>(register));
    }
    if((data & 3) == 1) {
      auto register = load(toRegister8<n32>(data));
      auto displacement = fetch<i16>();
      prefetch(6);
      return instructionSourceMemory(toMemory<n16>(register + displacement));
    }
    if(data == 0x03) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n8>());
      prefetch(6);
      return instructionSourceMemory(toMemory<n16>(register + (i8)displacement));
    }
    if(data == 0x07) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n16>());
      prefetch(6);
      return instructionSourceMemory(toMemory<n16>(register + (i16)displacement));
    }
    debug(unusual, "[TLCS900H::instruction] 0xd3 0x", hex(data, 2L));
    return undefined();
  }
  case 0xd4: {  //src.W
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionSourceMemory(toMemory<n16>(location));
  }
  case 0xd5: {  //src.W
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionSourceMemory(toMemory<n16>(location));
  }
  case 0xd6: {
    return undefined();
  }
  case 0xd7: {  //reg.W
    auto register = fetchRegister<n16>();
    prefetch(2);
    return instructionRegister(register);
  }
  case 0xd8 ... 0xdf: {  //reg.W
    return instructionRegister(toRegister3<n16>(data));
  }
  case 0xe0: {  //src.L
    auto memory = fetchMemory<n32, n8>();
    prefetch(2);
    return instructionSourceMemory(memory);
  }
  case 0xe1: {  //src.L
    auto memory = fetchMemory<n32, n16>();
    prefetch(4);
    return instructionSourceMemory(memory);
  }
  case 0xe2: {  //src.L
    auto memory = fetchMemory<n32, n24>();
    prefetch(6);
    return instructionSourceMemory(memory);
  }
  case 0xe3: {  //src.L
    data = fetch();
    if((data & 3) == 0) {
      auto register = load(toRegister8<n32>(data));
      prefetch(2);
      return instructionSourceMemory(toMemory<n32>(register));
    }
    if((data & 3) == 1) {
      auto register = load(toRegister8<n32>(data));
      auto displacement = fetch<i16>();
      prefetch(6);
      return instructionSourceMemory(toMemory<n32>(register + displacement));
    }
    if(data == 0x03) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n8>());
      prefetch(6);
      return instructionSourceMemory(toMemory<n32>(register + (i8)displacement));
    }
    if(data == 0x07) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n16>());
      prefetch(6);
      return instructionSourceMemory(toMemory<n32>(register + (i16)displacement));
    }
    debug(unusual, "[TLCS900H::instruction] 0xe3 0x", hex(data, 2L));
    return undefined();
  }
  case 0xe4: {  //src.L
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionSourceMemory(toMemory<n32>(location));
  }
  case 0xe5: {  //src.L
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionSourceMemory(toMemory<n32>(location));
  }
  case 0xe6: {
    return undefined();
  }
  case 0xe7: {  //reg.L
    auto register = fetchRegister<n32>();
    prefetch(2);
    return instructionRegister(register);
  }
  case 0xe8 ... 0xef: {  //reg.L
    return instructionRegister(toRegister3<n32>(data));
  }
  case 0xf0: {  //dst
    auto memory = fetch<n8>();
    prefetch(2);
    return instructionTargetMemory(memory);
  }
  case 0xf1: {  //dst
    auto memory = fetch<n16>();
    prefetch(4);
    return instructionTargetMemory(memory);
  }
  case 0xf2: {  //dst
    auto memory = fetch<n24>();
    prefetch(6);
    return instructionTargetMemory(memory);
  }
  case 0xf3: {  //dst
    data = fetch();
    if((data & 3) == 0) {
      auto register = load(toRegister8<n32>(data));
      prefetch(2);
      return instructionTargetMemory(register);
    }
    if((data & 3) == 1) {
      auto register = load(toRegister8<n32>(data));
      auto displacement = fetch<i16>();
      prefetch(6);
      return instructionTargetMemory(register + displacement);
    }
    if(data == 0x03) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n8>());
      prefetch(6);
      return instructionTargetMemory(register + (i8)displacement);
    }
    if(data == 0x07) {
      auto register = load(fetchRegister<n32>());
      auto displacement = load(fetchRegister<n16>());
      prefetch(6);
      return instructionTargetMemory(register + (i16)displacement);
    }
    if(data == 0x13) {  //LDAR R,PC+dd
      auto immediate = fetch<i16>();
      auto address = load(PC) + immediate;  //load PC before final byte fetch
      data = fetch();
      switch(data) {
      case 0x20 ... 0x27:
        prefetch(14);
        return instructionLoad(toRegister3<n16>(data), toImmediate<n16>(address));
      case 0x30 ... 0x37:
        prefetch(14);
        return instructionLoad(toRegister3<n32>(data), toImmediate<n32>(address));
      }
    }
    debug(unusual, "[TLCS900H::instruction] 0xf3 0x", hex(data, 2L));
    return undefined();
  }
  case 0xf4: {  //dst
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionTargetMemory(location);
  }
  case 0xf5: {  //dst
    data = fetch();
    prefetch(2);
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) (void)Undefined;
    return instructionTargetMemory(location);
  }
  case 0xf6: {
    return undefined();
  }
  case 0xf7: {  //LDX.B (n),n
    fetch();  //0x00
    auto memory = fetchMemory<n8, n8>();
    fetch();  //0x00
    auto immediate = fetchImmediate<n8>();
    fetch();  //0x00
    prefetch(14);
    return instructionLoad(memory, immediate);
  }
  case 0xf8 ... 0xff: {  //SWI n
    return instructionSoftwareInterrupt((n3)data);
  }

  }
}

template<typename R>
auto TLCS900H::instructionRegister(R register) -> void {
  using T = typename R::type;
  enum : bool { Byte = R::bits == 8, Word = R::bits == 16, Long = R::bits == 32 };
  auto data = fetch();

  switch(data) {

  case 0x00 ... 0x02: {
    return undefined();
  }
  case 0x03: {  //LD r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionLoad(register, immediate);
  }
  case 0x04: {  //PUSH r
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionPush(register);
  }
  case 0x05: {  //POP r
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionPop(register);
  }
  case 0x06: {  //CPL r
    if constexpr(!Long) {
      prefetch(4);
      return instructionComplement(register);
    }
    return undefined();
  }
  case 0x07: {  //NEG r
    if constexpr(!Long) {
      prefetch(4);
      return instructionNegate(register);
    }
    return undefined();
  }
  case 0x08: {  //MUL rr,#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(24);
      if constexpr(Word) prefetch(30);
      return instructionMultiply(register, immediate);
    }
    return undefined();
  }
  case 0x09: {  //MULS rr,#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(20);
      if constexpr(Word) prefetch(26);
      return instructionMultiplySigned(register, immediate);
    }
    return undefined();
  }
  case 0x0a: {  //DIV rr,#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(30);
      if constexpr(Word) prefetch(46);
      return instructionDivide(register, immediate);
    }
    return undefined();
  }
  case 0x0b: {  //DIVS rr,#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(36);
      if constexpr(Word) prefetch(52);
      return instructionDivideSigned(register, immediate);
    }
    return undefined();
  }
  case 0x0c: {  //LINK r,dd
    if constexpr(Long) {
      auto immediate = fetchImmediate<i16>();
      prefetch(12);
      return instructionLink(register, immediate);
    }
    return undefined();
  }
  case 0x0d: {  //UNLK r
    if constexpr(Long) {
      prefetch(10);
      return instructionUnlink(register);
    }
    return undefined();
  }
  case 0x0e: {  //BS1F A,r
    if constexpr(Word) {
      prefetch(6);
      return instructionBitSearch1Forward(register);
    }
    return undefined();
  }
  case 0x0f: {  //BS1B A,r
    if constexpr(Word) {
      prefetch(6);
      return instructionBitSearch1Backward(register);
    }
    return undefined();
  }
  case 0x10: {  //DAA r
    if constexpr(Byte) {
      prefetch(8);
      return instructionDecimalAdjustAccumulator(register);
    }
    return undefined();
  }
  case 0x11: {
    return undefined();
  }
  case 0x12: {  //EXTZ r
    if constexpr(!Byte) {
      prefetch(6);
      return instructionExtendZero(register);
    }
    return undefined();
  }
  case 0x13: {  //EXTS r
    if constexpr(!Byte) {
      prefetch(6);
      return instructionExtendSign(register);
    }
    return undefined();
  }
  case 0x14: {  //PAA r
    if constexpr(!Byte) {
      prefetch(8);
      return instructionPointerAdjustAccumulator(register);
    }
    return undefined();
  }
  case 0x15: {
    return undefined();
  }
  case 0x16: {  //MIRR r
    if constexpr(Word) {
      prefetch(6);
      return instructionMirror(register);
    }
    return undefined();
  }
  case 0x17 ... 0x18: {
    return undefined();
  }
  case 0x19: {  //MULA r
    if constexpr(Word) {
      prefetch(38);
      return instructionMultiplyAdd(register);
    }
    return undefined();
  }
  case 0x1a ... 0x1b: {
    return undefined();
  }
  case 0x1c: {  //DJNZ r,d
    if constexpr(!Long) {
      auto immediate = fetchImmediate<i8>();
      prefetch(8);
      return instructionDecrementJumpNotZero(register, immediate);
    }
    return undefined();
  }
  case 0x1d ... 0x1f: {
    return undefined();
  }
  case 0x20: {  //ANDCF #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionAndCarry(register, immediate);
    }
    return undefined();
  }
  case 0x21: {  //ORCF #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionOrCarry(register, immediate);
    }
    return undefined();
  }
  case 0x22: {  //XORCF #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionXorCarry(register, immediate);
    }
    return undefined();
  }
  case 0x23: {  //LDCF #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionLoadCarry(register, immediate);
    }
    return undefined();
  }
  case 0x24: {  //STCF #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionStoreCarry(register, immediate);
    }
    return undefined();
  }
  case 0x25 ... 0x27: {
    return undefined();
  }
  case 0x28: {  //ANDCF A,r
    if constexpr(!Long) {
      prefetch(6);
      return instructionAndCarry(register, A);
    }
    return undefined();
  }
  case 0x29: {  //ORCF A,r
    if constexpr(!Long) {
      prefetch(6);
      return instructionOrCarry(register, A);
    }
    return undefined();
  }
  case 0x2a: {  //XORCF A,r
    if constexpr(!Long) {
      prefetch(6);
      return instructionXorCarry(register, A);
    }
    return undefined();
  }
  case 0x2b: {  //LDCF A,r
    if constexpr(!Long) {
      prefetch(6);
      return instructionLoadCarry(register, A);
    }
    return undefined();
  }
  case 0x2c: {  //STCF A,r
    if constexpr(!Long) {
      prefetch(6);
      return instructionStoreCarry(register, A);
    }
    return undefined();
  }
  case 0x2d: {
    return undefined();
  }
  case 0x2e: {  //LDC cr,r
    data = fetch();
    prefetch(6);
    return instructionLoad(toControlRegister<T>(data), register);
  }
  case 0x2f: {  //LDC r,cr
    data = fetch();
    prefetch(6);
    return instructionLoad(register, toControlRegister<T>(data));
  }
  case 0x30: {  //RES #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionReset(register, immediate);
    }
    return undefined();
  }
  case 0x31: {  //SET #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionSet(register, immediate);
    }
    return undefined();
  }
  case 0x32: {  //CHG #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(6);
      return instructionChange(register, immediate);
    }
    return undefined();
  }
  case 0x33: {  //BIT #,r
    if constexpr(!Long) {
      data = fetch();
      prefetch(6);
      return instructionBit(register, toImmediate<n4>(data));
    }
    return undefined();
  }
  case 0x34: {  //TSET #,r
    if constexpr(!Long) {
      auto immediate = fetchImmediate<n8>();
      prefetch(8);
      return instructionTestSet(register, immediate);
    }
    return undefined();
  }
  case 0x35 ... 0x37: {
    return undefined();
  }
  case 0x38: {  //MINC1 #,r
    if constexpr(Word) {
      auto immediate = fetchImmediate<n16>();
      prefetch(10);
      return instructionModuloIncrement<1>(register, immediate);
    }
    return undefined();
  }
  case 0x39: {  //MINC2 #,r
    if constexpr(Word) {
      auto immediate = fetchImmediate<n16>();
      prefetch(10);
      return instructionModuloIncrement<2>(register, immediate);
    }
    return undefined();
  }
  case 0x3a: {  //MINC4 #,r
    if constexpr(Word) {
      auto immediate = fetchImmediate<n16>();
      prefetch(10);
      return instructionModuloIncrement<4>(register, immediate);
    }
    return undefined();
  }
  case 0x3b: {
    return undefined();
  }
  case 0x3c: {  //MDEC1 #,r
    if constexpr(Word) {
      auto immediate = fetchImmediate<n16>();
      prefetch(8);
      return instructionModuloDecrement<1>(register, immediate);
    }
    return undefined();
  }
  case 0x3d: {  //MDEC2 #,r
    if constexpr(Word) {
      auto immediate = fetchImmediate<n16>();
      prefetch(8);
      return instructionModuloDecrement<2>(register, immediate);
    }
    return undefined();
  }
  case 0x3e: {  //MDEC4 #,r
    if constexpr(Word) {
      auto immediate = fetchImmediate<n16>();
      prefetch(8);
      return instructionModuloDecrement<4>(register, immediate);
    }
    return undefined();
  }
  case 0x3f: {
    return undefined();
  }
  case 0x40 ... 0x47: {  //MUL R,r
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(22);
      if constexpr(Word) prefetch(28);
      return instructionMultiply(toRegister3<T>(data), register);
    }
    return undefined();
  }
  case 0x48 ... 0x4f: {  //MULS R,r
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(18);
      if constexpr(Word) prefetch(24);
      return instructionMultiplySigned(toRegister3<T>(data), register);
    }
    return undefined();
  }
  case 0x50 ... 0x57: {  //DIV R,r
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(30);
      if constexpr(Word) prefetch(46);
      return instructionDivide(toRegister3<T>(data), register);
    }
    return undefined();
  }
  case 0x58 ... 0x5f: {  //DIVS R,r
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(36);
      if constexpr(Word) prefetch(52);
      return instructionDivideSigned(toRegister3<T>(data), register);
    }
    return undefined();
  }
  case 0x60 ... 0x67: {  //INC #3,r
    prefetch(4);
    return instructionIncrement(register, toImmediate<T>((n3)data));
  }
  case 0x68 ... 0x6f: {  //DEC #3,r
    prefetch(4);
    return instructionDecrement(register, toImmediate<T>((n3)data));
  }
  case 0x70 ... 0x7f: {  //SCC cc,r
    if constexpr(!Long) {
      prefetch(4);
      return instructionSetConditionCode((n4)data, register);
    }
    return undefined();
  }
  case 0x80 ... 0x87: {  //ADD R,r
    prefetch(4);
    return instructionAdd(toRegister3<T>(data), register);
  }
  case 0x88 ... 0x8f: {  //LD R,r
    prefetch(4);
    return instructionLoad(toRegister3<T>(data), register);
  }
  case 0x90 ... 0x97: {  //ADC R,r
    prefetch(4);
    return instructionAddCarry(toRegister3<T>(data), register);
  }
  case 0x98 ... 0x9f: {  //LD r,R
    prefetch(4);
    return instructionLoad(register, toRegister3<T>(data));
  }
  case 0xa0 ... 0xa7: {  //SUB R,r
    prefetch(4);
    return instructionSubtract(toRegister3<T>(data), register);
  }
  case 0xa8 ... 0xaf: {  //LD r,#3
    prefetch(4);
    return instructionLoad(register, toImmediate<T>((n3)data));
  }
  case 0xb0 ... 0xb7: {  //SBC R,r
    prefetch(4);
    return instructionSubtractBorrow(toRegister3<T>(data), register);
  }
  case 0xb8 ... 0xbf: {  //EX R,r
    if constexpr(!Long) {
      prefetch(6);
      return instructionExchange(toRegister3<T>(data), register);
    }
    return undefined();
  }
  case 0xc0 ... 0xc7: {  //AND R,r
    prefetch(4);
    return instructionAnd(toRegister3<T>(data), register);
  }
  case 0xc8: {  //ADD r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionAdd(register, immediate);
  }
  case 0xc9: {  //ADC r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionAddCarry(register, immediate);
  }
  case 0xca: {  //SUB r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionSubtract(register, immediate);
  }
  case 0xcb: {  //SBC r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionSubtractBorrow(register, immediate);
  }
  case 0xcc: {  //AND r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionAnd(register, immediate);
  }
  case 0xcd: {  //XOR r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionXor(register, immediate);
  }
  case 0xce: {  //OR r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionOr(register, immediate);
  }
  case 0xcf: {  //CP r,#
    auto immediate = fetchImmediate<T>();
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionCompare(register, immediate);
  }
  case 0xd0 ... 0xd7: {  //XOR R,r
    prefetch(4);
    return instructionXor(toRegister3<T>(data), register);
  }
  case 0xd8 ... 0xdf: {  //CP r,#3
    if constexpr(!Long) {
      prefetch(4);
      return instructionCompare(register, toImmediate<T>((n3)data));
    }
    return undefined();
  }
  case 0xe0 ... 0xe7: {  //OR R,r
    prefetch(4);
    return instructionOr(toRegister3<T>(data), register);
  }
  case 0xe8: {  //RLC #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionRotateLeftWithoutCarry(register, immediate);
  }
  case 0xe9: {  //RRC #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionRotateRightWithoutCarry(register, immediate);
  }
  case 0xea: {  //RL #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionRotateLeft(register, immediate);
  }
  case 0xeb: {  //RR #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionRotateRight(register, immediate);
  }
  case 0xec: {  //SLA #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionShiftLeftArithmetic(register, immediate);
  }
  case 0xed: {  //SRA #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionShiftRightArithmetic(register, immediate);
  }
  case 0xee: {  //SLL #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionShiftLeftLogical(register, immediate);
  }
  case 0xef: {  //SRL #,r
    auto immediate = fetchImmediate<n8>();
    prefetch(6);
    return instructionShiftRightLogical(register, immediate);
  }
  case 0xf0 ... 0xf7: {  //CP R,r
    prefetch(4);
    return instructionCompare(toRegister3<T>(data), register);
  }
  case 0xf8: {  //RLC A,r
    prefetch(6);
    return instructionRotateLeftWithoutCarry(register, A);
  }
  case 0xf9: {  //RRC A,r
    prefetch(6);
    return instructionRotateRightWithoutCarry(register, A);
  }
  case 0xfa: {  //RL A,r
    prefetch(6);
    return instructionRotateLeft(register, A);
  }
  case 0xfb: {  //RR A,r
    prefetch(6);
    return instructionRotateRight(register, A);
  }
  case 0xfc: {  //SLA A,r
    prefetch(6);
    return instructionShiftLeftArithmetic(register, A);
  }
  case 0xfd: {  //SRA A,r
    prefetch(6);
    return instructionShiftRightArithmetic(register, A);
  }
  case 0xfe: {  //SLL A,r
    prefetch(6);
    return instructionShiftLeftLogical(register, A);
  }
  case 0xff: {  //SRL A,r
    prefetch(6);
    return instructionShiftRightLogical(register, A);
  }

  }
}

template<typename M>
auto TLCS900H::instructionSourceMemory(M memory) -> void {
  using T = typename M::type;
  enum : bool { Byte = M::bits == 8, Word = M::bits == 16, Long = M::bits == 32 };
  auto data = fetch();

  switch(data) {

  case 0x00 ... 0x03: {
    return undefined();
  }
  case 0x04: {  //PUSH (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionPush(memory);
    }
    return undefined();
  }
  case 0x05: {
    return undefined();
  }
  case 0x06: {  //RLD A,(mem)
    if constexpr(Byte) {
      prefetch(24);
      return instructionRotateLeftDigit(A, memory);
    }
    return undefined();
  }
  case 0x07: {  //RRD A,(mem)
    if constexpr(Byte) {
      prefetch(24);
      return instructionRotateRightDigit(A, memory);
    }
    return undefined();
  }
  case 0x08 ... 0x0f: {
    return undefined();
  }
  case 0x10: {  //LDI
    auto target = n3(OP) == 5 ? XIX : XDE;
    auto source = n3(OP) == 5 ? XIY : XHL;
    if constexpr(Byte) { prefetch(12); return instructionLoad<T, +1>(target, source); }
    if constexpr(Word) { prefetch(12); return instructionLoad<T, +2>(target, source); }
    return undefined();
  }
  case 0x11: {  //LDIR
    auto target = n3(OP) == 5 ? XIX : XDE;
    auto source = n3(OP) == 5 ? XIY : XHL;
    if constexpr(Byte) { prefetch(2); return instructionLoadRepeat<T, +1>(target, source); }
    if constexpr(Word) { prefetch(2); return instructionLoadRepeat<T, +2>(target, source); }
    return undefined();
  }
  case 0x12: {  //LDD
    auto target = n3(OP) == 5 ? XIX : XDE;
    auto source = n3(OP) == 5 ? XIY : XHL;
    if constexpr(Byte) { prefetch(12); return instructionLoad<T, -1>(target, source); }
    if constexpr(Word) { prefetch(12); return instructionLoad<T, -2>(target, source); }
    return undefined();
  }
  case 0x13: {  //LDDR
    auto target = n3(OP) == 5 ? XIX : XDE;
    auto source = n3(OP) == 5 ? XIY : XHL;
    if constexpr(Byte) { prefetch(2); return instructionLoadRepeat<T, -1>(target, source); }
    if constexpr(Word) { prefetch(2); return instructionLoadRepeat<T, -2>(target, source); }
    return undefined();
  }
  case 0x14: {  //CPI
    auto register = toRegister3<n32>(OP);
    if constexpr(Byte) { prefetch(10); return instructionCompare<T, +1>( A, register); }
    if constexpr(Word) { prefetch(10); return instructionCompare<T, +2>(WA, register); }
    return undefined();
  }
  case 0x15: {  //CPIR
    auto register = toRegister3<n32>(OP);
    if constexpr(Byte) { prefetch(2); return instructionCompareRepeat<T, +1>( A, register); }
    if constexpr(Word) { prefetch(2); return instructionCompareRepeat<T, +2>(WA, register); }
    return undefined();
  }
  case 0x16: {  //CPD
    if constexpr(!Long) {
      auto register = toRegister3<n32>(OP);
      if constexpr(Byte) { prefetch(10); return instructionCompare<T, -1>( A, register); }
      if constexpr(Word) { prefetch(10); return instructionCompare<T, -2>(WA, register); }
      return undefined();
    }
    return undefined();
  }
  case 0x17: {  //CPDR
    if constexpr(!Long) {
      prefetch(2);
      auto register = toRegister3<n32>(OP);
      if constexpr(Byte) { prefetch(2); return instructionCompareRepeat<T, -1>( A, register); }
      if constexpr(Word) { prefetch(2); return instructionCompareRepeat<T, -2>(WA, register); }
      return undefined();
    }
    return undefined();
  }
  case 0x18: {
    return undefined();
  }
  case 0x19: {  //LD (nn),(m)
    if constexpr(!Long) {
      auto target = fetchMemory<T, n16>();
      prefetch(12);
      return instructionLoad(target, memory);
    }
    return undefined();
  }
  case 0x1a ... 0x1f: {
    return undefined();
  }
  case 0x20 ... 0x27: {  //LD R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionLoad(toRegister3<T>(data), memory);
  }
  case 0x28 ... 0x2f: {
    return undefined();
  }
  case 0x30 ... 0x37: {  //EX (mem),R
    if constexpr(!Long) {
      prefetch(8);
      return instructionExchange(memory, toRegister3<T>(data));
    }
    return undefined();
  }
  case 0x38: {  //ADD (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionAdd(memory, immediate);
    }
    return undefined();
  }
  case 0x39: {  //ADC (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionAddCarry(memory, immediate);
    }
    return undefined();
  }
  case 0x3a: {  //SUB (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionSubtract(memory, immediate);
    }
    return undefined();
  }
  case 0x3b: {  //SBC (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionSubtractBorrow(memory, immediate);
    }
    return undefined();
  }
  case 0x3c: {  //AND (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionAnd(memory, immediate);
    }
    return undefined();
  }
  case 0x3d: {//XOR (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionXor(memory, immediate); }
    }
    return undefined();
  case 0x3e: {  //OR (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(10);
      if constexpr(Word) prefetch(12);
      return instructionOr(memory, immediate);
    }
    return undefined();
  }
  case 0x3f: {  //CP (mem),#
    if constexpr(!Long) {
      auto immediate = fetchImmediate<T>();
      if constexpr(Byte) prefetch(8);
      if constexpr(Word) prefetch(10);
      return instructionCompare(memory, immediate);
    }
    return undefined();
  }
  case 0x40 ... 0x47: {  //MUL R,(mem)
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(24);
      if constexpr(Word) prefetch(30);
      return instructionMultiply(toRegister3<T>(data), memory);
    }
    return undefined();
  }
  case 0x48 ... 0x4f: {  //MULS R,(mem)
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(20);
      if constexpr(Word) prefetch(26);
      return instructionMultiplySigned(toRegister3<T>(data), memory);
    }
    return undefined();
  }
  case 0x50 ... 0x57: {  //DIV R,(mem)
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(30);
      if constexpr(Word) prefetch(46);
      return instructionDivide(toRegister3<T>(data), memory);
    }
    return undefined();
  }
  case 0x58 ... 0x5f: {  //DIVS R,(mem)
    if constexpr(!Long) {
      if constexpr(Byte) prefetch(36);
      if constexpr(Word) prefetch(52);
      return instructionDivideSigned(toRegister3<T>(data), memory);
    }
    return undefined();
  }
  case 0x60 ... 0x67: {  //INC #3,(mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionIncrement(memory, toImmediate<T>((n3)data));
    }
    return undefined();
  }
  case 0x68 ... 0x6f: {  //DEC #3,(mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionDecrement(memory, toImmediate<T>((n3)data));
    }
    return undefined();
  }
  case 0x70 ... 0x77: {
    return undefined();
  }
  case 0x78: {  //RLC (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionRotateLeftWithoutCarry(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x79: {  //RRC (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionRotateRightWithoutCarry(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x7a: {  //RL (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionRotateLeft(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x7b: {  //RR (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionRotateRight(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x7c: {  //SLA (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionShiftLeftArithmetic(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x7d: {  //SRA (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionShiftRightArithmetic(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x7e: {  //SLL (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionShiftLeftLogical(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x7f: {  //SRL (mem)
    if constexpr(!Long) {
      prefetch(8);
      return instructionShiftRightLogical(memory, toImmediate<n4>(1));
    }
    return undefined();
  }
  case 0x80 ... 0x87: {  //ADD R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionAdd(toRegister3<T>(data), memory);
  }
  case 0x88 ... 0x8f: {  //ADD (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionAdd(memory, toRegister3<T>(data));
  }
  case 0x90 ... 0x97: {  //ADC R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionAddCarry(toRegister3<T>(data), memory);
  }
  case 0x98 ... 0x9f: {  //ADC (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionAddCarry(memory, toRegister3<T>(data));
  }
  case 0xa0 ... 0xa7: {  //SUB R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionSubtract(toRegister3<T>(data), memory);
  }
  case 0xa8 ... 0xaf: {  //SUB (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionSubtract(memory, toRegister3<T>(data));
  }
  case 0xb0 ... 0xb7: {  //SBC R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionSubtractBorrow(toRegister3<T>(data), memory);
  }
  case 0xb8 ... 0xbf: {  //SBC (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionSubtractBorrow(memory, toRegister3<T>(data));
  }
  case 0xc0 ... 0xc7: {  //AND R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionAnd(toRegister3<T>(data), memory);
  }
  case 0xc8 ... 0xcf: {  //AND (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionAnd(memory, toRegister3<T>(data));
  }
  case 0xd0 ... 0xd7: {  //XOR R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionXor(toRegister3<T>(data), memory);
  }
  case 0xd8 ... 0xdf: {  //XOR (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionXor(memory, toRegister3<T>(data));
  }
  case 0xe0 ... 0xe7: {  //OR R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionOr(toRegister3<T>(data), memory);
  }
  case 0xe8 ... 0xef: {  //OR (mem),R
    if constexpr(Byte) prefetch(8);
    if constexpr(Word) prefetch(8);
    if constexpr(Long) prefetch(12);
    return instructionOr(memory, toRegister3<T>(data));
  }
  case 0xf0 ... 0xf7: {  //CP R,(mem)
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionCompare(toRegister3<T>(data), memory);
  }
  case 0xf8 ... 0xff: {  //CP (mem),R
    if constexpr(Byte) prefetch(6);
    if constexpr(Word) prefetch(6);
    if constexpr(Long) prefetch(8);
    return instructionCompare(memory, toRegister3<T>(data));
  }

  }
}

auto TLCS900H::instructionTargetMemory(n32 address) -> void {
  auto data = fetch();

  switch(data) {

  case 0x00: {  //LD.B (m),#
    auto immediate = fetchImmediate<n8>();
    prefetch(8);
    return instructionLoad(toMemory<n8>(address), immediate);
  }
  case 0x01: {
    return undefined();
  }
  case 0x02: {  //LD.W (m),#
    auto immediate = fetchImmediate<n16>();
    prefetch(10);
    return instructionLoad(toMemory<n16>(address), immediate);
  }
  case 0x03: {
    return undefined();
  }
  case 0x04: {  //POP.B (mem)
    prefetch(10);
    return instructionPop(toMemory<n8>(address));
  }
  case 0x05: {
    return undefined();
  }
  case 0x06: {  //POP.W (mem)
    prefetch(10);
    return instructionPop(toMemory<n16>(address));
  }
  case 0x07 ... 0x13: {
    return undefined();
  }
  case 0x14: {  //LD.B (m),(nn)
    auto source = fetchMemory<n8, n16>();
    prefetch(12);
    return instructionLoad(toMemory<n8>(address), source);
  }
  case 0x15: {
    return undefined();
  }
  case 0x16: {  //LD.W (m),(nn)
    auto source = fetchMemory<n16, n16>();
    prefetch(12);
    return instructionLoad(toMemory<n16>(address), source);
  }
  case 0x17 ... 0x1f: {
    return undefined();
  }
  case 0x20 ... 0x27: {  //LDA.W R,mem
    prefetch(8);
    return instructionLoad(toRegister3<n16>(data), toImmediate<n16>(address));
  }
  case 0x28: {  //ANDCF A,(mem)
    prefetch(10);
    return instructionAndCarry(toMemory<n8>(address), A);
  }
  case 0x29: {  //ORCF A,(mem)
    prefetch(10);
    return instructionOrCarry(toMemory<n8>(address), A);
  }
  case 0x2a: {  //XORCF A,(mem)
    prefetch(10);
    return instructionXorCarry(toMemory<n8>(address), A);
  }
  case 0x2b: {  //LDCF A,(mem)
    prefetch(10);
    return instructionLoadCarry(toMemory<n8>(address), A);
  }
  case 0x2c: {  //STCF A,(mem)
    prefetch(12);
    return instructionStoreCarry(toMemory<n8>(address), A);
  }
  case 0x2d ... 0x2f: {
    return undefined();
  }
  case 0x30 ... 0x37: {  //LDA.L R,mem
    prefetch(8);
    return instructionLoad(toRegister3<n32>(data), toImmediate<n32>(address));
  }
  case 0x38 ... 0x3f: {
    return undefined();
  }
  case 0x40 ... 0x47: {  //LD.B (mem),R
    prefetch(6);
    return instructionLoad(toMemory<n8>(address), toRegister3<n8>(data));
  }
  case 0x48 ... 0x4f: {
    return undefined();
  }
  case 0x50 ... 0x57: {  //LD.W (mem),R
    prefetch(6);
    return instructionLoad(toMemory<n16>(address), toRegister3<n16>(data));
  }
  case 0x58 ... 0x5f: {
    return undefined();
  }
  case 0x60 ... 0x67: {  //LD.L (mem),R
    prefetch(8);
    return instructionLoad(toMemory<n32>(address), toRegister3<n32>(data));
  }
  case 0x68 ... 0x7f: {
    return undefined();
  }
  case 0x80 ... 0x87: {  //ANDCF #3,(mem)
    prefetch(10);
    return instructionAndCarry(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0x88 ... 0x8f: {  //ORCF #3,(mem)
    prefetch(10);
    return instructionOrCarry(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0x90 ... 0x97: {  //XORCF #3,(mem)
    prefetch(10);
    return instructionXorCarry(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0x98 ... 0x9f: {  //LDCF #3,(mem)
    prefetch(10);
    return instructionLoadCarry(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xa0 ... 0xa7: {  //STCF #3,(mem)
    prefetch(10);
    return instructionStoreCarry(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xa8 ... 0xaf: {  //TSET #3,(mem)
    prefetch(10);
    return instructionTestSet(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xb0 ... 0xb7: {  //RES #3,(mem)
    prefetch(10);
    return instructionReset(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xb8 ... 0xbf: {  //SET #3,(mem)
    prefetch(10);
    return instructionSet(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xc0 ... 0xc7: {  //CHG #3,(mem)
    prefetch(10);
    return instructionChange(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xc8 ... 0xcf: {  //BIT #3,(mem)
    prefetch(10);
    return instructionBit(toMemory<n8>(address), toImmediate<n3>(data));
  }
  case 0xd0 ... 0xdf: {  //JP cc,mem
    prefetch(8);
    if(!condition((n4)data)) return;
    prefetch(4);
    instructionJump(toImmediate<n32>(address));
    return prefetch(2);
  }
  case 0xe0 ... 0xef: {  //CALL cc,mem
    if(!condition((n4)data)) return prefetch(8);
    prefetch(20);
    instructionCall(toImmediate<n32>(address));
    return prefetch(2);
  }
  case 0xf0 ... 0xff: {  //RET cc
    if(!condition((n4)data)) return prefetch(8);
    prefetch(20);
    instructionReturn();
    return prefetch(2);
  }

  }
}
