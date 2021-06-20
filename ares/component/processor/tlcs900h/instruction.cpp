#if defined(COMPILER_CLANG)
  //this core uses "Undefined;" many times, which Clang does not like to ignore.
  #pragma clang diagnostic ignored "-Wunused-value"
#endif

template<u32 Bits> auto TLCS900H::wait(u32 b, u32 w, u32 l) -> void {
  if constexpr(Bits ==  8) return wait(b);
  if constexpr(Bits == 16) return wait(w);
  if constexpr(Bits == 32) return wait(l);
}

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
  wait(16);
  instructionSoftwareInterrupt(2);
}

auto TLCS900H::instruction() -> void {
  auto data = fetch();

  switch(r.prefix = data) {
  case 0x00:  //NOP
    wait(2);
    return instructionNoOperation();
  case 0x01:  //NORMAL (not present on 900/H)
    return undefined();
  case 0x02:  //PUSH SR
    wait(3);
    return instructionPush(SR);
  case 0x03:  //POP SR
    wait(4);
    return instructionPop(SR);
  case 0x04:  //MAX or MIN (not present on 900/H)
    return undefined();
  case 0x05:  //HALT
    wait(6);
    return instructionHalt();
  case 0x06:  //EI n
    data = fetch();
    wait(3 + (n3(data) == 7));
    return instructionSetInterruptFlipFlop((n3)data);
  case 0x07:  //RETI
    wait(12);
    return instructionReturnInterrupt();
  case 0x08: {//LD (n),n
    auto memory = fetchMemory<n8, n8>();
    auto immediate = fetchImmediate<n8>();
    wait(5);
    return instructionLoad(memory, immediate); }
  case 0x09: {//PUSH n
    auto immediate = fetchImmediate<n8>();
    wait(4);
    return instructionPush(immediate); }
  case 0x0a: {//LD.W (n),nn
    auto memory = fetchMemory<n16, n8>();
    auto immediate = fetchImmediate<n16>();
    wait(6);
    return instructionLoad(memory, immediate); }
  case 0x0b: {//PUSH.W nn
    auto immediate = fetchImmediate<n16>();
    wait(5);
    return instructionPush(immediate); }
  case 0x0c:  //INCF
    wait(2);
    return instructionSetRegisterFilePointer(RFP + 1);
  case 0x0d:  ///DECF
    wait(2);
    return instructionSetRegisterFilePointer(RFP - 1);
  case 0x0e:  //RET
    wait(9);
    return instructionReturn();
  case 0x0f: {//RETD dd
    auto immediate = fetchImmediate<i16>();
    wait(11);
    return instructionReturnDeallocate(immediate); }
  case 0x10:  //RCF
    wait(2);
    return instructionSetCarryFlag(0);
  case 0x11:  //SCF
    wait(2);
    return instructionSetCarryFlag(1);
  case 0x12:  //CCF
    wait(2);
    return instructionSetCarryFlagComplement(CF);
  case 0x13:  //ZCF
    wait(2);
    return instructionSetCarryFlagComplement(ZF);
  case 0x14:  //PUSH A
    wait(3);
    return instructionPush(A);
  case 0x15:  //POP A
    wait(4);
    return instructionPop(A);
  case 0x16:  //EX F,F'
    wait(2);
    return instructionExchange(F, FP);
  case 0x17:  //LDF n
    data = fetch();
    wait(2);
    return instructionSetRegisterFilePointer((n2)data);
  case 0x18:  //PUSH F
    wait(3);
    return instructionPush(F);
  case 0x19:  //POP F
    wait(4);
    return instructionPop(F);
  case 0x1a: {//JP nn
    auto immediate = fetchImmediate<n16>();
    wait(5);
    return instructionJump(immediate); }
  case 0x1b: {//JP nnn
    auto immediate = fetchImmediate<n24>();
    wait(6);
    return instructionJump(immediate); }
  case 0x1c: {//CALL nn
    auto immediate = fetchImmediate<n16>();
    wait(9);
    return instructionCall(immediate); }
  case 0x1d: {//CALL nnn
    auto immediate = fetchImmediate<n24>();
    wait(10);
    return instructionCall(immediate); }
  case 0x1e: {//CALR PC+dd
    auto immediate = fetchImmediate<i16>();
    wait(10);
    return instructionCallRelative(immediate); }
  case 0x1f:
    return undefined();
  case 0x20 ... 0x27: {//LD R,n
    auto immediate = fetchImmediate<n8>();
    wait(2);
    return instructionLoad(toRegister3<n8>(data), immediate); }
  case 0x28 ... 0x2f:  //PUSH rr
    wait(3);
    return instructionPush(toRegister3<n16>(data));
  case 0x30 ... 0x37: {//LD RR,nn
    auto immediate = fetchImmediate<n16>();
    wait(3);
    return instructionLoad(toRegister3<n16>(data), immediate); }
  case 0x38 ... 0x3f:  //PUSH XRR
    wait(5);
    return instructionPush(toRegister3<n32>(data));
  case 0x40 ... 0x47: {//LD XRR,nnnn
    auto immediate = fetchImmediate<n32>();
    wait(5);
    return instructionLoad(toRegister3<n32>(data), immediate); }
  case 0x48 ... 0x4f:  //POP RR
    wait(4);
    return instructionPop(toRegister3<n16>(data));
  case 0x50 ... 0x57:
    return undefined();
  case 0x58 ... 0x5f:  //POP XRR
    wait(6);
    return instructionPop(toRegister3<n32>(data));
  case 0x60 ... 0x6f: {//JR cc,PC+d
    auto immediate = fetchImmediate<i8>();
    if(!condition((n4)data)) return wait(2);
    wait(5);
    return instructionJumpRelative(immediate); }
  case 0x70 ... 0x7f: {//JRL cc,PC+dd
    auto immediate = fetchImmediate<i16>();
    if(!condition((n4)data)) return wait(2);
    wait(5);
    return instructionJumpRelative(immediate); }
  case 0x80 ... 0x87:  //src.B
    return instructionSourceMemory(toMemory<n8>(load(toRegister3<n32>(data))));
  case 0x88 ... 0x8f: {//src.B
    auto immediate = fetch<i8>();
    return instructionSourceMemory(toMemory<n8>(load(toRegister3<n32>(data)) + immediate)); }
  case 0x90 ... 0x97:  //src.W
    return instructionSourceMemory(toMemory<n16>(load(toRegister3<n32>(data))));
  case 0x98 ... 0x9f: {//src.W
    auto immediate = fetch<i8>();
    return instructionSourceMemory(toMemory<n16>(load(toRegister3<n32>(data)) + immediate)); }
  case 0xa0 ... 0xa7:  //src.L
    return instructionSourceMemory(toMemory<n32>(load(toRegister3<n32>(data))));
  case 0xa8 ... 0xaf: {//src.L
    auto immediate = fetch<i8>();
    return instructionSourceMemory(toMemory<n32>(load(toRegister3<n32>(data)) + immediate)); }
  case 0xb0 ... 0xb7:  //dst
    return instructionTargetMemory(load(toRegister3<n32>(data)));
  case 0xb8 ... 0xbf: {//dst
    auto immediate = fetch<i8>();
    return instructionTargetMemory(load(toRegister3<n32>(data)) + immediate); }
  case 0xc0: {//src.B
    auto memory = fetchMemory<n8, n8>();
    return instructionSourceMemory(memory); }
  case 0xc1: {//src.B
    auto memory = fetchMemory<n8, n16>();
    return instructionSourceMemory(memory); }
  case 0xc2: {//src.B
    auto memory = fetchMemory<n8, n24>();
    return instructionSourceMemory(memory); }
  case 0xc3: {//src.B
    data = fetch();
    if((data & 3) == 0) {
      return instructionSourceMemory(toMemory<n8>(load(toRegister8<n32>(data))));
    }
    if((data & 3) == 1) {
      auto immediate = fetch<i16>();
      return instructionSourceMemory(toMemory<n8>(load(toRegister8<n32>(data)) + immediate));
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<n32>());
      auto r8  = load(fetchRegister<n8 >());
      return instructionSourceMemory(toMemory<n8>(r32 + (i8)r8));
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<n32>());
      auto r16 = load(fetchRegister<n16>());
      return instructionSourceMemory(toMemory<n8>(r32 + (i16)r16));
    }
    debug(unusual, "[TLCS900H::instruction] 0xc3 0x", hex(data, 2L));
    return undefined(); }
  case 0xc4: {//src.B
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    return instructionSourceMemory(toMemory<n8>(location)); }
  case 0xc5: {//src.B
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    return instructionSourceMemory(toMemory<n8>(location)); }
  case 0xc6:
    return undefined();
  case 0xc7: {//reg.B
    auto register = fetchRegister<n8>();
    return instructionRegister(register);
  }
  case 0xc8 ... 0xcf:  //reg.B
    return instructionRegister(toRegister3<n8>(data));
  case 0xd0: {//src.W
    auto memory = fetchMemory<n16, n8>();
    return instructionSourceMemory(memory); }
  case 0xd1: {//src.W
    auto memory = fetchMemory<n16, n16>();
    return instructionSourceMemory(memory); }
  case 0xd2: {//src.W
    auto memory = fetchMemory<n16, n24>();
    return instructionSourceMemory(memory); }
  case 0xd3: {//src.W
    data = fetch();
    if((data & 3) == 0) {
      return instructionSourceMemory(toMemory<n16>(load(toRegister8<n32>(data))));
    }
    if((data & 3) == 1) {
      auto immediate = fetch<i16>();
      return instructionSourceMemory(toMemory<n16>(load(toRegister8<n32>(data)) + immediate));
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<n32>());
      auto r8  = load(fetchRegister<n8 >());
      return instructionSourceMemory(toMemory<n16>(r32 + (i8)r8));
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<n32>());
      auto r16 = load(fetchRegister<n16>());
      return instructionSourceMemory(toMemory<n16>(r32 + (i16)r16));
    }
    debug(unusual, "[TLCS900H::instruction] 0xd3 0x", hex(data, 2L));
    return undefined(); }
  case 0xd4: {//src.W
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    return instructionSourceMemory(toMemory<n16>(location)); }
  case 0xd5: {//src.W
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    return instructionSourceMemory(toMemory<n16>(location)); }
  case 0xd6:
    return undefined();
  case 0xd7: {//reg.W
    auto register = fetchRegister<n16>();
    return instructionRegister(register); }
  case 0xd8 ... 0xdf:  //reg.W
    return instructionRegister(toRegister3<n16>(data));
  case 0xe0: {//src.L
    auto memory = fetchMemory<n32, n8>();
    return instructionSourceMemory(memory); }
  case 0xe1: {//src.L
    auto memory = fetchMemory<n32, n16>();
    return instructionSourceMemory(memory); }
  case 0xe2: {//src.L
    auto memory = fetchMemory<n32, n24>();
    return instructionSourceMemory(memory); }
  case 0xe3: {//src.L
    data = fetch();
    if((data & 3) == 0) {
      return instructionSourceMemory(toMemory<n32>(load(toRegister8<n32>(data))));
    }
    if((data & 3) == 1) {
      auto immediate = fetch<i16>();
      return instructionSourceMemory(toMemory<n32>(load(toRegister8<n32>(data)) + immediate));
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<n32>());
      auto r8  = load(fetchRegister<n8 >());
      return instructionSourceMemory(toMemory<n32>(r32 + (i8)r8));
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<n32>());
      auto r16 = load(fetchRegister<n16>());
      return instructionSourceMemory(toMemory<n32>(r32 + (i16)r16));
    }
    debug(unusual, "[TLCS900H::instruction] 0xe3 0x", hex(data, 2L));
    return undefined(); }
  case 0xe4: {//src.L
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    return instructionSourceMemory(toMemory<n32>(location)); }
  case 0xe5: {//src.L
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    return instructionSourceMemory(toMemory<n32>(location)); }
  case 0xe6:
    return undefined();
  case 0xe7: {//reg.L
    auto register = fetchRegister<n32>();
    return instructionRegister(register); }
  case 0xe8 ... 0xef:  //reg.L
    return instructionRegister(toRegister3<n32>(data));
  case 0xf0: {//dst
    auto memory = fetch<n8>();
    return instructionTargetMemory(memory); }
  case 0xf1: {//dst
    auto memory = fetch<n16>();
    return instructionTargetMemory(memory); }
  case 0xf2: {//dst
    auto memory = fetch<n24>();
    return instructionTargetMemory(memory); }
  case 0xf3: {//dst
    data = fetch();
    if((data & 3) == 0) {
      return instructionTargetMemory(load(toRegister8<n32>(data)));
    }
    if((data & 3) == 1) {
      auto immediate = fetch<i16>();
      return instructionTargetMemory(load(toRegister8<n32>(data)) + immediate);
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<n32>());
      auto r8  = load(fetchRegister<n8 >());
      return instructionTargetMemory(r32 + (i8)r8);
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<n32>());
      auto r16 = load(fetchRegister<n16>());
      return instructionTargetMemory(r32 + (i16)r16);
    }
    if(data == 0x13) {  //LDAR
      auto immediate = fetch<i16>();
      data = fetch();
      switch(data) {
      case 0x20 ... 0x27:
        wait(7);
        return instructionLoad(toRegister3<n16>(data), toImmediate<n16>(load(PC) + immediate));
      case 0x30 ... 0x37:
        wait(7);
        return instructionLoad(toRegister3<n32>(data), toImmediate<n32>(load(PC) + immediate));
      }
    }
    debug(unusual, "[TLCS900H::instruction] 0xf3 0x", hex(data, 2L));
    return undefined(); }
  case 0xf4: {//dst
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    return instructionTargetMemory(location); }
  case 0xf5: {//dst
    data = fetch();
    auto register = toRegister8<n32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    return instructionTargetMemory(location); }
  case 0xf6:
    return undefined();
  case 0xf7: {//LDX (n),n
    fetch();  //0x00
    auto memory = fetchMemory<n8, n8>();
    fetch();  //0x00
    auto immediate = fetchImmediate<n8>();
    fetch();  //0x00
    wait(8);
    return instructionLoad(memory, immediate); }
  case 0xf8 ... 0xff:  //SWI n
    wait(19);
    return instructionSoftwareInterrupt((n3)data);
  }
}

