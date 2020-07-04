#include "gte-registers.hpp"

auto CPU::instructionCFC2(u32& rt, u8 rd) -> void {
  switch(rd) {
  case  0: rt = RT11 << 0 | RT12 << 16; break;
  case  1: rt = RT13 << 0 | RT21 << 16; break;
  case  2: rt = RT22 << 0 | RT23 << 16; break;
  case  3: rt = RT31 << 0 | RT32 << 16; break;
  case  4: rt = RT33 << 0; break;
  case  5: rt = TRX; break;
  case  6: rt = TRY; break;
  case  7: rt = TRZ; break;
  case  8: rt = L11 << 0 | L12 << 16; break;
  case  9: rt = L13 << 0 | L21 << 16; break;
  case 10: rt = L22 << 0 | L23 << 16; break;
  case 11: rt = L31 << 0 | L32 << 16; break;
  case 12: rt = L33 << 0; break;
  case 13: rt = RBK; break;
  case 14: rt = GBK; break;
  case 15: rt = BBK; break;
  case 16: rt = LR1 << 0 | LR2 << 16; break;
  case 17: rt = LR3 << 0 | LG1 << 16; break;
  case 18: rt = LG2 << 0 | LG3 << 16; break;
  case 19: rt = LB1 << 0 | LB2 << 16; break;
  case 20: rt = LB3 << 0; break;
  case 21: rt = RFC; break;
  case 22: rt = GFC; break;
  case 23: rt = BFC; break;
  case 24: rt = OFX; break;
  case 25: rt = OFY; break;
  case 26: rt = H; break;
  case 27: rt = DQA; break;
  case 28: rt = DQB; break;
  case 29: rt = ZSF3; break;
  case 30: rt = ZSF4; break;
  case 31: rt = 0;
    rt |= FLAG.saturated.ir0     << 12;
    rt |= FLAG.saturated.sy2     << 13;
    rt |= FLAG.saturated.sx2     << 14;
    rt |= FLAG.underflow.mac0    << 15;
    rt |= FLAG.overflow.mac0     << 16;
    rt |= FLAG.overflow.division << 17;
    rt |= FLAG.saturated.sz1_otz << 18;
    rt |= FLAG.saturated.b       << 19;
    rt |= FLAG.saturated.g       << 20;
    rt |= FLAG.saturated.r       << 21;
    rt |= FLAG.saturated.ir3     << 22;
    rt |= FLAG.saturated.ir2     << 23;
    rt |= FLAG.saturated.ir1     << 24;
    rt |= FLAG.underflow.mac3    << 25;
    rt |= FLAG.underflow.mac2    << 26;
    rt |= FLAG.underflow.mac1    << 27;
    rt |= FLAG.overflow.mac3     << 28;
    rt |= FLAG.overflow.mac2     << 29;
    rt |= FLAG.overflow.mac1     << 30;
    rt |= FLAG.error             << 31;
    break;
  }
}

