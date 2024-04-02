auto V30MZ::instructionLoop() -> void {
  wait(1);
  auto offset = (i8)fetch<Byte>();
  if(--CW) {
    wait(3);
    PC += offset;
    flush();
  }
}

auto V30MZ::instructionLoopWhile(bool value) -> void {
  wait(2);
  auto offset = (i8)fetch<Byte>();
  if(--CW && PSW.Z == value) {
    wait(3);
    PC += offset;
    flush();
  }
}

auto V30MZ::instructionJumpShort() -> void {
  wait(3);
  auto offset = (i8)fetch<Byte>();
  PC += offset;
  flush();
}

auto V30MZ::instructionJumpIf(bool condition) -> void {
  wait(1);
  auto offset = (i8)fetch<Byte>();
  if(condition) {
    wait(2);
    PC += offset;
    flush();
  }
}

auto V30MZ::instructionJumpNear() -> void {
  wait(3);
  PC += (i16)fetch<Word>();
  flush();
}

auto V30MZ::instructionJumpFar() -> void {
  wait(6);
  auto pc = fetch<Word>();
  auto ps = fetch<Word>();
  PS = ps;
  PC = pc;
  flush();
}

auto V30MZ::instructionCallNear() -> void {
  wait(2);
  auto offset = (i16)fetch<Word>();
  push(PC);
  PC += offset;
  flush();
}

auto V30MZ::instructionCallFar() -> void {
  wait(6);
  auto pc = fetch<Word>();
  auto ps = fetch<Word>();
  push(PS);
  push(PC);
  PS = ps;
  PC = pc;
  flush();
}

auto V30MZ::instructionReturn() -> void {
  wait(4);
  PC = pop();
  flush();
}

auto V30MZ::instructionReturnImm() -> void {
  wait(4);
  auto offset = fetch<Word>();
  PC = pop();
  SP += offset;
  flush();
}

auto V30MZ::instructionReturnFar() -> void {
  wait(5);
  PC = pop();
  PS = pop();
  flush();
}

auto V30MZ::instructionReturnFarImm() -> void {
  wait(6);
  auto offset = fetch<Word>();
  PC = pop();
  PS = pop();
  SP += offset;
  flush();
}

auto V30MZ::instructionReturnInt() -> void {
  wait(6);
  PC = pop();
  PS = pop();
  PSW = pop();
  flush();
  state.poll = 0;
}

auto V30MZ::instructionInt3() -> void {
  wait(9);
  interrupt(3, InterruptSource::CPU);
}

auto V30MZ::instructionIntImm() -> void {
  wait(10);
  interrupt(fetch<Byte>(), InterruptSource::CPU);
}

auto V30MZ::instructionInto() -> void {
  wait(6);
  if(PSW.V) {
    wait(7);
    interrupt(4, InterruptSource::CPU);
  }
}

auto V30MZ::instructionEnter() -> void {
  wait(7);
  auto offset = fetch<Word>();
  auto length = fetch<Byte>() & 0x1f;
  push(BP);
  u16 bp = SP;

  if(length) {
    wait(length > 1 ? 7 : 6);
    for(auto index = 1; index < length; index++) {
      wait(2);
      auto data = read<Word>(segment(SS), BP - index * 2);
      push(data);
    }
    push(bp);
  }

  BP = bp;
  SP -= offset;
}

auto V30MZ::instructionLeave() -> void {
  wait(2);
  SP = BP;
  BP = pop();
}

auto V30MZ::instructionPushReg(u16& reg) -> void {
  // Emulate the 8086/80186 "PUSH SP" bug.
  if(&reg == &SP) push(reg - 2);
  else push(reg);
}

auto V30MZ::instructionPushSeg(u16& seg) -> void {
  wait(1);
  push(seg);
}

auto V30MZ::instructionPopReg(u16& reg) -> void {
  reg = pop();
}

auto V30MZ::instructionPopSeg(u16& seg) -> void {
  wait(2);
  seg = pop();
  if(&seg == &SS) state.poll = 0;
}

auto V30MZ::instructionPushFlags() -> void {
  wait(1);
  push(PSW);
}

auto V30MZ::instructionPopFlags() -> void {
  wait(2);
  PSW = pop();
  state.poll = 0;
}

auto V30MZ::instructionPushAll() -> void {
  wait(1);
  auto sp = SP;
  push(AW);
  push(CW);
  push(DW);
  push(BW);
  push(sp);
  push(BP);
  push(IX);
  push(IY);
}

auto V30MZ::instructionPopAll() -> void {
  wait(1);
  IY = pop();
  IX = pop();
  BP = pop();
  auto sp = pop();
  BW = pop();
  DW = pop();
  CW = pop();
  AW = pop();
  //SP is not restored
}

template<u32 size> auto V30MZ::instructionPushImm() -> void {
  if constexpr(size == Byte) push((i8)fetch<Byte>());
  if constexpr(size == Word) push(fetch<Word>());
}

auto V30MZ::instructionPopMem() -> void {
  wait(1);
  modRM();
  auto data = pop();
  setMemory<Word>(data);
}
