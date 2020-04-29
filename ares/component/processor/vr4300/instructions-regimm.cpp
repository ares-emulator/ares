auto VR4300::instructionBGEZ() -> void {
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBGEZAL() -> void {
  wLR32i(PC + 4);
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBGEZALL() -> void {
  wLR32i(PC + 4);
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBGEZL() -> void {
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBLTZ() -> void {
  if(RS64i < 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBLTZAL() -> void {
  wLR32i(PC + 4);
  if(RS64i < 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBLTZALL() -> void {
  wLR32i(PC + 4);
  if(RS64i < 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBLTZL() -> void {
  if(RS64i < 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionTEQI() -> void {
  if(RS64u == IMM16u) exception(Trap);
}

auto VR4300::instructionTGEI() -> void {
  if(RS64i >= IMM16i) exception(Trap);
}

auto VR4300::instructionTGEIU() -> void {
  if(RS64u >= IMM16u) exception(Trap);
}

auto VR4300::instructionTLTI() -> void {
  if(RS64i < IMM16i) exception(Trap);
}

auto VR4300::instructionTLTIU() -> void {
  if(RS64u < IMM16u) exception(Trap);
}

auto VR4300::instructionTNEI() -> void {
  if(RS64u != IMM16u) exception(Trap);
}
