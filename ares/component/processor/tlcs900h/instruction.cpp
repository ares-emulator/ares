#if defined(COMPILER_CLANG)
  //this core uses "Undefined;" many times, which Clang does not like to ignore.
  #pragma clang diagnostic ignored "-Wunused-value"
#endif

template<uint Bits> auto TLCS900H::idleBW(uint b, uint w) -> void {
  if constexpr(Bits ==  8) return idle(b);
  if constexpr(Bits == 16) return idle(w);
}

template<uint Bits> auto TLCS900H::idleWL(uint w, uint l) -> void {
  if constexpr(Bits == 16) return idle(w);
  if constexpr(Bits == 32) return idle(l);
}

template<uint Bits> auto TLCS900H::idleBWL(uint b, uint w, uint l) -> void {
  if constexpr(Bits ==  8) return idle(b);
  if constexpr(Bits == 16) return idle(w);
  if constexpr(Bits == 32) return idle(l);
}

template<> auto TLCS900H::toRegister3<uint8>(uint3 code) const -> Register<uint8> {
  static const Register< uint8> lookup[] = {W, A, B, C, D, E, H, L};
  return lookup[code];
}

template<> auto TLCS900H::toRegister3<uint16>(uint3 code) const -> Register<uint16> {
  static const Register<uint16> lookup[] = {WA, BC, DE, HL, IX, IY, IZ, SP};
  return lookup[code];
}

template<> auto TLCS900H::toRegister3<uint32>(uint3 code) const -> Register<uint32> {
  static const Register<uint32> lookup[] = {XWA, XBC, XDE, XHL, XIX, XIY, XIZ, XSP};
  return lookup[code];
}

template<typename T> auto TLCS900H::toRegister8(uint8 code) const -> Register<T> { return {code}; }
template<typename T> auto TLCS900H::toControlRegister(uint8 code) const -> ControlRegister<T> { return {code}; }
template<typename T> auto TLCS900H::toMemory(uint32 address) const -> Memory<T> { return {address}; }
template<typename T> auto TLCS900H::toImmediate(uint32 constant) const -> Immediate<T> { return {constant}; }
template<typename T> auto TLCS900H::toImmediate3(natural constant) const -> Immediate<T> { return {constant.clip(3) ? constant.clip(3) : 8}; }

//note: much of this code is split to multiple statements due to C++ not guaranteeing
//the order of evaluations of function arguments. fetch() ordering is critical.

auto TLCS900H::undefined() -> void {
  instructionSoftwareInterrupt(2);
}