auto CPU::instructionCTC2(cu32& rt, u8 rd) -> void {
  switch(rd) {
  case  0: RT11 = rt >> 0; RT12 = rt >> 16; break;
  case  1: RT13 = rt >> 0; RT21 = rt >> 16; break;
  case  2: RT22 = rt >> 0; RT23 = rt >> 16; break;
  case  3: RT31 = rt >> 0; RT32 = rt >> 16; break;
  case  4: RT33 = rt >> 0; break;
  case  5: TRX = rt; break;
  case  6: TRY = rt; break;
  case  7: TRZ = rt; break;
  case  8: L11 = rt >> 0; L12 = rt >> 16; break;
  case  9: L13 = rt >> 0; L21 = rt >> 16; break;
  case 10: L22 = rt >> 0; L23 = rt >> 16; break;
  case 11: L31 = rt >> 0; L32 = rt >> 16; break;
  case 12: L33 = rt >> 0; break;
  case 13: RBK = rt; break;
  case 14: GBK = rt; break;
  case 15: BBK = rt; break;
  case 16: LR1 = rt >> 0; LR2 = rt >> 16; break;
  case 17: LR3 = rt >> 0; LG1 = rt >> 16; break;
  case 18: LG2 = rt >> 0; LG3 = rt >> 16; break;
  case 19: LB1 = rt >> 0; LB2 = rt >> 16; break;
  case 20: LB3 = rt >> 0; break;
  case 21: RFC = rt; break;
  case 22: GFC = rt; break;
  case 23: BFC = rt; break;
  case 24: OFX = rt; break;
  case 25: OFY = rt; break;
  case 26: H = rt; break;
  case 27: DQA = rt; break;
  case 28: DQB = rt; break;
  case 29: ZSF3 = rt; break;
  case 30: ZSF4 = rt; break;
  case 31:
    FLAG.saturated.ir0     = rt >> 12 & 1;
    FLAG.saturated.sy2     = rt >> 13 & 1;
    FLAG.saturated.sx2     = rt >> 14 & 1;
    FLAG.underflow.mac0    = rt >> 15 & 1;
    FLAG.overflow.mac0     = rt >> 16 & 1;
    FLAG.overflow.division = rt >> 17 & 1;
    FLAG.saturated.sz1_otz = rt >> 18 & 1;
    FLAG.saturated.b       = rt >> 19 & 1;
    FLAG.saturated.g       = rt >> 20 & 1;
    FLAG.saturated.r       = rt >> 21 & 1;
    FLAG.saturated.ir3     = rt >> 22 & 1;
    FLAG.saturated.ir2     = rt >> 23 & 1;
    FLAG.saturated.ir1     = rt >> 24 & 1;
    FLAG.underflow.mac3    = rt >> 25 & 1;
    FLAG.underflow.mac2    = rt >> 26 & 1;
    FLAG.underflow.mac1    = rt >> 27 & 1;
    FLAG.overflow.mac3     = rt >> 28 & 1;
    FLAG.overflow.mac2     = rt >> 29 & 1;
    FLAG.overflow.mac1     = rt >> 30 & 1;
    FLAG.error             = rt >> 31 & 1;
    break;
  }
}

auto CPU::instructionMFC2(u32& rt, u8 rd) -> void {
  switch(rd) {
  case  0: rt = VX0 << 0 | VY0 << 16; break;
  case  1: rt = VZ0 << 0; break;
  case  2: rt = VX1 << 0 | VY1 << 16; break;
  case  3: rt = VZ1 << 0; break;
  case  4: rt = VX2 << 0 | VY2 << 16; break;
  case  5: rt = VZ2 << 0; break;
  case  6: rt = RGBC; break;
  case  7: rt = OTZ; break;
  case  8: rt = IR0; break;
  case  9: rt = IR1; break;
  case 10: rt = IR2; break;
  case 11: rt = IR3; break;
  case 12: rt = SX0 << 0 | SY0 << 16; break;
  case 13: rt = SX1 << 0 | SY1 << 16; break;
  case 14: rt = SX2 << 0 | SY2 << 16; break;
  case 15: rt = SX2 << 0 | SY2 << 16; break;
  case 16: rt = SZ0; break;
  case 17: rt = SZ1; break;
  case 18: rt = SZ2; break;
  case 19: rt = SZ3; break;
  case 20: rt = RGB0; break;
  case 21: rt = RGB1; break;
  case 22: rt = RGB2; break;
  case 23: rt = RES1; break;
  case 24: rt = MAC0; break;
  case 25: rt = MAC1; break;
  case 26: rt = MAC2; break;
  case 27: rt = MAC3; break;
  case 28:  //IRGB
  case 29: {//ORGB
    i8 r = (IR1 >> 7 < 0) ? 0 : (IR1 >> 7 > 0x1f) ? 0x1f : (IR1 >> 7);
    i8 g = (IR2 >> 7 < 0) ? 0 : (IR2 >> 7 > 0x1f) ? 0x1f : (IR2 >> 7);
    i8 b = (IR3 >> 7 < 0) ? 0 : (IR3 >> 7 > 0x1f) ? 0x1f : (IR3 >> 7);
    rt = r << 0 | g << 5 | b << 10;
  } break;
  case 30: rt = LZCS; break;
  case 31: rt = LZCR; break;
  }
}

