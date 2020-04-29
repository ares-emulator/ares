auto VR4300::instructionADD() -> void {
  if(RS32u > ~RT32u) return exception(Overflow);
  wRD32i(RS32u + RT32u);
}

auto VR4300::instructionADDU() -> void {
  wRD32i(RS32u + RT32u);
}

auto VR4300::instructionAND() -> void {
  wRD64u(RS64u & RT64u);
}

auto VR4300::instructionBREAK() -> void {
  exception(Break);
}

auto VR4300::instructionDADD() -> void {
  if(RS64u > ~RT64u) return exception(Overflow);
  wRD64u(RS64u + RT64u);
}

auto VR4300::instructionDADDU() -> void {
  wRD64u(RS64u + RT64u);
}

auto VR4300::instructionDDIV() -> void {
  if(!RT64u) return;
  wLO64u(RS64i / RT64i);
  wHI64u(RS64i % RT64i);
}

auto VR4300::instructionDDIVU() -> void {
  if(!RT64u) return;
  wLO64u(RS64u / RT64u);
  wHI64u(RS64u % RT64u);
}

auto VR4300::instructionDIV() -> void {
  if(!RT32u) return;
  wLO32i(RS32i / RT32i);
  wHI32i(RS32i % RT32i);
}

auto VR4300::instructionDIVU() -> void {
  if(!RT32u) return;
  wLO32i(RS32u / RT32u);
  wHI32i(RS32u % RT32u);
}

auto VR4300::instructionDMULT() -> void {
  auto result = RS128i * RT128i;
  wLO64u(result >>  0);
  wHI64u(result >> 64);
}

auto VR4300::instructionDMULTU() -> void {
  auto result = RS128u * RT128u;
  wLO64u(result >>  0);
  wHI64u(result >> 64);
}

auto VR4300::instructionDSLL() -> void {
  wRD64u(RT64u << SA);
}

auto VR4300::instructionDSLL32() -> void {
  wRD64u(RT64u << SA + 32);
}

auto VR4300::instructionDSLLV() -> void {
  wRD64u(RT64u << (RS32u & 63));
}

auto VR4300::instructionDSRA() -> void {
  wRD64u(RT64i >> SA);
}

auto VR4300::instructionDSRA32() -> void {
  wRD64u(RT64i >> SA + 32);
}

auto VR4300::instructionDSRAV() -> void {
  wRD64u(RT64i >> (RS32u & 63));
}

auto VR4300::instructionDSRL() -> void {
  wRD64u(RT64u >> SA);
}

auto VR4300::instructionDSRL32() -> void {
  wRD64u(RT64u >> SA + 32);
}

auto VR4300::instructionDSRLV() -> void {
  wRD64u(RT64u >> (RS32u & 63));
}

auto VR4300::instructionDSUB() -> void {
  if(RS64u < RT64u) return exception(Overflow);
  wRD64u(RS64u - RT64u);
}

auto VR4300::instructionDSUBU() -> void {
  wRD64u(RS64u - RT64u);
}

auto VR4300::instructionJALR() -> void {
  IP = RS32u;
  GPR[1] = int32_t(PC + 4);
}

auto VR4300::instructionJR() -> void {
  IP = RS32u;
}

auto VR4300::instructionMFHI() -> void {
  wRD64u(HI);
}

auto VR4300::instructionMFLO() -> void {
  wRD64u(LO);
}

auto VR4300::instructionMTHI() -> void {
  wHI64u(RS64u);
}

auto VR4300::instructionMTLO() -> void {
  wLO64u(RS64u);
}

auto VR4300::instructionMULT() -> void {
  auto result = int64_t(RS32i) * int64_t(RT32i);
  wLO32i(result >>  0);
  wHI32i(result >> 32);
}

auto VR4300::instructionMULTU() -> void {
  auto result = uint64_t(RS32u) * uint64_t(RT32u);
  wLO32i(result >>  0);
  wHI32i(result >> 32);
}

auto VR4300::instructionNOR() -> void {
  wRD64u(~(RS64u | RT64u));
}

auto VR4300::instructionOR() -> void {
  wRD64u(RS64u | RT64u);
}

auto VR4300::instructionSLL() -> void {
  wRD32i(RT32u << SA);
}

auto VR4300::instructionSLLV() -> void {
  wRD32i(RT32u << (RS32u & 31));
}

auto VR4300::instructionSLT() -> void {
  wRD64u(RS64i < RT64i);
}

auto VR4300::instructionSLTU() -> void {
  wRD64u(RS64u < RT64u);
}

auto VR4300::instructionSRA() -> void {
  wRD64u(RT32i >> SA);
}

auto VR4300::instructionSRAV() -> void {
  wRD64u(RT32i >> (RS32u & 31));
}

auto VR4300::instructionSRL() -> void {
  wRD32i(RT32u >> SA);
}

auto VR4300::instructionSRLV() -> void {
  wRD32i(RT32u >> (RS32u & 31));
}

auto VR4300::instructionSUB() -> void {
  if(RS32u < RT32u) return exception(Overflow);
  wRD32i(RS32u - RT32u);
}

auto VR4300::instructionSUBU() -> void {
  wRD32i(RS32u - RT32u);
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
  wRD64u(RS64u ^ RT64u);
}