auto TLCS900H::instruction() -> void {
  prefetch();
  auto data = fetch();

  switch(r.prefix = data) {
  case 0x00:
    return instructionNoOperation();
  case 0x01:
    return undefined();  //NORMAL (not present on 900/H)
  case 0x02:
    idle(1);
    return instructionPush(SR);
  case 0x03:
    idle(2);
    return instructionPop(SR);
  case 0x04:
    return undefined();  //MAX or MIN (not present on 900/H)
  case 0x05:
    return instructionHalt();
  case 0x06:
    return instructionSetInterruptFlipFlop((uint3)fetch());
  case 0x07:
    return instructionReturnInterrupt();
  case 0x08: {
    auto memory = fetchMemory<uint8, uint8>();
    idle(1);
    return instructionLoad(memory, fetchImmediate<uint8>()); }
  case 0x09:
    return instructionPush(fetchImmediate<uint8>());
  case 0x0a: {
    auto memory = fetchMemory<uint16, uint8>();
    idle(2);
    return instructionLoad(memory, fetchImmediate<uint16>()); }
  case 0x0b:
    idle(1);
    return instructionPush(fetchImmediate<uint16>());
  case 0x0c:
    return instructionSetRegisterFilePointer(RFP + 1);
  case 0x0d:
    return instructionSetRegisterFilePointer(RFP - 1);
  case 0x0e:
    return instructionReturn();
  case 0x0f:
    return instructionReturnDeallocate(fetchImmediate<int16>());
  case 0x10:
    return instructionSetCarryFlag(0);
  case 0x11:
    return instructionSetCarryFlag(1);
  case 0x12:
    return instructionSetCarryFlagComplement(CF);
  case 0x13:
    return instructionSetCarryFlagComplement(ZF);
  case 0x14:
    idle(1);
    return instructionPush(A);
  case 0x15:
    idle(2);
    return instructionPop(A);
  case 0x16:
    return instructionExchange(F, FP);
  case 0x17:
    return instructionSetRegisterFilePointer((uint2)fetch());
  case 0x18:
    idle(1);
    return instructionPush(F);
  case 0x19:
    idle(2);
    return instructionPop(F);
  case 0x1a:
    return instructionJump(fetchImmediate<uint16>());
  case 0x1b:
    idle(1);
    return instructionJump(fetchImmediate<uint24>());
  case 0x1c:
    return instructionCall(fetchImmediate<uint16>());
  case 0x1d:
    idle(1);
    return instructionCall(fetchImmediate<uint24>());
  case 0x1e:
    return instructionCallRelative(fetchImmediate<int16>());
  case 0x1f:
    return undefined();
  case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    return instructionLoad(toRegister3<uint8>(data), fetchImmediate<uint8>());
  case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
    idle(1);
    return instructionPush(toRegister3<uint16>(data));
  case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    idle(1);
    return instructionLoad(toRegister3<uint16>(data), fetchImmediate<uint16>());
  case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
    idle(1);
    return instructionPush(toRegister3<uint32>(data));
  case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    idle(3);
    return instructionLoad(toRegister3<uint32>(data), fetchImmediate<uint32>());
  case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
    idle(2);
    return instructionPop(toRegister3<uint16>(data));
  case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    return undefined();
  case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    idle(4);
    return instructionPop(toRegister3<uint32>(data));
  case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
  case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: {
    auto immediate = fetchImmediate<int8>();
    if(!condition((uint4)data)) return;
    return instructionJumpRelative(immediate); }
  case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
  case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: {
    auto immediate = fetchImmediate<int16>();
    if(!condition((uint4)data)) return;
    return instructionJumpRelative(immediate); }
  case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    return instructionSourceMemory(toMemory<uint8>(load(toRegister3<uint32>(data))));
  case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    idle(1);
    return instructionSourceMemory(toMemory<uint8>(load(toRegister3<uint32>(data)) + fetch<int8>()));
  case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    return instructionSourceMemory(toMemory<uint16>(load(toRegister3<uint32>(data))));
  case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
    idle(1);
    return instructionSourceMemory(toMemory<uint16>(load(toRegister3<uint32>(data)) + fetch<int8>()));
  case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    return instructionSourceMemory(toMemory<uint32>(load(toRegister3<uint32>(data))));
  case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    idle(1);
    return instructionSourceMemory(toMemory<uint32>(load(toRegister3<uint32>(data)) + fetch<int8>()));
  case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    return instructionTargetMemory(load(toRegister3<uint32>(data)));
  case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
    idle(1);
    return instructionTargetMemory(load(toRegister3<uint32>(data)) + fetch<int8>());
  case 0xc0:
    idle(1);
    return instructionSourceMemory(fetchMemory<uint8,  uint8>());
  case 0xc1:
    idle(2);
    return instructionSourceMemory(fetchMemory<uint8, uint16>());
  case 0xc2:
    idle(3);
    return instructionSourceMemory(fetchMemory<uint8, uint24>());
  case 0xc3: {
    data = fetch();
    if((data & 3) == 0) {
      idle(1);
      return instructionSourceMemory(toMemory<uint8>(load(toRegister8<uint32>(data))));
    }
    if((data & 3) == 1) {
      idle(3);
      return instructionSourceMemory(toMemory<uint8>(load(toRegister8<uint32>(data)) + fetch<int16>()));
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<uint32>());
      auto  r8 = load(fetchRegister< uint8>());
      idle(3);
      return instructionSourceMemory(Memory<uint8>{r32 + (int8)r8});
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<uint32>());
      auto r16 = load(fetchRegister<uint16>());
      idle(3);
      return instructionSourceMemory(Memory<uint8>{r32 + (int16)r16});
    }
    return undefined(); }
  case 0xc4: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionSourceMemory(toMemory<uint8>(location)); }
  case 0xc5: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionSourceMemory(toMemory<uint8>(location)); }
  case 0xc6:
    return undefined();
  case 0xc7:
    return instructionRegister(fetchRegister<uint8>());
  case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
    return instructionRegister(toRegister3<uint8>(data));
  case 0xd0:
    idle(1);
    return instructionSourceMemory(fetchMemory<uint16,  uint8>());
  case 0xd1:
    idle(2);
    return instructionSourceMemory(fetchMemory<uint16, uint16>());
  case 0xd2:
    idle(3);
    return instructionSourceMemory(fetchMemory<uint16, uint24>());
  case 0xd3: {
    data = fetch();
    if((data & 3) == 0) {
      idle(1);
      return instructionSourceMemory(toMemory<uint16>(load(toRegister8<uint32>(data))));
    }
    if((data & 3) == 1) {
      idle(3);
      return instructionSourceMemory(toMemory<uint16>(load(toRegister8<uint32>(data)) + fetch<int16>()));
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<uint32>());
      auto  r8 = load(fetchRegister< uint8>());
      idle(3);
      return instructionSourceMemory(toMemory<uint16>(r32 + (int8)r8));
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<uint32>());
      auto r16 = load(fetchRegister<uint16>());
      idle(3);
      return instructionSourceMemory(toMemory<uint16>(r32 + (int16)r16));
    }
    return undefined(); }
  case 0xd4: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionSourceMemory(toMemory<uint16>(location)); }
  case 0xd5: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionSourceMemory(toMemory<uint16>(location)); }
  case 0xd6:
    return undefined();
  case 0xd7:
    return instructionRegister(fetchRegister<uint16>());
  case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
    return instructionRegister(toRegister3<uint16>(data));
  case 0xe0:
    idle(1);
    return instructionSourceMemory(fetchMemory<uint32,  uint8>());
  case 0xe1:
    idle(2);
    return instructionSourceMemory(fetchMemory<uint32, uint16>());
  case 0xe2:
    idle(3);
    return instructionSourceMemory(fetchMemory<uint32, uint24>());
  case 0xe3: {
    data = fetch();
    if((data & 3) == 0) {
      idle(1);
      return instructionSourceMemory(toMemory<uint32>(load(toRegister8<uint32>(data))));
    }
    if((data & 3) == 1) {
      idle(3);
      return instructionSourceMemory(toMemory<uint32>(load(toRegister8<uint32>(data)) + fetch<int16>()));
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<uint32>());
      auto  r8 = load(fetchRegister< uint8>());
      idle(3);
      return instructionSourceMemory(toMemory<uint32>(r32 + (int8)r8));
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<uint32>());
      auto r16 = load(fetchRegister<uint16>());
      idle(3);
      return instructionSourceMemory(toMemory<uint32>(r32 + (int16)r16));
    }
    return undefined(); }
  case 0xe4: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionSourceMemory(toMemory<uint32>(location)); }
  case 0xe5: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionSourceMemory(toMemory<uint32>(location)); }
  case 0xe6:
    return undefined();
  case 0xe7:
    return instructionRegister(fetchRegister<uint32>());
  case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
    return instructionRegister(toRegister3<uint32>(data));
  case 0xf0:
    idle(1);
    return instructionTargetMemory(fetch< uint8>());
  case 0xf1:
    idle(2);
    return instructionTargetMemory(fetch<uint16>());
  case 0xf2:
    idle(3);
    return instructionTargetMemory(fetch<uint24>());
  case 0xf3: {
    data = fetch();
    if((data & 3) == 0) {
      idle(1);
      return instructionTargetMemory(load(toRegister8<uint32>(data)));
    }
    if((data & 3) == 1) {
      idle(3);
      return instructionTargetMemory(load(toRegister8<uint32>(data)) + fetch<int16>());
    }
    if(data == 0x03) {
      auto r32 = load(fetchRegister<uint32>());
      auto  r8 = load(fetchRegister< uint8>());
      idle(3);
      return instructionTargetMemory(r32 + (int8)r8);
    }
    if(data == 0x07) {
      auto r32 = load(fetchRegister<uint32>());
      auto r16 = load(fetchRegister<uint16>());
      idle(3);
      return instructionTargetMemory(r32 + (int16)r16);
    }
    if(data == 0x17) {
      auto d16 = fetch<int16>();
      data = fetch();
      idle(5);
      if((data & 0xf8) == 0x20) {
        auto register = toRegister3<uint16>(data);
        store(register, load(PC) + d16);
        return;
      }
      if((data & 0xf8) == 0x30) {
        auto register = toRegister3<uint32>(data);
        store(register, load(PC) + d16);
        return;
      }
    }
    return undefined(); }
  case 0xf4: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location -= 1);
    if((data & 3) == 1) store(register, location -= 2);
    if((data & 3) == 2) store(register, location -= 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionTargetMemory(location); }
  case 0xf5: {
    data = fetch();
    auto register = toRegister8<uint32>(data);
    auto location = load(register);
    if((data & 3) == 0) store(register, location + 1);
    if((data & 3) == 1) store(register, location + 2);
    if((data & 3) == 2) store(register, location + 4);
    if((data & 3) == 3) Undefined;
    idle(1);
    return instructionTargetMemory(location); }
  case 0xf6:
    return undefined();
  case 0xf7: {
    if(fetch()) Undefined;
    auto memory = fetchMemory<uint8, uint8>();
    if(fetch()) Undefined;
    auto immediate = fetchImmediate<uint8>();
    if(fetch()) Undefined;
    idle(4);
    return instructionLoad(memory, immediate); }
  case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
    return instructionSoftwareInterrupt((uint3)data);
  }
}

