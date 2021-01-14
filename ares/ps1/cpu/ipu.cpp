#define PC pc
#define PD pd
#define RA r[31]
#define HI hi
#define LO lo
#define load self.load
#define store self.store
#define branch self.branch
#define read self.read
#define write self.write
#define exception self.exception

auto CPU::IPU::ADD(u32& rd, cu32& rs, cu32& rt) -> void {
  if(~(rs ^ rt) & (rs ^ rs + rt) & 0x8000'0000) return exception.arithmeticOverflow();
  store(rd, rs + rt);
}

auto CPU::IPU::ADDI(u32& rt, cu32& rs, s16 imm) -> void {
  if(~(rs ^ imm) & (rs ^ rs + imm) & 0x8000'0000) return exception.arithmeticOverflow();
  store(rt, rs + imm);
}

auto CPU::IPU::ADDIU(u32& rt, cu32& rs, s16 imm) -> void {
  store(rt, rs + imm);
}

auto CPU::IPU::ADDU(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs + rt);
}

auto CPU::IPU::AND(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs & rt);
}

auto CPU::IPU::ANDI(u32& rt, cu32& rs, u16 imm) -> void {
  store(rt, rs & imm);
}

auto CPU::IPU::BEQ(cu32& rs, cu32& rt, s16 imm) -> void {
  branch(PD + (imm << 2), rs == rt);
}

auto CPU::IPU::BGEZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs >= 0);
}

auto CPU::IPU::BGEZAL(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs >= 0);
  store(RA, PD + 4);
}

auto CPU::IPU::BGTZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs > 0);
}

auto CPU::IPU::BLEZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs <= 0);
}

auto CPU::IPU::BLTZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs < 0);
}

auto CPU::IPU::BLTZAL(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs < 0);
  store(RA, PD + 4);
}

auto CPU::IPU::BNE(cu32& rs, cu32& rt, s16 imm) -> void {
  branch(PD + (imm << 2), rs != rt);
}

auto CPU::IPU::BREAK() -> void {
  exception.breakpoint(0);
}

auto CPU::IPU::DIV(cs32& rs, cs32& rt) -> void {
  if(rt) {
    //cast to s64 to prevent exception on INT32_MIN / -1
    LO = s64(rs) / s64(rt);
    HI = s64(rs) % s64(rt);
  } else {
    LO = rs < 0 ? +1 : -1;
    HI = rs;
  }
}

auto CPU::IPU::DIVU(cu32& rs, cu32& rt) -> void {
  if(rt) {
    LO = rs / rt;
    HI = rs % rt;
  } else {
    LO = -1;
    HI = rs;
  }
}

