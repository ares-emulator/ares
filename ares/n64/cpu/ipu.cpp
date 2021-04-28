#define PC ipu.pc
#define RA ipu.r[31]
#define LO ipu.lo
#define HI ipu.hi

auto CPU::instructionADD(r64& rd, cr64& rs, cr64& rt) -> void {
  if(~(rs.u32 ^ rt.u32) & (rs.u32 ^ rs.u32 + rt.u32) & 1 << 31) return exception.arithmeticOverflow();
  rd.u64 = s32(rs.u32 + rt.u32);
}

auto CPU::instructionADDI(r64& rt, cr64& rs, s16 imm) -> void {
  if(~(rs.u32 ^ imm) & (rs.u32 ^ rs.u32 + imm) & 1 << 31) return exception.arithmeticOverflow();
  rt.u64 = s32(rs.u32 + imm);
}

auto CPU::instructionADDIU(r64& rt, cr64& rs, s16 imm) -> void {
  rt.u64 = s32(rs.u32 + imm);
}

auto CPU::instructionADDU(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = s32(rs.u32 + rt.u32);
}

auto CPU::instructionAND(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.u64 & rt.u64;
}

auto CPU::instructionANDI(r64& rt, cr64& rs, u16 imm) -> void {
  rt.u64 = rs.u64 & imm;
}

