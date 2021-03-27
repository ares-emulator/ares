auto SH2::exceptionHandler() -> void {
  if(!exceptions) return;
  if(inDelaySlot()) return;

  if(exceptions & ResetCold) {
    exceptions &= ~ResetCold;
    PC = busReadLong(0x00 * 4) + 4;
    SP = busReadLong(0x01 * 4);
  }

  if(exceptions & ResetWarm) {
    exceptions &= ~ResetWarm;
    PC = busReadLong(0x02 * 4) + 4;
    SP = busReadLong(0x03 * 4);
  }

  if constexpr(Accuracy::AddressErrors) {
    if(exceptions & AddressErrorCPU) {
      exceptions &= ~AddressErrorCPU;
      addressErrorCPU();
    }

    if(exceptions & AddressErrorDMA) {
      exceptions &= ~AddressErrorDMA;
      addressErrorDMA();
    }
  }
}

auto SH2::push(u32 data) -> void {
  SP -= 4;
  writeLong(SP, data);
}

auto SH2::interrupt(u8 level, u8 vector) -> void {
  push(SR);
  push(PC);
  jump(readLong(VBR + vector * 4) + 4);
  SR.I = level;
}

auto SH2::exception(u8 vector) -> void {
  push(SR);
  push(PC - 2);
  jump(readLong(VBR + vector * 4) + 4);
}

auto SH2::addressErrorCPU() -> void {
  static constexpr u8 vector = 0x09;
  SP &= ~3;  //not accurate; but prevents infinite recursion
  push(SR);
  push(PC);
  jump(readLong(VBR + vector * 4) + 4);
}

auto SH2::addressErrorDMA() -> void {
  static constexpr u8 vector = 0x0a;
  SP &= ~3;  //not accurate; but prevents infinite recursion
  push(SR);
  push(PC);
  jump(readLong(VBR + vector * 4) + 4);
}

auto SH2::illegalInstruction() -> void {
  if(inDelaySlot()) return illegalSlotInstruction();
  static constexpr u8 vector = 0x04;
  push(SR);
  push(PC);
  jump(readLong(VBR + vector * 4) + 4);
  debug(unusual, "[SH2] illegal instruction: 0x", hex(busReadWord(PC - 4), 4L));
}

auto SH2::illegalSlotInstruction() -> void {
  static constexpr u8 vector = 0x06;
  push(SR);
  push(PC - 2);
  jump(readLong(VBR + vector * 4) + 4);
  debug(unusual, "[SH2] illegal slot instruction: 0x", hex(busReadWord(PC - 4), 4L));
}