auto CPU::IPU::J(u32 imm) -> void {
  branch((PD & 0xf000'0000) + (imm << 2));
}

auto CPU::IPU::JAL(u32 imm) -> void {
  branch((PD & 0xf000'0000) + (imm << 2));
  store(RA, PD + 4);
}

auto CPU::IPU::JALR(u32& rd, cu32& rs) -> void {
  branch(rs);
  store(rd, PD + 4);
}

auto CPU::IPU::JR(cu32& rs) -> void {
  branch(rs);
}

auto CPU::IPU::LB(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Byte>(rs + imm);
  load(rt, s8(data));
}

auto CPU::IPU::LBU(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Byte>(rs + imm);
  load(rt, u8(data));
}

auto CPU::IPU::LH(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Half>(rs + imm);
  if(exception()) return;
  load(rt, s16(data));
}

auto CPU::IPU::LHU(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Half>(rs + imm);
  if(exception()) return;
  load(rt, u16(data));
}

auto CPU::IPU::LUI(u32& rt, u16 imm) -> void {
  store(rt, imm << 16);
}

auto CPU::IPU::LW(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Word>(rs + imm);
  if(exception()) return;
  load(rt, s32(data));
}

auto CPU::IPU::LWL(u32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 data = load(rt);
  switch(address & 3) {
  case 0:
    data &= 0x00ffffff;
    data |= read<Byte>(address & ~3 | 0) << 24; if(exception()) return;
    break;
  case 1:
    data &= 0x0000ffff;
    data |= read<Half>(address & ~3 | 0) << 16; if(exception()) return;
    break;
  case 2:
    data &= 0x000000ff;
    data |= read<Half>(address & ~3 | 0) <<  8; if(exception()) return;
    data |= read<Byte>(address & ~3 | 2) << 24; if(exception()) return;
    break;
  case 3:
    data &= 0x00000000;
    data |= read<Word>(address & ~3 | 0) <<  0; if(exception()) return;
    break;
  }
  load(rt, data);
}

auto CPU::IPU::LWR(u32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 data = load(rt);
  switch(address & 3) {
  case 0:
    data &= 0x00000000;
    data |= read<Word>(address & ~3 | 0) <<  0; if(exception()) break;
    break;
  case 1:
    data &= 0xff000000;
    data |= read<Byte>(address & ~3 | 1) <<  0; if(exception()) break;
    data |= read<Half>(address & ~3 | 2) <<  8; if(exception()) break;
    break;
  case 2:
    data &= 0xffff0000;
    data |= read<Half>(address & ~3 | 2) <<  0; if(exception()) break;
    break;
  case 3:
    data &= 0xffffff00;
    data |= read<Byte>(address & ~3 | 3) <<  0; if(exception()) break;
    break;
  }
  load(rt, data);
}

auto CPU::IPU::MFHI(u32& rd) -> void {
  store(rd, HI);
}

auto CPU::IPU::MFLO(u32& rd) -> void {
  store(rd, LO);
}

auto CPU::IPU::MTHI(cu32& rs) -> void {
  HI = rs;
}

auto CPU::IPU::MTLO(cu32& rs) -> void {
  LO = rs;
}

auto CPU::IPU::MULT(cs32& rs, cs32& rt) -> void {
  u64 result = s64(rs) * s64(rt);
  LO = result >>  0;
  HI = result >> 32;
}

auto CPU::IPU::MULTU(cu32& rs, cu32& rt) -> void {
  u64 result = u64(rs) * u64(rt);
  LO = result >>  0;
  HI = result >> 32;
}

auto CPU::IPU::NOR(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, ~(rs | rt));
}

auto CPU::IPU::OR(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs | rt);
}

auto CPU::IPU::ORI(u32& rt, cu32& rs, u16 imm) -> void {
  store(rt, rs | imm);
}

auto CPU::IPU::SB(cu32& rt, cu32& rs, s16 imm) -> void {
  write<Byte>(rs + imm, rt);
}

auto CPU::IPU::SH(cu32& rt, cu32& rs, s16 imm) -> void {
  write<Half>(rs + imm, rt);
}

auto CPU::IPU::SLL(u32& rd, cu32& rt, u8 sa) -> void {
  store(rd, rt << sa);
}

auto CPU::IPU::SLLV(u32& rd, cu32& rt, cu32& rs) -> void {
  store(rd, rt << (rs & 31));
}

auto CPU::IPU::SLT(u32& rd, cs32& rs, cs32& rt) -> void {
  store(rd, rs < rt);
}

auto CPU::IPU::SLTI(u32& rt, cs32& rs, s16 imm) -> void {
  store(rt, rs < imm);
}

auto CPU::IPU::SLTIU(u32& rt, cu32& rs, s16 imm) -> void {
  store(rt, rs < imm);
}

auto CPU::IPU::SLTU(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs < rt);
}

auto CPU::IPU::SRA(u32& rd, cs32& rt, u8 sa) -> void {
  store(rd, rt >> sa);
}

auto CPU::IPU::SRAV(u32& rd, cs32& rt, cu32& rs) -> void {
  store(rd, rt >> (rs & 31));
}

auto CPU::IPU::SRL(u32& rd, cu32& rt, u8 sa) -> void {
  store(rd, rt >> sa);
}

auto CPU::IPU::SRLV(u32& rd, cu32& rt, cu32& rs) -> void {
  store(rd, rt >> (rs & 31));
}

auto CPU::IPU::SUB(u32& rd, cu32& rs, cu32& rt) -> void {
  if((rs ^ rt) & (rs ^ rs - rt) & 0x8000'0000) return exception.arithmeticOverflow();
  store(rd, rs - rt);
}

auto CPU::IPU::SUBU(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs - rt);
}

auto CPU::IPU::SW(cu32& rt, cu32& rs, s16 imm) -> void {
  write<Word>(rs + imm, rt);
}

auto CPU::IPU::SWL(cu32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 data = rt;
  switch(address & 3) {
  case 0:
    write<Byte>(address & ~3 | 0, data >> 24); if(exception()) return;
    break;
  case 1:
    write<Half>(address & ~3 | 0, data >> 16); if(exception()) return;
    break;
  case 2:
    write<Half>(address & ~3 | 0, data >>  8); if(exception()) return;
    write<Byte>(address & ~3 | 2, data >> 24); if(exception()) return;
    break;
  case 3:
    write<Word>(address & ~3 | 0, data >>  0); if(exception()) return;
    break;
  }
}

auto CPU::IPU::SWR(cu32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 data = rt;
  switch(address & 3) {
  case 0:
    write<Word>(address & ~3 | 0, data >>  0); if(exception()) return;
    break;
  case 1:
    write<Byte>(address & ~3 | 1, data >>  0); if(exception()) return;
    write<Half>(address & ~3 | 2, data >>  8); if(exception()) return;
    break;
  case 2:
    write<Half>(address & ~3 | 2, data >>  0); if(exception()) return;
    break;
  case 3:
    write<Byte>(address & ~3 | 3, data >>  0); if(exception()) return;
    break;
  }
}

auto CPU::IPU::SYSCALL() -> void {
  exception.systemCall();
}

auto CPU::IPU::XOR(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs ^ rt);
}

auto CPU::IPU::XORI(u32& rt, cu32& rs, u16 imm) -> void {
  store(rt, rs ^ imm);
}

#undef PC
#undef PD
#undef RA
#undef HI
#undef LO
#undef load
#undef store
#undef branch
#undef read
#undef write
#undef exception