template<typename R>
auto TLCS900H::instructionRegister(R register) -> void {
  using T = typename R::type;
  enum : uint { bits = R::bits };
  auto data = fetch();

  switch(data) {
  case 0x00: case 0x01: case 0x02:
    return undefined();
  case 0x03:
    idleBWL<bits>(1, 2, 4);
    return instructionLoad(register, fetchImmediate<T>());
  case 0x04:
    idleBWL<bits>(2, 2, 4);
    return instructionPush(register);
  case 0x05:
    idleBWL<bits>(3, 3, 5);
    return instructionPop(register);
  case 0x06:
    if constexpr(bits == 32) return undefined(); else {
    return instructionComplement(register); }
  case 0x07:
    if constexpr(bits == 32) return undefined(); else {
    return instructionNegate(register); }
  case 0x08:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(10, 13);
    return instructionMultiply(register, fetchImmediate<T>()); }
  case 0x09:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(8, 11);
    return instructionMultiplySigned(register, fetchImmediate<T>()); }
  case 0x0a:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(13, 21);
    return instructionDivide(register, fetchImmediate<T>()); }
  case 0x0b:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(16, 24);
    return instructionDivideSigned(register, fetchImmediate<T>()); }
  case 0x0c:
    if constexpr(bits != 32) return undefined(); else {
    idle(6);
    return instructionLink(register, fetchImmediate<int16>()); }
  case 0x0d:
    if constexpr(bits != 32) return undefined(); else {
    idle(5);
    return instructionUnlink(register); }
  case 0x0e:
    if constexpr(bits != 16) return undefined(); else {
    idle(1);
    return instructionBitSearch1Forward(register); }
  case 0x0f:
    if constexpr(bits != 16) return undefined(); else {
    idle(1);
    return instructionBitSearch1Backward(register); }
  case 0x10:
    if constexpr(bits != 8) return undefined(); else {
    return instructionDecimalAdjustAccumulator(register); }
  case 0x11:
    return undefined();
  case 0x12:
    if constexpr(bits == 8) return undefined(); else {
    idle(1);
    return instructionExtendZero(register); }
  case 0x13:
    if constexpr(bits == 8) return undefined(); else {
    idle(1);
    return instructionExtendSign(register); }
  case 0x14:
    if constexpr(bits == 8) return undefined(); else {
    idle(2);
    return instructionPointerAdjustAccumulator(register); }
  case 0x15:
    return undefined();
  case 0x16:
    if constexpr(bits != 16) return undefined(); else {
    return instructionMirror(register); }
  case 0x17: case 0x18:
    return undefined();
  case 0x19:
    if constexpr(bits != 16) return undefined(); else {
    idle(17);
    return instructionMultiplyAdd(register); }
  case 0x1a: case 0x1b:
    return undefined();
  case 0x1c:
    if constexpr(bits == 32) return undefined(); else {
    return instructionDecrementJumpNotZero(register, fetchImmediate<int8>()); }
  case 0x1d: case 0x1e: case 0x1f:
    return undefined();
  case 0x20:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionAndCarry(register, fetchImmediate<uint8>()); }
  case 0x21:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionOrCarry(register, fetchImmediate<uint8>()); }
  case 0x22:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionXorCarry(register, fetchImmediate<uint8>()); }
  case 0x23:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionLoadCarry(register, fetchImmediate<uint8>()); }
  case 0x24:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionStoreCarry(register, fetchImmediate<uint8>()); }
  case 0x25: case 0x26: case 0x27:
    return undefined();
  case 0x28:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionAndCarry(register, A); }
  case 0x29:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionOrCarry(register, A); }
  case 0x2a:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionXorCarry(register, A); }
  case 0x2b:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionLoadCarry(register, A); }
  case 0x2c:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionStoreCarry(register, A); }
  case 0x2d:
    return undefined();
  case 0x2e:
    data = fetch();
    idle(1);
    return instructionLoad(toControlRegister<T>(data), register);
  case 0x2f:
    data = fetch();
    idle(1);
    return instructionLoad(register, toControlRegister<T>(data));
  case 0x30:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionReset(register, fetchImmediate<uint8>()); }
  case 0x31:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionSet(register, fetchImmediate<uint8>()); }
  case 0x32:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionChange(register, fetchImmediate<uint8>()); }
  case 0x33:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionBit(register, fetchImmediate<uint8>()); }
  case 0x34:
    if constexpr(bits == 32) return undefined(); else {
    idle(2);
    return instructionTestSet(register, fetchImmediate<uint8>()); }
  case 0x35: case 0x36: case 0x37:
    return undefined();
  case 0x38:
    if constexpr(bits != 16) return undefined(); else {
    idle(3);
    return instructionModuloIncrement<1>(register, fetchImmediate<uint16>()); }
  case 0x39:
    if constexpr(bits != 16) return undefined(); else {
    idle(3);
    return instructionModuloIncrement<2>(register, fetchImmediate<uint16>()); }
  case 0x3a:
    if constexpr(bits != 16) return undefined(); else {
    idle(3);
    return instructionModuloIncrement<4>(register, fetchImmediate<uint16>()); }
  case 0x3b:
    return undefined();
  case 0x3c:
    if constexpr(bits != 16) return undefined(); else {
    idle(2);
    return instructionModuloDecrement<1>(register, fetchImmediate<uint16>()); }
  case 0x3d:
    if constexpr(bits != 16) return undefined(); else {
    idle(2);
    return instructionModuloDecrement<2>(register, fetchImmediate<uint16>()); }
  case 0x3e:
    if constexpr(bits != 16) return undefined(); else {
    idle(2);
    return instructionModuloDecrement<4>(register, fetchImmediate<uint16>()); }
  case 0x3f:
    return undefined();
  case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(9, 12);
    return instructionMultiply(toRegister3<T>(data), register); }
  case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(7, 10);
    return instructionMultiplySigned(toRegister3<T>(data), register); }
  case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(13, 21);
    return instructionDivide(toRegister3<T>(data), register); }
  case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(16, 24);
    return instructionDivideSigned(toRegister3<T>(data), register); }
  case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    idle(1);
    return instructionIncrement(register, toImmediate<T>((uint3)data));
  case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
    idle(1);
    return instructionDecrement(register, toImmediate<T>((uint3)data));
  case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
  case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
    if constexpr(bits == 32) return undefined(); else {
    return instructionSetConditionCode((uint4)data, register); }
  case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    return instructionAdd(toRegister3<T>(data), register);
  case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    return instructionLoad(toRegister3<T>(data), register);
  case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    return instructionAddCarry(toRegister3<T>(data), register);
  case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
    return instructionLoad(register, toRegister3<T>(data));
  case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    return instructionSubtract(toRegister3<T>(data), register);
  case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    return instructionLoad(register, toImmediate<T>((uint3)data));
  case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    return instructionSubtractBorrow(toRegister3<T>(data), register);
  case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
    if constexpr(bits == 32) return undefined(); else {
    idle(1);
    return instructionExchange(toRegister3<T>(data), register); }
  case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    return instructionAnd(toRegister3<T>(data), register);
  case 0xc8:
    idleBWL<bits>(1, 2, 4);
    return instructionAdd(register, fetchImmediate<T>());
  case 0xc9:
    idleBWL<bits>(1, 2, 4);
    return instructionAddCarry(register, fetchImmediate<T>());
  case 0xca:
    idleBWL<bits>(1, 2, 4);
    return instructionSubtract(register, fetchImmediate<T>());
  case 0xcb:
    idleBWL<bits>(1, 2, 4);
    return instructionSubtractBorrow(register, fetchImmediate<T>());
  case 0xcc:
    idleBWL<bits>(1, 2, 4);
    return instructionAnd(register, fetchImmediate<T>());
  case 0xcd:
    idleBWL<bits>(1, 2, 4);
    return instructionXor(register, fetchImmediate<T>());
  case 0xce:
    idleBWL<bits>(1, 2, 4);
    return instructionOr(register, fetchImmediate<T>());
  case 0xcf:
    idleBWL<bits>(1, 2, 4);
    return instructionCompare(register, fetchImmediate<T>());
  case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
    return instructionXor(toRegister3<T>(data), register);
  case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
    if constexpr(bits == 32) return undefined(); else {
    return instructionCompare(register, toImmediate<T>((uint3)data)); }
  case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    return instructionOr(toRegister3<T>(data), register);
  case 0xe8:
    idle(1);
    return instructionRotateLeftWithoutCarry(register, fetchImmediate<uint8>());
  case 0xe9:
    idle(1);
    return instructionRotateRightWithoutCarry(register, fetchImmediate<uint8>());
  case 0xea:
    idle(1);
    return instructionRotateLeft(register, fetchImmediate<uint8>());
  case 0xeb:
    idle(1);
    return instructionRotateRight(register, fetchImmediate<uint8>());
  case 0xec:
    idle(1);
    return instructionShiftLeftArithmetic(register, fetchImmediate<uint8>());
  case 0xed:
    idle(1);
    return instructionShiftRightArithmetic(register, fetchImmediate<uint8>());
  case 0xee:
    idle(1);
    return instructionShiftLeftLogical(register, fetchImmediate<uint8>());
  case 0xef:
    idle(1);
    return instructionShiftRightLogical(register, fetchImmediate<uint8>());
  case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
    return instructionCompare(toRegister3<T>(data), register);
  case 0xf8:
    idle(1);
    return instructionRotateLeftWithoutCarry(register, A);
  case 0xf9:
    idle(1);
    return instructionRotateRightWithoutCarry(register, A);
  case 0xfa:
    idle(1);
    return instructionRotateLeft(register, A);
  case 0xfb:
    idle(1);
    return instructionRotateRight(register, A);
  case 0xfc:
    idle(1);
    return instructionShiftLeftArithmetic(register, A);
  case 0xfd:
    idle(1);
    return instructionShiftRightArithmetic(register, A);
  case 0xfe:
    idle(1);
    return instructionShiftLeftLogical(register, A);
  case 0xff:
    idle(1);
    return instructionShiftRightLogical(register, A);
  }
}

