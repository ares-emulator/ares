//todo:
//* CACHE
//* LDC1, LDC2
//* LL, LLD: set COP0 LL address
//* LWC1, LWC2, LWC3
//* SC, SCD: test COP0 LL address
//* SDC1, SDC2
//* SWC1, SWC2, SWC3

auto VR4300::instructionADD() -> void {
  if(RS32u > ~RT32u) return exception(Overflow);
  wRD32i(RS32u + RT32u);
}

auto VR4300::instructionADDI() -> void {
  if(RS32i > ~IMM16i) return exception(Overflow);
  wRT32i(RS32u + IMM16i);
}

auto VR4300::instructionADDIU() -> void {
  wRT32i(RS32u + IMM16i);
}

auto VR4300::instructionADDU() -> void {
  wRD32i(RS32u + RT32u);
}

auto VR4300::instructionAND() -> void {
  wRD64u(RS64u & RT64u);
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

auto VR4300::instructionBGTZ() -> void {
  if(RS64i > 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBGTZL() -> void {
  if(RS64i > 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBLEZ() -> void {
  if(RS64i <= 0) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBLEZL() -> void {
  if(RS64i <= 0) IP = PC + (IMM16i << 2);
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

auto VR4300::instructionBNE() -> void {
  if(RS64u != RT64u) IP = PC + (IMM16i << 2);
}

auto VR4300::instructionBNEL() -> void {
  if(RS64u != RT64u) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto VR4300::instructionBREAK() -> void {
  exception(Break);
}

auto VR4300::instructionCACHE() -> void {
  //todo
}

auto VR4300::instructionDADD() -> void {
  if(RS64u > ~RT64u) return exception(Overflow);
  wRD64u(RS64u + RT64u);
}

auto VR4300::instructionDADDI() -> void {
  if(RS64i > ~IMM16i) return exception(Overflow);
  wRT64u(RS64u + int64_t(IMM16i));
}

auto VR4300::instructionDADDIU() -> void {
  wRT64u(RS64u + uint64_t(IMM16i));
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

auto VR4300::instructionJ() -> void {
  IP = (PC & 0xf0000000) | (IMM26u << 2);
}

auto VR4300::instructionJAL() -> void {
  IP = (PC & 0xf0000000) | (IMM26u << 2);
  GPR[1] = int32_t(PC + 4);
}

auto VR4300::instructionJALR() -> void {
  IP = RS32u;
  GPR[1] = int32_t(PC + 4);
}

auto VR4300::instructionJR() -> void {
  IP = RS32u;
}

auto VR4300::instructionLB() -> void {
  if(auto data = readByte(RS32u + IMM16i)) wRT64u(int8_t(*data));
}

auto VR4300::instructionLBU() -> void {
  if(auto data = readByte(RS32u + IMM16i)) wRT64u(uint8_t(*data));
}

auto VR4300::instructionLD() -> void {
  if(auto data = readDouble(RS32u + IMM16i)) wRT64u(*data);
}

auto VR4300::instructionLDC1() -> void {
  //todo
}

auto VR4300::instructionLDC2() -> void {
  //todo
}

auto VR4300::instructionLDL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = uint64_t(0) - 1 << shift;
  if(auto data = readDouble(address & ~7, mask >> shift)) {
    wRT64u(RT64u & ~mask | *data << shift);
  }
}

auto VR4300::instructionLDR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = uint64_t(0) - 1 >> shift;
  if(auto data = readDouble(address & ~7, mask << shift)) {
    wRT64u(RT64u & ~mask | *data >> shift);
  }
}

auto VR4300::instructionLH() -> void {
  if(auto data = readHalf(RS32u + IMM16i)) wRT64u(int16_t(*data));
}

auto VR4300::instructionLHU() -> void {
  if(auto data = readHalf(RS32u + IMM16i)) wRT64u(uint16_t(*data));
}

auto VR4300::instructionLL() -> void {
  if(auto data = readWord(RS32u + IMM16i)) {
  //todo: set COP0 LL address
    wRT64u(*data);
    LLBIT = 1;
  }
}

auto VR4300::instructionLLD() -> void {
  if(auto data = readDouble(RS32u + IMM16i)) {
  //todo: set COP0 LL address
    wRT64u(*data);
    LLBIT = 1;
  }
}

auto VR4300::instructionLUI() -> void {
  wRT32i(IMM16u << 16);
}

auto VR4300::instructionLW() -> void {
  if(auto data = readWord(RS32u + IMM16i)) wRT64u(int32_t(*data));
}

auto VR4300::instructionLWC1() -> void {
  //todo
}

auto VR4300::instructionLWC2() -> void {
  //todo
}

auto VR4300::instructionLWC3() -> void {
  //todo
}

auto VR4300::instructionLWL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = uint32_t(0) - 1 << shift;
  if(auto data = readWord(address & ~3, mask >> shift)) {
    wRT32i(RT32u & ~mask | *data << shift);
  }
}

auto VR4300::instructionLWR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = uint32_t(0) - 1 >> shift;
  if(auto data = readWord(address & ~3, mask << shift)) {
    wRT32i(RT32u & ~mask | *data >> shift);
  }
}

auto VR4300::instructionLWU() -> void {
  if(auto data = readWord(RS32u + IMM16i)) wRT64u(uint32_t(*data));
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

auto VR4300::instructionORI() -> void {
  wRT64u(RS64u | IMM16u);
}

auto VR4300::instructionSB() -> void {
  writeByte(RS32u + IMM16i, RT32u);
}

auto VR4300::instructionSC() -> void {
  if(readWord(RS32u + IMM16i) && RTn && 0) {  //todo: check COP0 address as well
    writeWord(RS32u + IMM16i, RT32u);
    wRT64u(1);
  } else {
    wRT64u(0);
  }
}

auto VR4300::instructionSCD() -> void {
  if(readDouble(RS32u + IMM16i) && RTn && 0) {  //todo: check COP0 address as well
    writeDouble(RS32u + IMM16i, RT64u);
    wRT64u(1);
  } else {
    wRT64u(0);
  }
}

auto VR4300::instructionSD() -> void {
  writeDouble(RS32u + IMM16i, RT64u);
}

auto VR4300::instructionSDC1() -> void {
  //todo
}

auto VR4300::instructionSDC2() -> void {
  //todo
}

auto VR4300::instructionSDL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = uint64_t(0) - 1 >> shift;
  writeDouble(address & ~7, mask, RT64u >> shift);
}

auto VR4300::instructionSDR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = uint64_t(0) - 1 << shift;
  writeDouble(address & ~7, mask, RT64u << shift);
}

auto VR4300::instructionSH() -> void {
  writeHalf(RS32u + IMM16i, RT32u);
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

auto VR4300::instructionSLTI() -> void {
  wRT64u(RS64i < int64_t(IMM16i));
}

auto VR4300::instructionSLTIU() -> void {
  wRT64u(RS64u < uint64_t(IMM16u));
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

auto VR4300::instructionSW() -> void {
  writeWord(RS32u + IMM16i, RT32u);
}

auto VR4300::instructionSWC1() -> void {
  //todo
}

auto VR4300::instructionSWC2() -> void {
  //todo
}

auto VR4300::instructionSWC3() -> void {
  //todo
}

auto VR4300::instructionSWL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = uint32_t(0) - 1 >> shift;
  writeWord(address & ~3, mask, RT32u >> shift);
}

auto VR4300::instructionSWR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = uint32_t(0) - 1 << shift;
  writeWord(address & ~3, mask, RT32u << shift);
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

auto VR4300::instructionTEQI() -> void {
  if(RS64u == IMM16u) exception(Trap);
}

auto VR4300::instructionTGE() -> void {
  if(RS64i >= RT64i) exception(Trap);
}

auto VR4300::instructionTGEI() -> void {
  if(RS64i >= IMM16i) exception(Trap);
}

auto VR4300::instructionTGEIU() -> void {
  if(RS64u >= IMM16u) exception(Trap);
}

auto VR4300::instructionTGEU() -> void {
  if(RS64u >= RT64u) exception(Trap);
}

auto VR4300::instructionTLT() -> void {
  if(RS64i < RT64i) exception(Trap);
}

auto VR4300::instructionTLTI() -> void {
  if(RS64i < IMM16i) exception(Trap);
}

auto VR4300::instructionTLTIU() -> void {
  if(RS64u < IMM16u) exception(Trap);
}

auto VR4300::instructionTLTU() -> void {
  if(RS64u < RT64u) exception(Trap);
}

auto VR4300::instructionTNE() -> void {
  if(RS64u != RT64u) exception(Trap);
}

auto VR4300::instructionTNEI() -> void {
  if(RS64u != IMM16u) exception(Trap);
}

auto VR4300::instructionXOR() -> void {
  wRD64u(RS64u ^ RT64u);
}

auto VR4300::instructionXORI() -> void {
  wRT64u(RS64u ^ IMM16u);
}
