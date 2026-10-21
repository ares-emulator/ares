#define PC ipu.pc
#define PD ipu.pd
#define RA ipu.r[31]
#define HI ipu.hi
#define LO ipu.lo

auto CPU::ADD(u32& rd, cu32& rs, cu32& rt) -> void {
  if(~(rs ^ rt) & (rs ^ rs + rt) & 0x8000'0000) return exception.arithmeticOverflow();
  store(rd, rs + rt);
}

auto CPU::ADDI(u32& rt, cu32& rs, s16 imm) -> void {
  if(~(rs ^ imm) & (rs ^ rs + imm) & 0x8000'0000) return exception.arithmeticOverflow();
  store(rt, rs + imm);
}

auto CPU::ADDIU(u32& rt, cu32& rs, s16 imm) -> void {
  store(rt, rs + imm);
}

auto CPU::ADDU(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs + rt);
}

auto CPU::AND(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs & rt);
}

auto CPU::ANDI(u32& rt, cu32& rs, u16 imm) -> void {
  store(rt, rs & imm);
}

auto CPU::BEQ(cu32& rs, cu32& rt, s16 imm) -> void {
  branch(PD + (imm << 2), rs == rt);
}

auto CPU::BGEZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs >= 0);
}

auto CPU::BGEZAL(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs >= 0);
  store(RA, PD + 4);
}

auto CPU::BGTZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs > 0);
}

auto CPU::BLEZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs <= 0);
}

auto CPU::BLTZ(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs < 0);
}

auto CPU::BLTZAL(cs32& rs, s16 imm) -> void {
  branch(PD + (imm << 2), rs < 0);
  store(RA, PD + 4);
}

auto CPU::BNE(cu32& rs, cu32& rt, s16 imm) -> void {
  branch(PD + (imm << 2), rs != rt);
}

auto CPU::BREAK() -> void {
  exception.breakpoint(0);
}

auto CPU::DIV(cs32& rs, cs32& rt) -> void {
  mulDivRemaining = 36;
  if(rt) {
    //cast to s64 to prevent exception on INT32_MIN / -1
    LO = s64(rs) / s64(rt);
    HI = s64(rs) % s64(rt);
  } else {
    LO = rs < 0 ? +1 : -1;
    HI = rs;
  }
}

auto CPU::DIVU(cu32& rs, cu32& rt) -> void {
  mulDivRemaining = 36;
  if(rt) {
    LO = rs / rt;
    HI = rs % rt;
  } else {
    LO = -1;
    HI = rs;
  }
}