template<typename M>
auto TLCS900H::instructionSourceMemory(M memory) -> void {
  using T = typename M::type;
  enum : uint { bits = M::bits };
  auto data = fetch();

  switch(data) {
  case 0x00: case 0x01: case 0x02: case 0x03:
    return undefined();
  case 0x04:
    if constexpr(bits == 32) return undefined(); else {
    return instructionPush(memory); }
  case 0x05:
    return undefined();
  case 0x06:
    if constexpr(bits != 8) return undefined(); else {
    idle(12);
    return instructionRotateLeftDigit(A, memory); }
  case 0x07:
    if constexpr(bits != 8) return undefined(); else {
    idle(12);
    return instructionRotateRightDigit(A, memory); }
  case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
    return undefined();
  case 0x10:
    if constexpr(bits ==  8) { idle(6); return instructionLoad<T, +1>(); }
    if constexpr(bits == 16) { idle(6); return instructionLoad<T, +2>(); }
    return undefined();
  case 0x11:
    if constexpr(bits ==  8) { idle(5); return instructionLoadRepeat<T, +1>(); }
    if constexpr(bits == 16) { idle(5); return instructionLoadRepeat<T, +2>(); }
    return undefined();
  case 0x12:
    if constexpr(bits ==  8) { idle(6); return instructionLoad<T, -1>(); }
    if constexpr(bits == 16) { idle(6); return instructionLoad<T, -2>(); }
    return undefined();
  case 0x13:
    if constexpr(bits ==  8) { idle(5); return instructionLoadRepeat<T, -1>(); }
    if constexpr(bits == 16) { idle(5); return instructionLoadRepeat<T, -2>(); }
    return undefined();
  case 0x14:
    if constexpr(bits ==  8) { idle(4); return instructionCompare<T, +1>( A); }
    if constexpr(bits == 16) { idle(4); return instructionCompare<T, +2>(WA); }
    return undefined();
  case 0x15:
    if constexpr(bits ==  8) { idle(4); return instructionCompareRepeat<T, +1>( A); }
    if constexpr(bits == 16) { idle(4); return instructionCompareRepeat<T, +2>(WA); }
    return undefined();
  case 0x16:
    if constexpr(bits ==  8) { idle(4); return instructionCompare<T, -1>( A); }
    if constexpr(bits == 16) { idle(4); return instructionCompare<T, -2>(WA); }
    return undefined();
  case 0x17:
    if constexpr(bits ==  8) { idle(4); return instructionCompareRepeat<T, -1>( A); }
    if constexpr(bits == 16) { idle(4); return instructionCompareRepeat<T, -2>(WA); }
    return undefined();
  case 0x18:
    return undefined();
  case 0x19:
    if constexpr(bits == 32) return undefined(); else {
    idle(6);
    return instructionLoad(fetchMemory<T, uint16>(), memory); }
  case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
    return undefined();
  case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    return instructionLoad(toRegister3<T>(data), memory);
  case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
    return undefined();
  case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionExchange(memory, toRegister3<T>(data)); }
  case 0x38:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionAdd(memory, fetchImmediate<T>()); }
  case 0x39:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionAddCarry(memory, fetchImmediate<T>()); }
  case 0x3a:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionSubtract(memory, fetchImmediate<T>()); }
  case 0x3b:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionSubtractBorrow(memory, fetchImmediate<T>()); }
  case 0x3c:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionAnd(memory, fetchImmediate<T>()); }
  case 0x3d:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionXor(memory, fetchImmediate<T>()); }
  case 0x3e:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(5, 6);
    return instructionOr(memory, fetchImmediate<T>()); }
  case 0x3f:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(3, 4);
    return instructionCompare(memory, fetchImmediate<T>()); }
  case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(11, 14);
    return instructionMultiply(toRegister3<T>(data), memory); }
  case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(9, 12);
    return instructionMultiplySigned(toRegister3<T>(data), memory); }
  case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(14, 22);
    return instructionDivide(toRegister3<T>(data), memory); }
  case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    if constexpr(bits == 32) return undefined(); else {
    idleBW<bits>(17, 25);
    return instructionDivideSigned(toRegister3<T>(data), memory); }
  case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionIncrement(memory, toImmediate<T>((uint3)data)); }
  case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionDecrement(memory, toImmediate<T>((uint3)data)); }
  case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    return undefined();
  case 0x78:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionRotateLeftWithoutCarry(memory, toImmediate<uint4>(1)); }
  case 0x79:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionRotateRightWithoutCarry(memory, toImmediate<uint4>(1)); }
  case 0x7a:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionRotateLeft(memory, toImmediate<uint4>(1)); }
  case 0x7b:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionRotateRight(memory, toImmediate<uint4>(1)); }
  case 0x7c:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionShiftLeftArithmetic(memory, toImmediate<uint4>(1)); }
  case 0x7d:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionShiftRightArithmetic(memory, toImmediate<uint4>(1)); }
  case 0x7e:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionShiftLeftLogical(memory, toImmediate<uint4>(1)); }
  case 0x7f:
    if constexpr(bits == 32) return undefined(); else {
    idle(4);
    return instructionShiftRightLogical(memory, toImmediate<uint4>(1)); }
  case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    idleBWL<bits>(2, 2, 4);
    return instructionAdd(toRegister3<T>(data), memory);
  case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    idleBWL<bits>(4, 4, 8);
    return instructionAdd(memory, toRegister3<T>(data));
  case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    idleBWL<bits>(2, 2, 4);
    return instructionAddCarry(toRegister3<T>(data), memory);
  case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
    idleBWL<bits>(4, 4, 8);
    return instructionAddCarry(memory, toRegister3<T>(data));
  case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    idleBWL<bits>(2, 2, 4);
    return instructionSubtract(toRegister3<T>(data), memory);
  case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    idleBWL<bits>(4, 4, 8);
    return instructionSubtract(memory, toRegister3<T>(data));
  case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    idleBWL<bits>(2, 2, 4);
    return instructionSubtractBorrow(toRegister3<T>(data), memory);
  case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
    idleBWL<bits>(4, 4, 8);
    return instructionSubtractBorrow(memory, toRegister3<T>(data));
  case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    idleBWL<bits>(2, 2, 4);
    return instructionAnd(toRegister3<T>(data), memory);
  case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
    idleBWL<bits>(4, 4, 8);
    return instructionAnd(memory, toRegister3<T>(data));
  case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
    idleBWL<bits>(2, 2, 4);
    return instructionXor(toRegister3<T>(data), memory);
  case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
    idleBWL<bits>(4, 4, 8);
    return instructionXor(memory, toRegister3<T>(data));
  case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
    idleBWL<bits>(2, 2, 4);
    return instructionOr(toRegister3<T>(data), memory);
  case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
    idleBWL<bits>(4, 4, 8);
    return instructionOr(memory, toRegister3<T>(data));
  case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
    idleBWL<bits>(2, 2, 4);
    return instructionCompare(toRegister3<T>(data), memory);
  case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
    idleBWL<bits>(2, 2, 4);
    return instructionCompare(memory, toRegister3<T>(data));
  }
}

