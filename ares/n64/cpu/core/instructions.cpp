//todo:
//* CACHE
//* LDC1, LDC2
//* LL, LLD: set COP0 LL address
//* LWC1, LWC2, LWC3
//* SC, SCD: test COP0 LL address
//* SDC1, SDC2
//* SWC1, SWC2, SWC3

auto CPU::instructionADD() -> void {
  if(RS32u > ~RT32u) return exception(Overflow);
  wRD32i(RS32u + RT32u);
}

auto CPU::instructionADDI() -> void {
  if(RS32i > ~IMM16i) return exception(Overflow);
  wRT32i(RS32u + IMM16i);
}

auto CPU::instructionADDIU() -> void {
  wRT32i(RS32u + IMM16i);
}

auto CPU::instructionADDU() -> void {
  wRD32i(RS32u + RT32u);
}

auto CPU::instructionAND() -> void {
  wRD64u(RS64u & RT64u);
}

auto CPU::instructionANDI() -> void {
  wRT64u(RS64u & IMM16u);
}

auto CPU::instructionBEQ() -> void {
  if(RS64u == RT64u) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBEQL() -> void {
  if(RS64u == RT64u) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBGEZ() -> void {
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBGEZAL() -> void {
  wLR32i(PC + 4);
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBGEZALL() -> void {
  wLR32i(PC + 4);
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBGEZL() -> void {
  if(RS64i >= 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBGTZ() -> void {
  if(RS64i > 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBGTZL() -> void {
  if(RS64i > 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBLEZ() -> void {
  if(RS64i <= 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBLEZL() -> void {
  if(RS64i <= 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBLTZ() -> void {
  if(RS64i < 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBLTZAL() -> void {
  wLR32i(PC + 4);
  if(RS64i < 0) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBLTZALL() -> void {
  wLR32i(PC + 4);
  if(RS64i < 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBLTZL() -> void {
  if(RS64i < 0) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBNE() -> void {
  if(RS64u != RT64u) IP = PC + (IMM16i << 2);
}

auto CPU::instructionBNEL() -> void {
  if(RS64u != RT64u) IP = PC + (IMM16i << 2);
  else PC += 4;
}

auto CPU::instructionBREAK() -> void {
  exception(Break);
}

auto CPU::instructionCACHE() -> void {
  //todo
}

auto CPU::instructionDADD() -> void {
  if(RS64u > ~RT64u) return exception(Overflow);
  wRD64u(RS64u + RT64u);
}

auto CPU::instructionDADDI() -> void {
  if(RS64i > ~IMM16i) return exception(Overflow);
  wRT64u(RS64u + int64_t(IMM16i));
}

auto CPU::instructionDADDIU() -> void {
  wRT64u(RS64u + uint64_t(IMM16i));
}

auto CPU::instructionDADDU() -> void {
  wRD64u(RS64u + RT64u);
}

auto CPU::instructionDDIV() -> void {
  if(!RT64u) return;
  wLO64u(RS64i / RT64i);
  wHI64u(RS64i % RT64i);
}

auto CPU::instructionDDIVU() -> void {
  if(!RT64u) return;
  wLO64u(RS64u / RT64u);
  wHI64u(RS64u % RT64u);
}

auto CPU::instructionDIV() -> void {
  if(!RT32u) return;
  wLO32i(RS32i / RT32i);
  wHI32i(RS32i % RT32i);
}

auto CPU::instructionDIVU() -> void {
  if(!RT32u) return;
  wLO32i(RS32u / RT32u);
  wHI32i(RS32u % RT32u);
}

auto CPU::instructionDMULT() -> void {
  auto result = RS128i * RT128i;
  wLO64u(result >>  0);
  wHI64u(result >> 64);
}

auto CPU::instructionDMULTU() -> void {
  auto result = RS128u * RT128u;
  wLO64u(result >>  0);
  wHI64u(result >> 64);
}

auto CPU::instructionDSLL() -> void {
  wRD64u(RT64u << SA);
}

auto CPU::instructionDSLL32() -> void {
  wRD64u(RT64u << SA + 32);
}

auto CPU::instructionDSLLV() -> void {
  wRD64u(RT64u << (RS32u & 63));
}

auto CPU::instructionDSRA() -> void {
  wRD64u(RT64i >> SA);
}

auto CPU::instructionDSRA32() -> void {
  wRD64u(RT64i >> SA + 32);
}

auto CPU::instructionDSRAV() -> void {
  wRD64u(RT64i >> (RS32u & 63));
}

auto CPU::instructionDSRL() -> void {
  wRD64u(RT64u >> SA);
}

auto CPU::instructionDSRL32() -> void {
  wRD64u(RT64u >> SA + 32);
}

auto CPU::instructionDSRLV() -> void {
  wRD64u(RT64u >> (RS32u & 63));
}

auto CPU::instructionDSUB() -> void {
  if(RS64u < RT64u) return exception(Overflow);
  wRD64u(RS64u - RT64u);
}

auto CPU::instructionDSUBU() -> void {
  wRD64u(RS64u - RT64u);
}

auto CPU::instructionJ() -> void {
  IP = (PC & 0xf0000000) | (IMM26u << 2);
}

auto CPU::instructionJAL() -> void {
  IP = (PC & 0xf0000000) | (IMM26u << 2);
  GPR[1] = int32_t(PC + 4);
}

auto CPU::instructionJALR() -> void {
  IP = RS32u;
  GPR[1] = int32_t(PC + 4);
}

auto CPU::instructionJR() -> void {
  IP = RS32u;
}

auto CPU::instructionLB() -> void {
  if(auto data = readByte(RS32u + IMM16i)) wRT64u(int8_t(*data));
}

auto CPU::instructionLBU() -> void {
  if(auto data = readByte(RS32u + IMM16i)) wRT64u(uint8_t(*data));
}

auto CPU::instructionLD() -> void {
  if(auto data = readDouble(RS32u + IMM16i)) wRT64u(*data);
}

auto CPU::instructionLDC1() -> void {
  //todo
}

auto CPU::instructionLDC2() -> void {
  //todo
}

auto CPU::instructionLDL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = uint64_t(0) - 1 << shift;
  if(auto data = readDouble(address & ~7, mask >> shift)) {
    wRT64u(RT64u & ~mask | *data << shift);
  }
}

auto CPU::instructionLDR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = uint64_t(0) - 1 >> shift;
  if(auto data = readDouble(address & ~7, mask << shift)) {
    wRT64u(RT64u & ~mask | *data >> shift);
  }
}

auto CPU::instructionLH() -> void {
  if(auto data = readHalf(RS32u + IMM16i)) wRT64u(int16_t(*data));
}

auto CPU::instructionLHU() -> void {
  if(auto data = readHalf(RS32u + IMM16i)) wRT64u(uint16_t(*data));
}

auto CPU::instructionLL() -> void {
  if(auto data = readWord(RS32u + IMM16i)) {
  //todo: set COP0 LL address
    wRT64u(*data);
    LLBIT = 1;
  }
}

auto CPU::instructionLLD() -> void {
  if(auto data = readDouble(RS32u + IMM16i)) {
  //todo: set COP0 LL address
    wRT64u(*data);
    LLBIT = 1;
  }
}

auto CPU::instructionLUI() -> void {
  wRT32i(IMM16u << 16);
}

auto CPU::instructionLW() -> void {
  if(auto data = readWord(RS32u + IMM16i)) wRT64u(int32_t(*data));
}

auto CPU::instructionLWC1() -> void {
  //todo
}

auto CPU::instructionLWC2() -> void {
  //todo
}

auto CPU::instructionLWC3() -> void {
  //todo
}

auto CPU::instructionLWL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = uint32_t(0) - 1 << shift;
  if(auto data = readWord(address & ~3, mask >> shift)) {
    wRT32i(RT32u & ~mask | *data << shift);
  }
}

auto CPU::instructionLWR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = uint32_t(0) - 1 >> shift;
  if(auto data = readWord(address & ~3, mask << shift)) {
    wRT32i(RT32u & ~mask | *data >> shift);
  }
}

auto CPU::instructionLWU() -> void {
  if(auto data = readWord(RS32u + IMM16i)) wRT64u(uint32_t(*data));
}

auto CPU::instructionMFHI() -> void {
  wRD64u(HI);
}

auto CPU::instructionMFLO() -> void {
  wRD64u(LO);
}

auto CPU::instructionMTHI() -> void {
  wHI64u(RS64u);
}

auto CPU::instructionMTLO() -> void {
  wLO64u(RS64u);
}

auto CPU::instructionMULT() -> void {
  auto result = int64_t(RS32i) * int64_t(RT32i);
  wLO32i(result >>  0);
  wHI32i(result >> 32);
}

auto CPU::instructionMULTU() -> void {
  auto result = uint64_t(RS32u) * uint64_t(RT32u);
  wLO32i(result >>  0);
  wHI32i(result >> 32);
}

auto CPU::instructionNOR() -> void {
  wRD64u(~(RS64u | RT64u));
}

auto CPU::instructionOR() -> void {
  wRD64u(RS64u | RT64u);
}

auto CPU::instructionORI() -> void {
  wRT64u(RS64u | IMM16u);
}

auto CPU::instructionSB() -> void {
  writeByte(RS32u + IMM16i, RT32u);
}

auto CPU::instructionSC() -> void {
  if(readWord(RS32u + IMM16i) && RTn && 0) {  //todo: check COP0 address as well
    writeWord(RS32u + IMM16i, RT32u);
    wRT64u(1);
  } else {
    wRT64u(0);
  }
}

auto CPU::instructionSCD() -> void {
  if(readDouble(RS32u + IMM16i) && RTn && 0) {  //todo: check COP0 address as well
    writeDouble(RS32u + IMM16i, RT64u);
    wRT64u(1);
  } else {
    wRT64u(0);
  }
}

auto CPU::instructionSD() -> void {
  writeDouble(RS32u + IMM16i, RT64u);
}

auto CPU::instructionSDC1() -> void {
  //todo
}

auto CPU::instructionSDC2() -> void {
  //todo
}

auto CPU::instructionSDL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = uint64_t(0) - 1 >> shift;
  writeDouble(address & ~7, mask, RT64u >> shift);
}

auto CPU::instructionSDR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = uint64_t(0) - 1 << shift;
  writeDouble(address & ~7, mask, RT64u << shift);
}

auto CPU::instructionSH() -> void {
  writeHalf(RS32u + IMM16i, RT32u);
}

auto CPU::instructionSLL() -> void {
  wRD32i(RT32u << SA);
}

auto CPU::instructionSLLV() -> void {
  wRD32i(RT32u << (RS32u & 31));
}

auto CPU::instructionSLT() -> void {
  wRD64u(RS64i < RT64i);
}

auto CPU::instructionSLTI() -> void {
  wRT64u(RS64i < int64_t(IMM16i));
}

auto CPU::instructionSLTIU() -> void {
  wRT64u(RS64u < uint64_t(IMM16u));
}

auto CPU::instructionSLTU() -> void {
  wRD64u(RS64u < RT64u);
}

auto CPU::instructionSRA() -> void {
  wRD64u(RT32i >> SA);
}

auto CPU::instructionSRAV() -> void {
  wRD64u(RT32i >> (RS32u & 31));
}

auto CPU::instructionSRL() -> void {
  wRD32i(RT32u >> SA);
}

auto CPU::instructionSRLV() -> void {
  wRD32i(RT32u >> (RS32u & 31));
}

auto CPU::instructionSUB() -> void {
  if(RS32u < RT32u) return exception(Overflow);
  wRD32i(RS32u - RT32u);
}

auto CPU::instructionSUBU() -> void {
  wRD32i(RS32u - RT32u);
}

auto CPU::instructionSW() -> void {
  writeWord(RS32u + IMM16i, RT32u);
}

auto CPU::instructionSWC1() -> void {
  //todo
}

auto CPU::instructionSWC2() -> void {
  //todo
}

auto CPU::instructionSWC3() -> void {
  //todo
}

auto CPU::instructionSWL() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = uint32_t(0) - 1 >> shift;
  writeWord(address & ~3, mask, RT32u >> shift);
}

auto CPU::instructionSWR() -> void {
  auto address = RS32u + IMM16i;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = uint32_t(0) - 1 << shift;
  writeWord(address & ~3, mask, RT32u << shift);
}

auto CPU::instructionSYNC() -> void {
  //NOP on VR4300
  //this instruction exists for software compatibility with VR4400 code
}

auto CPU::instructionSYSCALL() -> void {
  exception(Syscall);
}

auto CPU::instructionTEQ() -> void {
  if(RS64u == RT64u) exception(Trap);
}

auto CPU::instructionTEQI() -> void {
  if(RS64u == IMM16u) exception(Trap);
}

auto CPU::instructionTGE() -> void {
  if(RS64i >= RT64i) exception(Trap);
}

auto CPU::instructionTGEI() -> void {
  if(RS64i >= IMM16i) exception(Trap);
}

auto CPU::instructionTGEIU() -> void {
  if(RS64u >= IMM16u) exception(Trap);
}

auto CPU::instructionTGEU() -> void {
  if(RS64u >= RT64u) exception(Trap);
}

auto CPU::instructionTLT() -> void {
  if(RS64i < RT64i) exception(Trap);
}

auto CPU::instructionTLTI() -> void {
  if(RS64i < IMM16i) exception(Trap);
}

auto CPU::instructionTLTIU() -> void {
  if(RS64u < IMM16u) exception(Trap);
}

auto CPU::instructionTLTU() -> void {
  if(RS64u < RT64u) exception(Trap);
}

auto CPU::instructionTNE() -> void {
  if(RS64u != RT64u) exception(Trap);
}

auto CPU::instructionTNEI() -> void {
  if(RS64u != IMM16u) exception(Trap);
}

auto CPU::instructionXOR() -> void {
  wRD64u(RS64u ^ RT64u);
}

auto CPU::instructionXORI() -> void {
  wRT64u(RS64u ^ IMM16u);
}