auto CPU::J(u32 imm) -> void {
  branch((PD & 0xf000'0000) + (imm << 2));
}

auto CPU::JAL(u32 imm) -> void {
  branch((PD & 0xf000'0000) + (imm << 2));
  store(RA, PD + 4);
}

auto CPU::JALR(u32& rd, cu32& rs) -> void {
  branch(rs);
  store(rd, PD + 4);
}

auto CPU::JR(cu32& rs) -> void {
  branch(rs);
}

alwaysinline auto CPU::LB(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Byte>(rs + imm);
  load(rt, s8(data));
}

alwaysinline auto CPU::LBU(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Byte>(rs + imm);
  load(rt, u8(data));
}

alwaysinline auto CPU::LH(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Half>(rs + imm);
  if(unlikely(exception())) return;
  load(rt, s16(data));
}

alwaysinline auto CPU::LHU(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Half>(rs + imm);
  if(unlikely(exception())) return;
  load(rt, u16(data));
}

auto CPU::LUI(u32& rt, u16 imm) -> void {
  store(rt, imm << 16);
}

alwaysinline auto CPU::LW(u32& rt, cu32& rs, s16 imm) -> void {
  auto data = read<Word>(rs + imm);
  if(unlikely(exception())) return;
  load(rt, s32(data));
}

auto CPU::LWL(u32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 offset = address & 3;

  static constexpr u32 lanes[4] = {
    0b0001,
    0b0011,
    0b0111,
    0b1111,
  };

  static constexpr u32 mask[4] = {
    0x00ffffff,
    0x0000ffff,
    0x000000ff,
    0x00000000,
  };

  static constexpr u32 shift[4] = {24, 16, 8, 0};

  u32 mem = readWordPartial(address, lanes[offset]);
  if(unlikely(exception())) return;
  load(rt, (load(rt) & mask[offset]) | (mem << shift[offset]));
}

auto CPU::LWR(u32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 offset = address & 3;

  static constexpr u32 lanes[4] = {
    0b1111,
    0b1110,
    0b1100,
    0b1000,
  };

  static constexpr u32 mask[4] = {
    0x00000000,
    0xff000000,
    0xffff0000,
    0xffffff00,
  };

  static constexpr u32 shift[4] = {0, 8, 16, 24};

  u32 mem = readWordPartial(address, lanes[offset]);
  if(unlikely(exception())) return;
  load(rt, (load(rt) & mask[offset]) | (mem >> shift[offset]));
}

auto CPU::MFHI(u32& rd) -> void {
  mulDivStall();
  store(rd, HI);
}

auto CPU::MFLO(u32& rd) -> void {
  mulDivStall();
  store(rd, LO);
}

auto CPU::MTHI(cu32& rs) -> void {
  HI = rs;
}

auto CPU::MTLO(cu32& rs) -> void {
  LO = rs;
}

auto CPU::MULT(cs32& rs, cs32& rt) -> void {
  u32 timing = rs < 0 ? ~rs : rs;
  mulDivRemaining = (timing >> 11 == 0 ? 6 : timing >> 20 == 0 ? 9 : 13);
  u64 result = s64(rs) * s64(rt);
  LO = result >>  0;
  HI = result >> 32;
}

auto CPU::MULTU(cu32& rs, cu32& rt) -> void {
  mulDivRemaining = (rs >> 11 == 0 ? 6 : rs >> 20 == 0 ? 9 : 13);
  u64 result = u64(rs) * u64(rt);
  LO = result >>  0;
  HI = result >> 32;
}

auto CPU::NOR(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, ~(rs | rt));
}

auto CPU::OR(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs | rt);
}

auto CPU::ORI(u32& rt, cu32& rs, u16 imm) -> void {
  store(rt, rs | imm);
}

alwaysinline auto CPU::SB(cu32& rt, cu32& rs, s16 imm) -> void {
  write<Byte>(rs + imm, rt);
}

alwaysinline auto CPU::SH(cu32& rt, cu32& rs, s16 imm) -> void {
  write<Half>(rs + imm, rt);
}

auto CPU::SLL(u32& rd, cu32& rt, u8 sa) -> void {
  store(rd, rt << sa);
}

auto CPU::SLLV(u32& rd, cu32& rt, cu32& rs) -> void {
  store(rd, rt << (rs & 31));
}

auto CPU::SLT(u32& rd, cs32& rs, cs32& rt) -> void {
  store(rd, rs < rt);
}

auto CPU::SLTI(u32& rt, cs32& rs, s16 imm) -> void {
  store(rt, rs < imm);
}

auto CPU::SLTIU(u32& rt, cu32& rs, s16 imm) -> void {
  store(rt, rs < imm);
}

auto CPU::SLTU(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs < rt);
}

auto CPU::SRA(u32& rd, cs32& rt, u8 sa) -> void {
  store(rd, rt >> sa);
}

auto CPU::SRAV(u32& rd, cs32& rt, cu32& rs) -> void {
  store(rd, rt >> (rs & 31));
}

auto CPU::SRL(u32& rd, cu32& rt, u8 sa) -> void {
  store(rd, rt >> sa);
}

auto CPU::SRLV(u32& rd, cu32& rt, cu32& rs) -> void {
  store(rd, rt >> (rs & 31));
}

auto CPU::SUB(u32& rd, cu32& rs, cu32& rt) -> void {
  if((rs ^ rt) & (rs ^ rs - rt) & 0x8000'0000) return exception.arithmeticOverflow();
  store(rd, rs - rt);
}

auto CPU::SUBU(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs - rt);
}

alwaysinline auto CPU::SW(cu32& rt, cu32& rs, s16 imm) -> void {
  write<Word>(rs + imm, rt);
}

auto CPU::SWL(cu32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 offset = address & 3;
  writeWordPartial(address, rt >> (24 - offset * 8), (1u << (offset + 1)) - 1);
}

auto CPU::SWR(cu32& rt, cu32& rs, s16 imm) -> void {
  u32 address = rs + imm;
  u32 offset = address & 3;
  writeWordPartial(address, rt << (offset * 8), (15u << offset) & 15);
}
auto CPU::SYSCALL() -> void {
  exception.systemCall();
}

auto CPU::XOR(u32& rd, cu32& rs, cu32& rt) -> void {
  store(rd, rs ^ rt);
}

auto CPU::XORI(u32& rt, cu32& rs, u16 imm) -> void {
  store(rt, rs ^ imm);
}

#undef PC
#undef PD
#undef RA
#undef HI
#undef LO