auto TLCS900H::instructionTargetMemory(uint32 address) -> void {
  auto data = fetch();

  switch(data) {
  case 0x00:
    idle(1);
    return instructionLoad(toMemory<uint8>(address), fetchImmediate<uint8>());
  case 0x01:
    return undefined();
  case 0x02:
    idle(2);
    return instructionLoad(toMemory<uint16>(address), fetchImmediate<uint16>());
  case 0x03:
    return undefined();
  case 0x04:
    idle(1);
    return instructionPop(toMemory<uint8>(address));
  case 0x05:
    return undefined();
  case 0x06:
    idle(1);
    return instructionPop(toMemory<uint16>(address));
  case 0x07:
    return undefined();
  case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f:
    return undefined();
  case 0x10: case 0x11: case 0x12: case 0x13:
    return undefined();
  case 0x14:
    idle(4);
    return instructionLoad(toMemory<uint8>(address), fetchMemory<uint8, uint16>());
  case 0x15:
    return undefined();
  case 0x16:
    idle(4);
    return instructionLoad(toMemory<uint16>(address), fetchMemory<uint16, uint16>());
  case 0x17:
    return undefined();
  case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f:
    return undefined();
  case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
    idle(2);
    return instructionLoad(toRegister3<uint16>(data), toImmediate<uint16>(address));
  case 0x28:
    idle(1);
    return instructionAndCarry(toMemory<uint8>(address), A);
  case 0x29:
    idle(1);
    return instructionOrCarry(toMemory<uint8>(address), A);
  case 0x2a:
    idle(1);
    return instructionXorCarry(toMemory<uint8>(address), A);
  case 0x2b:
    idle(1);
    return instructionLoadCarry(toMemory<uint8>(address), A);
  case 0x2c:
    idle(2);
    return instructionStoreCarry(toMemory<uint8>(address), A);
  case 0x2d: case 0x2e: case 0x2f:
    return undefined();
  case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
    idle(2);
    return instructionLoad(toRegister3<uint32>(data), toImmediate<uint32>(address));
  case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
    return undefined();
  case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    idle(2);
    return instructionLoad(toMemory<uint8>(address), toRegister3<uint8>(data));
  case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
    return undefined();
  case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
    idle(2);
    return instructionLoad(toMemory<uint16>(address), toRegister3<uint16>(data));
  case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
    return undefined();
  case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
    idle(4);
    return instructionLoad(toMemory<uint32>(address), toRegister3<uint32>(data));
  case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f:
    return undefined();
  case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
    return undefined();
  case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
    return undefined();
  case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
    idle(4);
    return instructionAndCarry(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:
    idle(4);
    return instructionOrCarry(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
    idle(4);
    return instructionXorCarry(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:
    idle(4);
    return instructionLoadCarry(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
    idle(5);
    return instructionStoreCarry(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:
    idle(5);
    return instructionTestSet(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
    idle(5);
    return instructionReset(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
    idle(5);
    return instructionSet(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
    idle(5);
    return instructionChange(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
    idle(4);
    return instructionBit(toMemory<uint8>(address), toImmediate<uint3>(data));
  case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
  case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
    if(!condition((uint4)data)) return;
    return instructionJump(toImmediate<uint32>(address));
  case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
  case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:
    if(!condition((uint4)data)) return;
    idle(3);
    return instructionCall(toImmediate<uint32>(address));
  case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
  case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
    if(!condition((uint4)data)) return;
    idle(2);
    return instructionReturn();
  }
}
