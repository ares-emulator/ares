auto VR4300::instructionADDI() -> void {
  if(RS32i > ~IMM16i) return exception(Overflow);
  wRT32i(RS32u + IMM16i);
}

auto VR4300::instructionADDIU() -> void {
  wRT32i(RS32u + IMM16i);
}

auto VR4300::instructionANDI() -> void {
  wRT64u(RS64u & IMM16u);
}

auto VR4300::instructionBEQ() -> void {
  if(RS64u == RT64u) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBEQL() -> void {
  if(RS64u == RT64u) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBLEZ() -> void {
  if(RS64i <= 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBLEZL() -> void {
  if(RS64i <= 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBNE() -> void {
  if(RS64u != RT64u) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBNEL() -> void {
  if(RS64u != RT64u) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBGTZ() -> void {
  if(RS64i > 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBGTZL() -> void {
  if(RS64i > 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionDADDI() -> void {
  if(RS64i > ~IMM16i) return exception(Overflow);
  wRT64u(RS64u + int64_t(IMM16i));
}

auto VR4300::instructionDADDIU() -> void {
  wRT64u(RS64u + uint64_t(IMM16i));
}

auto VR4300::instructionJ() -> void {
  IP = (PC & 0xf0000000) | (IMM26u << 2);
}

auto VR4300::instructionJAL() -> void {
  IP = (PC & 0xf0000000) | (IMM26u << 2);
  GPR[1] = int32_t(PC + 4);
}

auto VR4300::instructionLDL() -> void {
  auto address = RS32i + IMM16i;
  auto shift = 8 * (address & 7);
  auto mask = uint64_t(0) - 1 << shift;
  if(1) {
    //todo
  }
  if(0) {
    //todo
  }
}

auto VR4300::instructionLDR() -> void {
  auto address = RS32i + IMM16i;
  auto shift = 8 * (~address & 7);
  auto mask = uint64_t(0) - 1 >> shift;
  if(1) {
    //todo
  }
  if(0) {
    //todo
  }
}

auto VR4300::instructionLUI() -> void {
  wRT32i(IMM16u << 16);
}

auto VR4300::instructionORI() -> void {
  wRT64u(RS64u | IMM16u);
}

auto VR4300::instructionSLTI() -> void {
  wRT64u(RS64i < int64_t(IMM16i));
}

auto VR4300::instructionSLTIU() -> void {
  wRT64u(RS64u < uint64_t(IMM16u));
}

auto VR4300::instructionXORI() -> void {
  wRT64u(RS64u ^ IMM16u);
}