template<typename R>
auto TLCS900H::instructionRegister(R register) -> void {
  using T = typename R::type;
  enum : u32 { bits = R::bits };
  auto data = fetch();

  switch(data) {
  case 0x00 ... 0x02:
    return undefined();
  case 0x03: {//LD r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionLoad(register, immediate); }
  case 0x04:  //PUSH r
    wait<bits>(4,4,6);
    return instructionPush(register);
  case 0x05:  //POP r
    wait<bits>(5,5,7);
    return instructionPop(register);
  case 0x06:  //CPL r
    if constexpr(bits == 32) return undefined(); else {
    wait(2);
    return instructionComplement(register); }
  case 0x07:  //NEG r
    if constexpr(bits == 32) return undefined(); else {
    wait(2);
    return instructionNegate(register); }
  case 0x08:  //MUL rr,#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(12,15,0);
    return instructionMultiply(register, immediate); }
  case 0x09:  //MULS rr,#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(10,13,0);
    return instructionMultiplySigned(register, immediate); }
  case 0x0a:  //DIV rr,#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(15,23,0);
    return instructionDivide(register, immediate); }
  case 0x0b:  //DIVS rr,#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(18,26,0);
    return instructionDivideSigned(register, immediate); }
  case 0x0c:  //LINK r,dd
    if constexpr(bits != 32) return undefined(); else {
    auto immediate = fetchImmediate<i16>();
    wait(8);
    return instructionLink(register, immediate); }
  case 0x0d:  //UNLK r
    if constexpr(bits != 32) return undefined(); else {
    wait(7);
    return instructionUnlink(register); }
  case 0x0e:  //BS1F A,r
    if constexpr(bits != 16) return undefined(); else {
    wait(3);
    return instructionBitSearch1Forward(register); }
  case 0x0f:  //BS1B A,r
    if constexpr(bits != 16) return undefined(); else {
    wait(3);
    return instructionBitSearch1Backward(register); }
  case 0x10:  //DAA r
    if constexpr(bits != 8) return undefined(); else {
    wait(4);
    return instructionDecimalAdjustAccumulator(register); }
  case 0x11:
    return undefined();
  case 0x12:  //EXTZ r
    if constexpr(bits == 8) return undefined(); else {
    wait(3);
    return instructionExtendZero(register); }
  case 0x13:  //EXTS r
    if constexpr(bits == 8) return undefined(); else {
    wait(3);
    return instructionExtendSign(register); }
  case 0x14:  //PAA r
    if constexpr(bits == 8) return undefined(); else {
    wait(4);
    return instructionPointerAdjustAccumulator(register); }
  case 0x15:
    return undefined();
  case 0x16:  //MIRR r
    if constexpr(bits != 16) return undefined(); else {
    wait(3);
    return instructionMirror(register); }
  case 0x17 ... 0x18:
    return undefined();
  case 0x19:  //MULA r
    if constexpr(bits != 16) return undefined(); else {
    wait(19);
    return instructionMultiplyAdd(register); }
  case 0x1a ... 0x1b:
    return undefined();
  case 0x1c:  //DJNZ r,d
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<i8>();
    wait(6);  //todo: 4 when not looping
    return instructionDecrementJumpNotZero(register, immediate); }
  case 0x1d ... 0x1f:
    return undefined();
  case 0x20:  //ANDCF #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionAndCarry(register, immediate); }
  case 0x21:  //ORCF #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionOrCarry(register, immediate); }
  case 0x22:  //XORCF #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionXorCarry(register, immediate); }
  case 0x23:  //LDCF #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionLoadCarry(register, immediate); }
  case 0x24:  //STCF #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionStoreCarry(register, immediate); }
  case 0x25 ... 0x27:
    return undefined();
  case 0x28:  //ANDCF A,r
    if constexpr(bits == 32) return undefined(); else {
    wait(3);
    return instructionAndCarry(register, A); }
  case 0x29:  //ORCF A,r
    if constexpr(bits == 32) return undefined(); else {
    wait(3);
    return instructionOrCarry(register, A); }
  case 0x2a:  //XORCF A,r
    if constexpr(bits == 32) return undefined(); else {
    wait(3);
    return instructionXorCarry(register, A); }
  case 0x2b:  //LDCF A,r
    if constexpr(bits == 32) return undefined(); else {
    wait(3);
    return instructionLoadCarry(register, A); }
  case 0x2c:  //STCF A,r
    if constexpr(bits == 32) return undefined(); else {
    wait(3);
    return instructionStoreCarry(register, A); }
  case 0x2d:
    return undefined();
  case 0x2e:  //LDC cr,r
    data = fetch();
    wait(3);
    return instructionLoad(toControlRegister<T>(data), register);
  case 0x2f:  //LDC r,cr
    data = fetch();
    wait(3);
    return instructionLoad(register, toControlRegister<T>(data));
  case 0x30:  //RES #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionReset(register, immediate); }
  case 0x31:  //SET #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionSet(register, immediate); }
  case 0x32:  //CHG #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionChange(register, immediate); }
  case 0x33:  //BIT #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionBit(register, immediate); }
  case 0x34:  //TSET #,r
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<n8>();
    wait(4);
    return instructionTestSet(register, immediate); }
  case 0x35 ... 0x37:
    return undefined();
  case 0x38:  //MINC1 #,r
    if constexpr(bits != 16) return undefined(); else {
    auto immediate = fetchImmediate<n16>();
    wait(5);
    return instructionModuloIncrement<1>(register, immediate); }
  case 0x39:  //MINC2 #,r
    if constexpr(bits != 16) return undefined(); else {
    auto immediate = fetchImmediate<n16>();
    wait(5);
    return instructionModuloIncrement<2>(register, immediate); }
  case 0x3a:  //MINC4 #,r
    if constexpr(bits != 16) return undefined(); else {
    auto immediate = fetchImmediate<n16>();
    wait(5);
    return instructionModuloIncrement<4>(register, immediate); }
  case 0x3b:
    return undefined();
  case 0x3c:  //MDEC1 #,r
    if constexpr(bits != 16) return undefined(); else {
    auto immediate = fetchImmediate<n16>();
    wait(4);
    return instructionModuloDecrement<1>(register, immediate); }
  case 0x3d:  //MDEC2 #,r
    if constexpr(bits != 16) return undefined(); else {
    auto immediate = fetchImmediate<n16>();
    wait(4);
    return instructionModuloDecrement<2>(register, immediate); }
  case 0x3e:  //MDEC4 #,r
    if constexpr(bits != 16) return undefined(); else {
    auto immediate = fetchImmediate<n16>();
    wait(4);
    return instructionModuloDecrement<4>(register, immediate); }
  case 0x3f:
    return undefined();
  case 0x40 ... 0x47:  //MUL R,r
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(11,14,0);
    return instructionMultiply(toRegister3<T>(data), register); }
  case 0x48 ... 0x4f:  //MULS R,r
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(9,12,0);
    return instructionMultiplySigned(toRegister3<T>(data), register); }
  case 0x50 ... 0x57:  //DIV R,r
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(15,23,0);
    return instructionDivide(toRegister3<T>(data), register); }
  case 0x58 ... 0x5f:  //DIVS R,r
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(18,26,0);
    return instructionDivideSigned(toRegister3<T>(data), register); }
  case 0x60 ... 0x67:  //INC #3,r
    wait(2);
    return instructionIncrement(register, toImmediate<T>((n3)data));
  case 0x68 ... 0x6f:  //DEC #3,r
    wait(2);
    return instructionDecrement(register, toImmediate<T>((n3)data));
  case 0x70 ... 0x7f:  //SCC cc,r
    if constexpr(bits == 32) return undefined(); else {
    wait(2);
    return instructionSetConditionCode((n4)data, register); }
  case 0x80 ... 0x87:  //ADD R,r
    wait(2);
    return instructionAdd(toRegister3<T>(data), register);
  case 0x88 ... 0x8f:  //LD R,r
    wait(2);
    return instructionLoad(toRegister3<T>(data), register);
  case 0x90 ... 0x97:  //ADC R,r
    wait(2);
    return instructionAddCarry(toRegister3<T>(data), register);
  case 0x98 ... 0x9f:  //LD r,R
    wait(2);
    return instructionLoad(register, toRegister3<T>(data));
  case 0xa0 ... 0xa7:  //SUB R,r
    wait(2);
    return instructionSubtract(toRegister3<T>(data), register);
  case 0xa8 ... 0xaf:  //LD r,#3
    wait(2);
    return instructionLoad(register, toImmediate<T>((n3)data));
  case 0xb0 ... 0xb7:  //SBC R,r
    wait(2);
    return instructionSubtractBorrow(toRegister3<T>(data), register);
  case 0xb8 ... 0xbf:  //EX R,r
    if constexpr(bits == 32) return undefined(); else {
    wait(3);
    return instructionExchange(toRegister3<T>(data), register); }
  case 0xc0 ... 0xc7:  //AND R,r
    wait(2);
    return instructionAnd(toRegister3<T>(data), register);
  case 0xc8: {//ADD r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionAdd(register, immediate); }
  case 0xc9: {//ADC r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionAddCarry(register, immediate); }
  case 0xca: {//SUB r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionSubtract(register, immediate); }
  case 0xcb: {//SBC r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionSubtractBorrow(register, immediate); }
  case 0xcc: {//AND r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionAnd(register, immediate); }
  case 0xcd: {//XOR r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionXor(register, immediate); }
  case 0xce: {//OR r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionOr(register, immediate); }
  case 0xcf: {//CP r,#
    auto immediate = fetchImmediate<T>();
    wait<bits>(3,4,6);
    return instructionCompare(register, immediate); }
  case 0xd0 ... 0xd7:  //XOR R,r
    wait(2);
    return instructionXor(toRegister3<T>(data), register);
  case 0xd8 ... 0xdf:  //CP r,#3
    if constexpr(bits == 32) return undefined(); else {
    wait(2);
    return instructionCompare(register, toImmediate<T>((n3)data)); }
  case 0xe0 ... 0xe7:  //OR R,r
    wait(2);
    return instructionOr(toRegister3<T>(data), register);
  case 0xe8: {//RLC #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionRotateLeftWithoutCarry(register, immediate); }
  case 0xe9: {//RRC #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionRotateRightWithoutCarry(register, immediate); }
  case 0xea: {//RL #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionRotateLeft(register, immediate); }
  case 0xeb: {//RR #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionRotateRight(register, immediate); }
  case 0xec: {//SLA #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionShiftLeftArithmetic(register, immediate); }
  case 0xed: {//SRA #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionShiftRightArithmetic(register, immediate); }
  case 0xee: {//SLL #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionShiftLeftLogical(register, immediate); }
  case 0xef: {//SRL #,r
    auto immediate = fetchImmediate<n8>();
    wait(3);
    return instructionShiftRightLogical(register, immediate); }
  case 0xf0 ... 0xf7:  //CP R,r
    wait(2);
    return instructionCompare(toRegister3<T>(data), register);
  case 0xf8:  //RLC A,r
    wait(3);
    return instructionRotateLeftWithoutCarry(register, A);
  case 0xf9:  //RRC A,r
    wait(3);
    return instructionRotateRightWithoutCarry(register, A);
  case 0xfa:  //RL A,r
    wait(3);
    return instructionRotateLeft(register, A);
  case 0xfb:  //RR A,r
    wait(3);
    return instructionRotateRight(register, A);
  case 0xfc:  //SLA A,r
    wait(3);
    return instructionShiftLeftArithmetic(register, A);
  case 0xfd:  //SRA A,r
    wait(3);
    return instructionShiftRightArithmetic(register, A);
  case 0xfe:  //SLL A,r
    wait(3);
    return instructionShiftLeftLogical(register, A);
  case 0xff:  //SRL A,r
    wait(3);
    return instructionShiftRightLogical(register, A);
  }
}

template<typename M>
auto TLCS900H::instructionSourceMemory(M memory) -> void {
  using T = typename M::type;
  enum : u32 { bits = M::bits };
  auto data = fetch();

  switch(data) {
  case 0x00 ... 0x03:
    return undefined();
  case 0x04:  //PUSH (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionPush(memory); }
  case 0x05:
    return undefined();
  case 0x06:  //RLD A,(mem)
    if constexpr(bits != 8) return undefined(); else {
    wait(14);
    return instructionRotateLeftDigit(A, memory); }
  case 0x07:  //RRD A,(mem)
    if constexpr(bits != 8) return undefined(); else {
    wait(14);
    return instructionRotateRightDigit(A, memory); }
  case 0x08 ... 0x0f:
    return undefined();
  case 0x10:  //LDI
    if constexpr(bits ==  8) { wait(8); return instructionLoad<T, +1>(); }
    if constexpr(bits == 16) { wait(8); return instructionLoad<T, +2>(); }
    return undefined();
  case 0x11:  //LDIR
    if constexpr(bits ==  8) { wait(1); return instructionLoadRepeat<T, +1>(); }
    if constexpr(bits == 16) { wait(1); return instructionLoadRepeat<T, +2>(); }
    return undefined();
  case 0x12:  //LDD
    if constexpr(bits ==  8) { wait(8); return instructionLoad<T, -1>(); }
    if constexpr(bits == 16) { wait(8); return instructionLoad<T, -2>(); }
    return undefined();
  case 0x13:  //LDDR
    if constexpr(bits ==  8) { wait(1); return instructionLoadRepeat<T, -1>(); }
    if constexpr(bits == 16) { wait(1); return instructionLoadRepeat<T, -2>(); }
    return undefined();
  case 0x14:  //CPI
    if constexpr(bits ==  8) { wait(6); return instructionCompare<T, +1>( A); }
    if constexpr(bits == 16) { wait(6); return instructionCompare<T, +2>(WA); }
    return undefined();
  case 0x15:  //CPIR
    if constexpr(bits ==  8) { wait(1); return instructionCompareRepeat<T, +1>( A); }
    if constexpr(bits == 16) { wait(1); return instructionCompareRepeat<T, +2>(WA); }
    return undefined();
  case 0x16:  //CPD
    if constexpr(bits ==  8) { wait(6); return instructionCompare<T, -1>( A); }
    if constexpr(bits == 16) { wait(6); return instructionCompare<T, -2>(WA); }
    return undefined();
  case 0x17:  //CPDR
    if constexpr(bits ==  8) { wait(1); return instructionCompareRepeat<T, -1>( A); }
    if constexpr(bits == 16) { wait(1); return instructionCompareRepeat<T, -2>(WA); }
    return undefined();
  case 0x18:
    return undefined();
  case 0x19:  //LD (nn),(m)
    if constexpr(bits == 32) return undefined(); else {
    auto target = fetchMemory<T, n16>();
    wait(8);
    return instructionLoad(target, memory); }
  case 0x1a ... 0x1f:
    return undefined();
  case 0x20 ... 0x27:  //LD R,(mem)
    wait<bits>(4,4,6);
    return instructionLoad(toRegister3<T>(data), memory);
  case 0x28 ... 0x2f:
    return undefined();
  case 0x30 ... 0x37:  //EX (mem),R
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionExchange(memory, toRegister3<T>(data)); }
  case 0x38:  //ADD (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionAdd(memory, immediate); }
  case 0x39:  //ADC (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionAddCarry(memory, immediate); }
  case 0x3a:  //SUB (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionSubtract(memory, immediate); }
  case 0x3b:  //SBC (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionSubtractBorrow(memory, immediate); }
  case 0x3c:  //AND (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionAnd(memory, immediate); }
  case 0x3d:  //XOR (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionXor(memory, immediate); }
  case 0x3e:  //OR (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(7,8,0);
    return instructionOr(memory, immediate); }
  case 0x3f:  //CP (mem),#
    if constexpr(bits == 32) return undefined(); else {
    auto immediate = fetchImmediate<T>();
    wait<bits>(5,6,0);
    return instructionCompare(memory, immediate); }
  case 0x40 ... 0x47:  //MUL R,(mem)
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(13,16,0);
    return instructionMultiply(toRegister3<T>(data), memory); }
  case 0x48 ... 0x4f:  //MULS R,(mem)
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(11,14,0);
    return instructionMultiplySigned(toRegister3<T>(data), memory); }
  case 0x50 ... 0x57:  //DIV R,(mem)
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(16,24,0);
    return instructionDivide(toRegister3<T>(data), memory); }
  case 0x58 ... 0x5f:  //DIVS R,(mem)
    if constexpr(bits == 32) return undefined(); else {
    wait<bits>(19,27,0);
    return instructionDivideSigned(toRegister3<T>(data), memory); }
  case 0x60 ... 0x67:  //INC #3,(mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionIncrement(memory, toImmediate<T>((n3)data)); }
  case 0x68 ... 0x6f:  //DEC #3,(mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionDecrement(memory, toImmediate<T>((n3)data)); }
  case 0x70 ... 0x77:
    return undefined();
  case 0x78:  //RLC (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionRotateLeftWithoutCarry(memory, toImmediate<n4>(1)); }
  case 0x79:  //RRC (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionRotateRightWithoutCarry(memory, toImmediate<n4>(1)); }
  case 0x7a:  //RL (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionRotateLeft(memory, toImmediate<n4>(1)); }
  case 0x7b:  //RR (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionRotateRight(memory, toImmediate<n4>(1)); }
  case 0x7c:  //SLA (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionShiftLeftArithmetic(memory, toImmediate<n4>(1)); }
  case 0x7d:  //SRA (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionShiftRightArithmetic(memory, toImmediate<n4>(1)); }
  case 0x7e:  //SLL (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionShiftLeftLogical(memory, toImmediate<n4>(1)); }
  case 0x7f:  //SRL (mem)
    if constexpr(bits == 32) return undefined(); else {
    wait(6);
    return instructionShiftRightLogical(memory, toImmediate<n4>(1)); }
  case 0x80 ... 0x87:  //ADD R,(mem)
    wait<bits>(4,4,6);
    return instructionAdd(toRegister3<T>(data), memory);
  case 0x88 ... 0x8f:  //ADD (mem),R
    wait<bits>(6,6,10);
    return instructionAdd(memory, toRegister3<T>(data));
  case 0x90 ... 0x97:  //ADC R,(mem)
    wait<bits>(4,4,6);
    return instructionAddCarry(toRegister3<T>(data), memory);
  case 0x98 ... 0x9f:  //ADC (mem),R
    wait<bits>(6,6,10);
    return instructionAddCarry(memory, toRegister3<T>(data));
  case 0xa0 ... 0xa7:  //SUB R,(mem)
    wait<bits>(4,4,6);
    return instructionSubtract(toRegister3<T>(data), memory);
  case 0xa8 ... 0xaf:  //SUB (mem),R
    wait<bits>(6,6,10);
    return instructionSubtract(memory, toRegister3<T>(data));
  case 0xb0 ... 0xb7:  //SBC R,(mem)
    wait<bits>(4,4,6);
    return instructionSubtractBorrow(toRegister3<T>(data), memory);
  case 0xb8 ... 0xbf:  //SBC (mem),R
    wait<bits>(6,6,10);
    return instructionSubtractBorrow(memory, toRegister3<T>(data));
  case 0xc0 ... 0xc7:  //AND R,(mem)
    wait<bits>(4,4,6);
    return instructionAnd(toRegister3<T>(data), memory);
  case 0xc8 ... 0xcf:  //AND (mem),R
    wait<bits>(6,6,10);
    return instructionAnd(memory, toRegister3<T>(data));
  case 0xd0 ... 0xd7:  //XOR R,(mem)
    wait<bits>(4,4,6);
    return instructionXor(toRegister3<T>(data), memory);
  case 0xd8 ... 0xdf:  //XOR (mem),R
    wait<bits>(6,6,10);
    return instructionXor(memory, toRegister3<T>(data));
  case 0xe0 ... 0xe7:  //OR R,(mem)
    wait<bits>(4,4,6);
    return instructionOr(toRegister3<T>(data), memory);
  case 0xe8 ... 0xef:  //OR (mem),R
    wait<bits>(6,6,10);
    return instructionOr(memory, toRegister3<T>(data));
  case 0xf0 ... 0xf7:  //CP R,(mem)
    wait<bits>(4,4,6);
    return instructionCompare(toRegister3<T>(data), memory);
  case 0xf8 ... 0xff:  //CP (mem),R
    wait<bits>(4,4,6);
    return instructionCompare(memory, toRegister3<T>(data));
  }
}

auto TLCS900H::instructionTargetMemory(n32 address) -> void {
  auto data = fetch();

  switch(data) {
  case 0x00: {//LD.B (m),#
    auto immediate = fetchImmediate<n8>();
    wait(5);
    return instructionLoad(toMemory<n8>(address), immediate); }
  case 0x01:
    return undefined();
  case 0x02: {//LD.W (m),#
    auto immediate = fetchImmediate<n16>();
    wait(6);
    return instructionLoad(toMemory<n16>(address), immediate); }
  case 0x03:
    return undefined();
  case 0x04:  //POP.B (mem)
    wait(7);
    return instructionPop(toMemory<n8>(address));
  case 0x05:
    return undefined();
  case 0x06:  //POP.W (mem)
    wait(7);
    return instructionPop(toMemory<n16>(address));
  case 0x07 ... 0x13:
    return undefined();
  case 0x14: {//LD.B (m),(nn)
    auto source = fetchMemory<n8, n16>();
    wait(8);
    return instructionLoad(toMemory<n8>(address), source); }
  case 0x15:
    return undefined();
  case 0x16: {//LD.W (m),(nn)
    auto source = fetchMemory<n16, n16>();
    wait(8);
    return instructionLoad(toMemory<n16>(address), source); }
  case 0x17 ... 0x1f:
    return undefined();
  case 0x20 ... 0x27:  //LDA.W R,mem
    wait(4);
    return instructionLoad(toRegister3<n16>(data), toImmediate<n16>(address));
  case 0x28:  //ANDCF A,(mem)
    wait(6);
    return instructionAndCarry(toMemory<n8>(address), A);
  case 0x29:  //ORCF A,(mem)
    wait(6);
    return instructionOrCarry(toMemory<n8>(address), A);
  case 0x2a:  //XORCF A,(mem)
    wait(6);
    return instructionXorCarry(toMemory<n8>(address), A);
  case 0x2b:  //LDCF A,(mem)
    wait(6);
    return instructionLoadCarry(toMemory<n8>(address), A);
  case 0x2c:  //STCF A,(mem)
    wait(7);
    return instructionStoreCarry(toMemory<n8>(address), A);
  case 0x2d ... 0x2f:
    return undefined();
  case 0x30 ... 0x37:  //LDA.L R,mem
    wait(4);
    return instructionLoad(toRegister3<n32>(data), toImmediate<n32>(address));
  case 0x38 ... 0x3f:
    return undefined();
  case 0x40 ... 0x47:  //LD.B (mem),R
    wait(4);
    return instructionLoad(toMemory<n8>(address), toRegister3<n8>(data));
  case 0x48 ... 0x4f:
    return undefined();
  case 0x50 ... 0x57:  //LD.W (mem),R
    wait(4);
    return instructionLoad(toMemory<n16>(address), toRegister3<n16>(data));
  case 0x58 ... 0x5f:
    return undefined();
  case 0x60 ... 0x67:  //LD.L (mem),R
    wait(6);
    return instructionLoad(toMemory<n32>(address), toRegister3<n32>(data));
  case 0x68 ... 0x7f:
    return undefined();
  case 0x80 ... 0x87:  //ANDCF #3,(mem)
    wait(6);
    return instructionAndCarry(toMemory<n8>(address), toImmediate<n3>(data));
  case 0x88 ... 0x8f:  //ORCF #3,(mem)
    wait(6);
    return instructionOrCarry(toMemory<n8>(address), toImmediate<n3>(data));
  case 0x90 ... 0x97:  //XORCF #3,(mem)
    wait(6);
    return instructionXorCarry(toMemory<n8>(address), toImmediate<n3>(data));
  case 0x98 ... 0x9f:  //LDCF #3,(mem)
    wait(6);
    return instructionLoadCarry(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xa0 ... 0xa7:  //STCF #3,(mem)
    wait(7);
    return instructionStoreCarry(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xa8 ... 0xaf:  //TSET #3,(mem)
    wait(7);
    return instructionTestSet(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xb0 ... 0xb7:  //RES #3,(mem)
    wait(7);
    return instructionReset(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xb8 ... 0xbf:  //SET #3,(mem)
    wait(7);
    return instructionSet(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xc0 ... 0xc7:  //CHG #3,(mem)
    wait(7);
    return instructionChange(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xc8 ... 0xcf:  //BIT #3,(mem)
    wait(6);
    return instructionBit(toMemory<n8>(address), toImmediate<n3>(data));
  case 0xd0 ... 0xdf:  //JP cc,mem
    if(!condition((n4)data)) return wait(4);
    wait(7);
    return instructionJump(toImmediate<n32>(address));
  case 0xe0 ... 0xef:  //CALL cc,mem
    if(!condition((n4)data)) return wait(4);
    wait(12);
    return instructionCall(toImmediate<n32>(address));
  case 0xf0 ... 0xff:  //RET cc
    if(!condition((n4)data)) return wait(4);
    wait(12);
    return instructionReturn();
  }
}