auto CPU::instructionMTC2(cu32& rt, u8 rd) -> void {
  switch(rd) {
  case  0: VX0 = rt >> 0; VY0 = rt >> 16; break;
  case  1: VZ0 = rt >> 0; break;
  case  2: VX1 = rt >> 0; VY1 = rt >> 16; break;
  case  3: VZ1 = rt >> 0; break;
  case  4: VX2 = rt >> 0; VY2 = rt >> 16; break;
  case  5: VZ2 = rt >> 0; break;
  case  6: RGBC = rt; break;
  case  7: OTZ = rt; break;
  case  8: IR0 = rt; break;
  case  9: IR1 = rt; break;
  case 10: IR2 = rt; break;
  case 11: IR3 = rt; break;
  case 12: SX0 = rt >> 0; SY0 = rt >> 16; break;
  case 13: SX1 = rt >> 0; SY1 = rt >> 16; break;
  case 14: SX2 = rt >> 0; SY2 = rt >> 16; break;
  case 15: {//SXP
    SX0 = SX1; SY0 = SY1;
    SX1 = SX2; SY1 = SY2;
    SX2 = rt >> 0; SY2 = rt >> 16;
  } break;
  case 16: SZ0 = rt; break;
  case 17: SZ1 = rt; break;
  case 18: SZ2 = rt; break;
  case 19: SZ3 = rt; break;
  case 20: RGB0 = rt; break;
  case 21: RGB1 = rt; break;
  case 22: RGB2 = rt; break;
  case 23: RES1 = rt; break;
  case 24: MAC0 = rt; break;
  case 25: MAC1 = rt; break;
  case 26: MAC2 = rt; break;
  case 27: MAC3 = rt; break;
  case 28: IRGB = rt; break;
  case 29: ORGB = rt; break;
  case 30: LZCS = rt; break;
  case 31: LZCR = rt; break;
  }
}

//

auto CPU::instructionAVSZ3() -> void {
  print("AVSZ3\n");
}

auto CPU::instructionAVSZ4() -> void {
  print("AVSZ4\n");
}

auto CPU::instructionCC() -> void {
  print("CC\n");
}

auto CPU::instructionCDP() -> void {
  print("CDP\n");
}

auto CPU::instructionDCPL() -> void {
  print("DCPL\n");
}

auto CPU::instructionDPCS() -> void {
  print("DPCS\n");
}

auto CPU::instructionDPCT() -> void {
  print("DPCT\n");
}

auto CPU::instructionGPF() -> void {
  print("GPF\n");
}

auto CPU::instructionGPL() -> void {
  print("GPL\n");
}

auto CPU::instructionINTPL() -> void {
  print("INTPL\n");
}

auto CPU::instructionMVMVA() -> void {
  print("MVMVA\n");
}

auto CPU::instructionNCCS() -> void {
  print("NCCS\n");
}

auto CPU::instructionNCCT() -> void {
  print("NCCT\n");
}

auto CPU::instructionNCDS() -> void {
  print("NCDS\n");
}

auto CPU::instructionNCDT() -> void {
  print("NCDT\n");
}

auto CPU::instructionNCLIP() -> void {
  print("NCLIP\n");
}

auto CPU::instructionNCS() -> void {
  print("NCS\n");
}

auto CPU::instructionNCT() -> void {
  print("NCT\n");
}

auto CPU::instructionOP() -> void {
  print("OP\n");
}

auto CPU::instructionRTPS() -> void {
  print("RTPS\n");
}

auto CPU::instructionRTPT() -> void {
  print("RTPT\n");
}

auto CPU::instructionSQR() -> void {
  print("SQR\n");
}

#include "gte-registers.hpp"