auto CPU::instructionBEQ(cr64& rs, cr64& rt, s16 imm) -> void {
  if(rs.u64 == rt.u64) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBEQL(cr64& rs, cr64& rt, s16 imm) -> void {
  if(rs.u64 == rt.u64) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBGEZ(cr64& rs, s16 imm) -> void {
  if(rs.s64 >= 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBGEZAL(cr64& rs, s16 imm) -> void {
  RA.u64 = s32(PC + 8);
  if(rs.s64 >= 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBGEZALL(cr64& rs, s16 imm) -> void {
  RA.u64 = s32(PC + 8);
  if(rs.s64 >= 0) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBGEZL(cr64& rs, s16 imm) -> void {
  if(rs.s64 >= 0) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBGTZ(cr64& rs, s16 imm) -> void {
  if(rs.s64 > 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBGTZL(cr64& rs, s16 imm) -> void {
  if(rs.s64 > 0) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBLEZ(cr64& rs, s16 imm) -> void {
  if(rs.s64 <= 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBLEZL(cr64& rs, s16 imm) -> void {
  if(rs.s64 <= 0) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBLTZ(cr64& rs, s16 imm) -> void {
  if(rs.s64 < 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBLTZAL(cr64& rs, s16 imm) -> void {
  RA.u64 = s32(PC + 8);
  if(rs.s64 < 0) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBLTZALL(cr64& rs, s16 imm) -> void {
  RA.u64 = s32(PC + 8);
  if(rs.s64 < 0) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBLTZL(cr64& rs, s16 imm) -> void {
  if(rs.s64 < 0) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBNE(cr64& rs, cr64& rt, s16 imm) -> void {
  if(rs.u64 != rt.u64) branch.take(PC + 4 + (imm << 2));
}

auto CPU::instructionBNEL(cr64& rs, cr64& rt, s16 imm) -> void {
  if(rs.u64 != rt.u64) branch.take(PC + 4 + (imm << 2));
  else branch.discard();
}

auto CPU::instructionBREAK() -> void {
  exception.breakpoint();
}

auto CPU::instructionCACHE(u8 operation, cr64& rs, s16 imm) -> void {
  if(operation & 1) return;
  u32 address = rs.u64 + imm;

  switch(operation) {

  case 0x00: {  //icache index invalidate
    auto& line = icache.line(address);
    line.valid = 0;
    break;
  }

  case 0x04: {  //icache load tag
    auto& line = icache.line(address);
    scc.tagLo.primaryCacheState = line.valid ? 0b10 : 0b00;
    scc.tagLo.physicalAddress   = line.tag;
    break;
  }

  case 0x08: {  //icache store tag
    auto& line = icache.line(address);
    line.valid = scc.tagLo.primaryCacheState == 0b10;
    line.tag   = scc.tagLo.physicalAddress;
    if(scc.tagLo.primaryCacheState == 0b01) debug(unusual, "[CPU] CACHE CPCS=1");
    if(scc.tagLo.primaryCacheState == 0b11) debug(unusual, "[CPU] CACHE CPCS=3");
    break;
  }

  case 0x10: {  //icache hit invalidate
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = icache.line(address);
      if(line.hit(address)) line.valid = 0;
    }
    break;
  }

  case 0x14: {  //icache fill
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = icache.line(address);
      line.tag = address & ~0xfff;
      line.fill();
      line.valid = 1;
    }
    break;
  }

  case 0x18: {  //icache hit write back
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = icache.line(address);
      if(line.hit(address)) line.writeBack();
    }
    break;
  }

  case 0x01: {  //dcache index write back invalidate
    auto& line = dcache.line(address);
    if(line.valid && line.dirty) line.writeBack();
    line.valid = 0;
    break;
  }

  case 0x05: {  //dcache index load tag
    auto& line = dcache.line(address);
    scc.tagLo.primaryCacheState = line.valid ? 0b11 : 0b00;
    scc.tagLo.physicalAddress   = line.tag;
    break;
  }

  case 0x09: {  //dcache index store tag
    auto& line = dcache.line(address);
    line.valid = scc.tagLo.primaryCacheState == 0b11;
    line.dirty = scc.tagLo.primaryCacheState == 0b11;
    line.tag   = scc.tagLo.physicalAddress;
    if(scc.tagLo.primaryCacheState == 0b01) debug(unusual, "[CPU] CACHE DPCS=1");
    if(scc.tagLo.primaryCacheState == 0b10) debug(unusual, "[CPU] CACHE DPCS=2");
    break;
  }

  case 0x0d: {  //dcache create dirty exclusive
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = dcache.line(address);
      if(!line.hit(address) && line.dirty) line.writeBack();
      line.tag   = address & ~0xfff;
      line.valid = 1;
      line.dirty = 1;
    }
    break;
  }

  case 0x11: {  //dcache hit invalidate
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = dcache.line(address);
      if(line.hit(address)) line.valid = 0;
    }
    break;
  }

  case 0x15: {  //dcache hit write back invalidate
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = dcache.line(address);
      if(line.hit(address)) {
        if(line.dirty) line.writeBack();
        line.valid = 0;
      }
    }
    break;
  }

  case 0x19: {  //dcache hit write back
    if(auto physicalAddress = devirtualize(address)) {
      address = physicalAddress();
      auto& line = dcache.line(address);
      if(line.hit(address) && line.dirty) line.writeBack();
    }
    break;
  }

  }
}

auto CPU::instructionDADD(r64& rd, cr64& rs, cr64& rt) -> void {
  if(~(rs.u64 ^ rt.u64) & (rs.u64 ^ rs.u64 + rt.u64) & 1ull << 63) return exception.arithmeticOverflow();
  rd.u64 = rs.u64 + rt.u64;
}

auto CPU::instructionDADDI(r64& rt, cr64& rs, s16 imm) -> void {
  if(~(rs.u64 ^ imm) & (rs.u64 ^ rs.u64 + imm) & 1ull << 63) return exception.arithmeticOverflow();
  rt.u64 = rs.u64 + imm;
}

auto CPU::instructionDADDIU(r64& rt, cr64& rs, s16 imm) -> void {
  rt.u64 = rs.u64 + imm;
}

auto CPU::instructionDADDU(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.u64 + rt.u64;
}

auto CPU::instructionDDIV(cr64& rs, cr64& rt) -> void {
  if(rt.s64) {
    //cast to i128 to prevent exception on INT64_MIN / -1
    LO.u64 = s128(rs.s64) / s128(rt.s64);
    HI.u64 = s128(rs.s64) % s128(rt.s64);
  } else {
    LO.u64 = rs.s64 < 0 ? +1 : -1;
    HI.u64 = rs.s64;
  }
}

auto CPU::instructionDDIVU(cr64& rs, cr64& rt) -> void {
  if(rt.u64) {
    LO.u64 = rs.u64 / rt.u64;
    HI.u64 = rs.u64 % rt.u64;
  } else {
    LO.u64 = -1;
    HI.u64 = rs.u64;
  }
}

auto CPU::instructionDIV(cr64& rs, cr64& rt) -> void {
  if(rt.s32) {
    //cast to s64 to prevent exception on INT32_MIN / -1
    LO.u64 = s32(s64(rs.s32) / s64(rt.s32));
    HI.u64 = s32(s64(rs.s32) % s64(rt.s32));
  } else {
    LO.u64 = rs.s32 < 0 ? +1 : -1;
    HI.u64 = rs.s32;
  }
}

auto CPU::instructionDIVU(cr64& rs, cr64& rt) -> void {
  if(rt.u32) {
    LO.u64 = s32(rs.u32 / rt.u32);
    HI.u64 = s32(rs.u32 % rt.u32);
  } else {
    LO.u64 = -1;
    HI.u64 = rs.s32;
  }
}

auto CPU::instructionDMULT(cr64& rs, cr64& rt) -> void {
  u128 result = rs.s128() * rt.s128();
  LO.u64 = result >>  0;
  HI.u64 = result >> 64;
}

auto CPU::instructionDMULTU(cr64& rs, cr64& rt) -> void {
  u128 result = rs.u128() * rt.u128();
  LO.u64 = result >>  0;
  HI.u64 = result >> 64;
}

auto CPU::instructionDSLL(r64& rd, cr64& rt, u8 sa) -> void {
  rd.u64 = rt.u64 << sa;
}

auto CPU::instructionDSLLV(r64& rd, cr64& rt, cr64& rs) -> void {
  rd.u64 = rt.u64 << (rs.u32 & 63);
}

auto CPU::instructionDSRA(r64& rd, cr64& rt, u8 sa) -> void {
  rd.u64 = rt.s64 >> sa;
}

auto CPU::instructionDSRAV(r64& rd, cr64& rt, cr64& rs) -> void {
  rd.u64 = rt.s64 >> (rs.u32 & 63);
}

auto CPU::instructionDSRL(r64& rd, cr64& rt, u8 sa) -> void {
  rd.u64 = rt.u64 >> sa;
}

auto CPU::instructionDSRLV(r64& rd, cr64& rt, cr64& rs) -> void {
  rd.u64 = rt.u64 >> (rs.u32 & 63);
}

auto CPU::instructionDSUB(r64& rd, cr64& rs, cr64& rt) -> void {
  if((rs.u64 ^ rt.u64) & (rs.u64 ^ rs.u64 - rt.u64) & 1ull << 63) return exception.arithmeticOverflow();
  rd.u64 = rs.u64 - rt.u64;
}

auto CPU::instructionDSUBU(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.u64 - rt.u64;
}

auto CPU::instructionJ(u32 imm) -> void {
  branch.take((PC + 4 & 0xf000'0000) | (imm << 2));
}

auto CPU::instructionJAL(u32 imm) -> void {
  RA.u64 = s32(PC + 8);
  branch.take((PC + 4 & 0xf000'0000) | (imm << 2));
}

auto CPU::instructionJALR(r64& rd, cr64& rs) -> void {
  rd.u64 = s32(PC + 8);
  branch.take(rs.u32);
}

auto CPU::instructionJR(cr64& rs) -> void {
  branch.take(rs.u32);
}

auto CPU::instructionLB(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Byte>(rs.u32 + imm)) rt.u64 = s8(*data);
}

auto CPU::instructionLBU(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Byte>(rs.u32 + imm)) rt.u64 = u8(*data);
}

auto CPU::instructionLD(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Dual>(rs.u32 + imm)) rt.u64 = *data;
}

auto CPU::instructionLDL(r64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = u64(0) - 1 << shift;
  if(auto data = read<Dual>(address & ~7)) {
    rt.u64 = rt.u64 & ~mask | *data << shift;
  }
}

auto CPU::instructionLDR(r64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = u64(0) - 1 >> shift;
  if(auto data = read<Dual>(address & ~7)) {
    rt.u64 = rt.u64 & ~mask | *data >> shift;
  }
}

auto CPU::instructionLH(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Half>(rs.u32 + imm)) rt.u64 = s16(*data);
}

auto CPU::instructionLHU(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Half>(rs.u32 + imm)) rt.u64 = u16(*data);
}

auto CPU::instructionLL(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Word>(rs.u32 + imm)) {
    rt.u64 = s32(*data);
    scc.ll = tlb.physicalAddress >> 4;
    scc.llbit = 1;
  }
}

auto CPU::instructionLLD(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Dual>(rs.u32 + imm)) {
    rt.u64 = *data;
    scc.ll = tlb.physicalAddress >> 4;
    scc.llbit = 1;
  }
}

auto CPU::instructionLUI(r64& rt, u16 imm) -> void {
  rt.u64 = s32(imm << 16);
}

auto CPU::instructionLW(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Word>(rs.u32 + imm)) rt.u64 = s32(*data);
}

auto CPU::instructionLWL(r64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = u32(0) - 1 << shift;
  if(auto data = read<Word>(address & ~3)) {
    rt.u64 = s32(rt.u32 & ~mask | *data << shift);
  }
}

auto CPU::instructionLWR(r64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = u32(0) - 1 >> shift;
  if(auto data = read<Word>(address & ~3)) {
    rt.u64 = s32(rt.u32 & ~mask | *data >> shift);
  }
}

auto CPU::instructionLWU(r64& rt, cr64& rs, s16 imm) -> void {
  if(auto data = read<Word>(rs.u32 + imm)) rt.u64 = u32(*data);
}

auto CPU::instructionMFHI(r64& rd) -> void {
  rd.u64 = HI.u64;
}

auto CPU::instructionMFLO(r64& rd) -> void {
  rd.u64 = LO.u64;
}

auto CPU::instructionMTHI(cr64& rs) -> void {
  HI.u64 = rs.u64;
}

auto CPU::instructionMTLO(cr64& rs) -> void {
  LO.u64 = rs.u64;
}

auto CPU::instructionMULT(cr64& rs, cr64& rt) -> void {
  u64 result = s64(rs.s32) * s64(rt.s32);
  LO.u64 = s32(result >>  0);
  HI.u64 = s32(result >> 32);
}

auto CPU::instructionMULTU(cr64& rs, cr64& rt) -> void {
  u64 result = u64(rs.u32) * u64(rt.u32);
  LO.u64 = s32(result >>  0);
  HI.u64 = s32(result >> 32);
}

auto CPU::instructionNOR(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = ~(rs.u64 | rt.u64);
}

auto CPU::instructionOR(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.u64 | rt.u64;
}

auto CPU::instructionORI(r64& rt, cr64& rs, u16 imm) -> void {
  rt.u64 = rs.u64 | imm;
}

auto CPU::instructionSB(cr64& rt, cr64& rs, s16 imm) -> void {
  write<Byte>(rs.u32 + imm, rt.u32);
}

auto CPU::instructionSC(r64& rt, cr64& rs, s16 imm) -> void {
  if(scc.llbit) {
    scc.llbit = 0;
    rt.u64 = write<Word>(rs.u32 + imm, rt.u32);
  } else {
    rt.u64 = 0;
  }
}

auto CPU::instructionSCD(r64& rt, cr64& rs, s16 imm) -> void {
  if(scc.llbit) {
    scc.llbit = 0;
    rt.u64 = write<Dual>(rs.u32 + imm, rt.u64);
  } else {
    rt.u64 = 0;
  }
}

auto CPU::instructionSD(cr64& rt, cr64& rs, s16 imm) -> void {
  write<Dual>(rs.u32 + imm, rt.u64);
}

auto CPU::instructionSDL(cr64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipLE) & 7);
  auto mask = u64(0) - 1 >> shift;
  if(auto data = read<Dual>(address & ~7)) {
    write<Dual>(address & ~7, *data & ~mask | rt.u64 >> shift);
  }
}

auto CPU::instructionSDR(cr64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipBE) & 7);
  auto mask = u64(0) - 1 << shift;
  if(auto data = read<Dual>(address & ~7)) {
    write<Dual>(address & ~7, *data & ~mask | rt.u64 << shift);
  }
}

auto CPU::instructionSH(cr64& rt, cr64& rs, s16 imm) -> void {
  write<Half>(rs.u32 + imm, rt.u32);
}

auto CPU::instructionSLL(r64& rd, cr64& rt, u8 sa) -> void {
  rd.u64 = s32(rt.u32 << sa);
}

auto CPU::instructionSLLV(r64& rd, cr64& rt, cr64& rs) -> void {
  rd.u64 = s32(rt.u32 << (rs.u32 & 31));
}

auto CPU::instructionSLT(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.s64 < rt.s64;
}

auto CPU::instructionSLTI(r64& rt, cr64& rs, s16 imm) -> void {
  rt.u64 = rs.s64 < imm;
}

auto CPU::instructionSLTIU(r64& rt, cr64& rs, s16 imm) -> void {
  rt.u64 = rs.u64 < imm;
}

auto CPU::instructionSLTU(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.u64 < rt.u64;
}

auto CPU::instructionSRA(r64& rd, cr64& rt, u8 sa) -> void {
  rd.u64 = rt.s32 >> sa;
}

auto CPU::instructionSRAV(r64& rd, cr64& rt, cr64& rs) -> void {
  rd.u64 = rt.s32 >> (rs.u32 & 31);
}

auto CPU::instructionSRL(r64& rd, cr64& rt, u8 sa) -> void {
  rd.u64 = s32(rt.u32 >> sa);
}

auto CPU::instructionSRLV(r64& rd, cr64& rt, cr64& rs) -> void {
  rd.u64 = s32(rt.u32 >> (rs.u32 & 31));
}

auto CPU::instructionSUB(r64& rd, cr64& rs, cr64& rt) -> void {
  if((rs.u32 ^ rt.u32) & (rs.u32 ^ rs.u32 - rt.u32) & 1 << 31) return exception.arithmeticOverflow();
  rd.u64 = s32(rs.u32 - rt.u32);
}

auto CPU::instructionSUBU(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = s32(rs.u32 - rt.u32);
}

auto CPU::instructionSW(cr64& rt, cr64& rs, s16 imm) -> void {
  write<Word>(rs.u32 + imm, rt.u32);
}

auto CPU::instructionSWL(cr64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipLE) & 3);
  auto mask = u32(0) - 1 >> shift;
  if(auto data = read<Word>(address & ~3)) {
    write<Word>(address & ~3, *data & ~mask | rt.u32 >> shift);
  }
}

auto CPU::instructionSWR(cr64& rt, cr64& rs, s16 imm) -> void {
  auto address = rs.u32 + imm;
  auto shift = 8 * ((address ^ FlipBE) & 3);
  auto mask = u32(0) - 1 << shift;
  if(auto data = read<Word>(address & ~3)) {
    write<Word>(address & ~3, *data & ~mask | rt.u32 << shift);
  }
}

auto CPU::instructionSYNC() -> void {
  //no operation; for compatibility with R4000-series code
}

auto CPU::instructionSYSCALL() -> void {
  exception.systemCall();
}

auto CPU::instructionTEQ(cr64& rs, cr64& rt) -> void {
  if(rs.u64 == rt.u64) exception.trap();
}

auto CPU::instructionTEQI(cr64& rs, s16 imm) -> void {
  if(rs.s64 == imm) exception.trap();
}

auto CPU::instructionTGE(cr64& rs, cr64& rt) -> void {
  if(rs.s64 >= rt.s64) exception.trap();
}

auto CPU::instructionTGEI(cr64& rs, s16 imm) -> void {
  if(rs.s64 >= imm) exception.trap();
}

auto CPU::instructionTGEIU(cr64& rs, s16 imm) -> void {
  if(rs.u64 >= imm) exception.trap();
}

auto CPU::instructionTGEU(cr64& rs, cr64& rt) -> void {
  if(rs.u64 >= rt.u64) exception.trap();
}

auto CPU::instructionTLT(cr64& rs, cr64& rt) -> void {
  if(rs.s64 < rt.s64) exception.trap();
}

auto CPU::instructionTLTI(cr64& rs, s16 imm) -> void {
  if(rs.s64 < imm) exception.trap();
}

auto CPU::instructionTLTIU(cr64& rs, s16 imm) -> void {
  if(rs.u64 < imm) exception.trap();
}

auto CPU::instructionTLTU(cr64& rs, cr64& rt) -> void {
  if(rs.u64 < rt.u64) exception.trap();
}

auto CPU::instructionTNE(cr64& rs, cr64& rt) -> void {
  if(rs.u64 != rt.u64) exception.trap();
}

auto CPU::instructionTNEI(cr64& rs, s16 imm) -> void {
  if(rs.s64 != imm) exception.trap();
}

auto CPU::instructionXOR(r64& rd, cr64& rs, cr64& rt) -> void {
  rd.u64 = rs.u64 ^ rt.u64;
}

auto CPU::instructionXORI(r64& rt, cr64& rs, u16 imm) -> void {
  rt.u64 = rs.u64 ^ imm;
}

#undef PC
#undef RA
#undef LO
#undef HI
