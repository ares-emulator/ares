auto VR4300::instructionADD() -> void {
  if(RS32u > ~RT32u) return exception(Overflow);
  RDw(int32_t(RS32u + RT32u));
}

auto VR4300::instructionADDU() -> void {
  RDw(int32_t(RS32u + RT32u));
}

auto VR4300::instructionAND() -> void {
  if(!RDn) return;
  RD = RS64u & RT64u;
}

auto VR4300::instructionBREAK() -> void {
  exception(Break);
}

auto VR4300::instructionDADD() -> void {
  if(RS64u > ~RT64u) return exception(Overflow);
  RDw(RS64u + RT64u);
}

auto VR4300::instructionDADDU() -> void {
  RDw(RS64u + RT64u);
}

auto VR4300::instructionDDIV() -> void {
  if(!RT64u) return;
  LO = RS64i / RT64i;
  HI = RS64i % RT64i;
}

auto VR4300::instructionDDIVU() -> void {
  if(!RT64u) return;
  LO = RS64u / RT64u;
  HI = RS64u % RT64u;
}

auto VR4300::instructionDIV() -> void {
  if(!RT32u) return;
  LO = int32_t(RS32i / RT32i);
  HI = int32_t(RS32i % RT32i);
}

auto VR4300::instructionDIVU() -> void {
  if(!RT32u) return;
  LO = int32_t(RS32u / RT32u);
  HI = int32_t(RS32u / RT32u);
}

auto VR4300::instructionDMULT() -> void {
  auto result = RS128i * RT128i;
  LO = result >>  0;
  HI = result >> 64;
}

auto VR4300::instructionDMULTU() -> void {
  auto result = RS128u * RT128u;
  LO = result >>  0;
  HI = result >> 64;
}

auto VR4300::instructionDSLL() -> void {
  RDw(RT64u << SHIFT);
}

auto VR4300::instructionDSLL32() -> void {
  RDw(RT64u << SHIFT + 32);
}

auto VR4300::instructionDSLLV() -> void {
  RDw(RT64u << RS32u & 63);
}

auto VR4300::instructionDSRA() -> void {
  RDw(RT64i >> SHIFT);
}

auto VR4300::instructionDSRA32() -> void {
  RDw(RT64i >> SHIFT + 32);
}

auto VR4300::instructionDSRAV() -> void {
  RDw(RT64i >> RS32u & 63);
}

auto VR4300::instructionDSRL() -> void {
  RDw(RT64u >> SHIFT);
}

auto VR4300::instructionDSRL32() -> void {
  RDw(RT64u >> SHIFT + 32);
}

auto VR4300::instructionDSRLV() -> void {
  RDw(RT64u >> RS32u & 63);
}

auto VR4300::instructionDSUB() -> void {
  if(RS64u < RT64u) return exception(Overflow);
  RDw(RS64u - RT64u);
}

auto VR4300::instructionDSUBU() -> void {
  RDw(RS64u - RT64u);
}

auto VR4300::instructionJALR() -> void {
  //...
}

auto VR4300::instructionJR() -> void {
}

auto VR4300::instructionMFHI() -> void {
  RDw(HI64u);
}

auto VR4300::instructionMFLO() -> void {
  RDw(LO64u);
}

auto VR4300::instructionMTHI() -> void {
  HI = RS64u;
}

auto VR4300::instructionMTLO() -> void {
  LO = RS64u;
}

auto VR4300::instructionMULT() -> void {
  auto result = int64_t(RS32i) * int64_t(RT32i);
  LO = int32_t(result >>  0);
  HI = int32_t(result >> 32);
}

auto VR4300::instructionMULTU() -> void {
  auto result = uint64_t(RS32u) * uint64_t(RT32u);
  LO = int32_t(result >>  0);
  HI = int32_t(result >> 32);
}

auto VR4300::instructionNOR() -> void {
  RDw(~(RS64u | RT64u));
}

auto VR4300::instructionOR() -> void {
  RDw(RS64u | RT64u);
}

auto VR4300::instructionSLL() -> void {
  RDw(int32_t(RT32u << SHIFT));
}

auto VR4300::instructionSLLV() -> void {
  RDw(int32_t(RT32u << RS32u & 31));
}

auto VR4300::instructionSLT() -> void {
  RDw(RS64i < RT64i);
}

auto VR4300::instructionSLTU() -> void {
  RDw(RS64u < RT64u);
}

auto VR4300::instructionSRA() -> void {
  RDw(RT32i >> SHIFT);
}

auto VR4300::instructionSRAV() -> void {
  RDw(RT32i >> RS32u & 31);
}

auto VR4300::instructionSRL() -> void {
  RDw(int32_t(RT32u >> SHIFT));
}

auto VR4300::instructionSRLV() -> void {
  RDw(int32_t(RT32u >> RS32u & 31));
}

auto VR4300::instructionSUB() -> void {
  if(RS32u < RT32u) return exception(Overflow);
  RDw(int32_t(RS32u - RT32u));
}

auto VR4300::instructionSUBU() -> void {
  RDw(int32_t(RS32u - RT32u));
}

auto VR4300::instructionSYNC() -> void {
  //NOP on VR4300
  //this instruction exists for software compatibility with VR4400 code
}

auto VR4300::instructionSYSCALL() -> void {
  exception(Syscall);
}

auto VR4300::instructionTEQ() -> void {
  if(RS64u == RT64u) exception(Trap);
}

auto VR4300::instructionTGE() -> void {
  if(RS64i >= RT64i) exception(Trap);
}

auto VR4300::instructionTGEU() -> void {
  if(RS64u >= RT64u) exception(Trap);
}

auto VR4300::instructionTLT() -> void {
  if(RS64i < RT64i) exception(Trap);
}

auto VR4300::instructionTLTU() -> void {
  if(RS64u < RT64u) exception(Trap);
}

auto VR4300::instructionTNE() -> void {
  if(RS64u != RT64u) exception(Trap);
}

auto VR4300::instructionXOR() -> void {
  RDw(RS64u ^ RT64u);
}
