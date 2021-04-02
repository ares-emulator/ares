//ADD Rm,Rn
auto SH2::ADD(u32 m, u32 n) -> void {
  R[n] += R[m];
}

//ADDC Rm,Rn
auto SH2::ADDC(u32 m, u32 n) -> void {
  bool c = (u64)R[n] + R[m] >> 32;
  R[n] = R[n] + R[m] + SR.T;
  SR.T = c;
}

//ADD #imm,Rn
auto SH2::ADDI(u32 i, u32 n) -> void {
  R[n] += (s8)i;
}

//ADDV Rm,Rn
auto SH2::ADDV(u32 m, u32 n) -> void {
  bool v = ~(R[n] ^ R[m]) & (R[n] ^ R[n] + R[m]) >> 31;
  R[n] += R[m];
  SR.T = v;
}

//AND Rm,Rn
auto SH2::AND(u32 m, u32 n) -> void {
  R[n] &= R[m];
}

//AND #imm,R0
auto SH2::ANDI(u32 i) -> void {
  R[0] &= i;
}

//AND.B #imm,@(R0,GBR)
auto SH2::ANDM(u32 i) -> void {
  u8 b = readByte(GBR + R[0]);
  writeByte(GBR + R[0], b & i);
}

//BF disp
auto SH2::BF(u32 d) -> void {
  if(SR.T == 0) {
    branch(PC + 4 + (s8)d * 2);
  }
}

//BF/S disp
auto SH2::BFS(u32 d) -> void {
  if(SR.T == 0) {
    delaySlot(PC + 4 + (s8)d * 2);
  }
}

//BRA disp
auto SH2::BRA(u32 d) -> void {
  delaySlot(PC + 4 + (i12)d * 2);
}

//BRAF disp
auto SH2::BRAF(u32 m) -> void {
  delaySlot(PC + 4 + R[m]);
}

//BSR disp
auto SH2::BSR(u32 d) -> void {
  PR = PC;
  delaySlot(PC + 4 + (i12)d * 2);
}

//BSRF Rm
auto SH2::BSRF(u32 m) -> void {
  PR = PC;
  delaySlot(PC + 4 + R[m]);
}

//BT disp
auto SH2::BT(u32 d) -> void {
  if(SR.T == 1) {
    branch(PC + 4 + (s8)d * 2);
  }
}

//BT/S disp
auto SH2::BTS(u32 d) -> void {
  if(SR.T == 1) {
    delaySlot(PC + 4 + (s8)d * 2);
  }
}

//CLRMAC
auto SH2::CLRMAC() -> void {
  MAC = 0;
}

//CLRT
auto SH2::CLRT() -> void {
  SR.T = 0;
}

//CMP/EQ Rm,Rn
auto SH2::CMPEQ(u32 m, u32 n) -> void {
  SR.T = R[n] == R[m];
}

//CMP/GE Rm,Rn
auto SH2::CMPGE(u32 m, u32 n) -> void {
  SR.T = (s32)R[n] >= (s32)R[m];
}

//CMP/GT Rm,Rn
auto SH2::CMPGT(u32 m, u32 n) -> void {
  SR.T = (s32)R[n] > (s32)R[m];
}

//CMP/HI Rm,Rn
auto SH2::CMPHI(u32 m, u32 n) -> void {
  SR.T = R[n] > R[m];
}

//CMP/HS Rm,Rn
auto SH2::CMPHS(u32 m, u32 n) -> void {
  SR.T = R[n] >= R[m];
}

//CMP/EQ #imm,R0
auto SH2::CMPIM(u32 i) -> void {
  SR.T = R[0] == (s8)i;
}

//CMP/PL Rn
auto SH2::CMPPL(u32 n) -> void {
  SR.T = (s32)R[n] > 0;
}

//CMP/PZ Rn
auto SH2::CMPPZ(u32 n) -> void {
  SR.T = (s32)R[n] >= 0;
}

//CMP/STR Rm,Rn
auto SH2::CMPSTR(u32 m, u32 n) -> void {
  SR.T = bool(R[n] ^ R[m]);
}

//DIV0S Rm,Rn
auto SH2::DIV0S(u32 m, u32 n) -> void {
  SR.Q = R[n] >> 31;
  SR.M = R[m] >> 31;
  SR.T = SR.Q != SR.M;
}

//DIV0U
auto SH2::DIV0U() -> void {
  SR.Q = 0;
  SR.M = 0;
  SR.T = 0;
}

//DIV1 Rm,Rn
auto SH2::DIV1(u32 m, u32 n) -> void {
  bool q = SR.Q;
  SR.Q = R[n] >> 31;
  u64 r = u32(R[n] << 1) | SR.T;
  if(q == SR.M) {
    r -= R[m];
  } else {
    r += R[m];
  }
  R[n] = r;
  SR.Q = (SR.Q ^ SR.M) ^ (r >> 32 & 1);
  SR.T = 1 - (SR.Q ^ SR.M);
}

//DMULS.L Rm,Rn
auto SH2::DMULS(u32 m, u32 n) -> void {
  MAC = (s64)(s32)R[n] * (s32)R[m];
}

//DMULU.L Rm,Rn
auto SH2::DMULU(u32 m, u32 n) -> void {
  MAC = (u64)R[n] * (u64)R[m];
}

//DT Rn
auto SH2::DT(u32 n) -> void {
  SR.T = --R[n] == 0;
}

//EXTS.B Rm,Rn
auto SH2::EXTSB(u32 m, u32 n) -> void {
  R[n] = (s8)R[m];
}

//EXTS.W Rm,Rn
auto SH2::EXTSW(u32 m, u32 n) -> void {
  R[n] = (s16)R[m];
}

//EXTU.B Rm,Rn
auto SH2::EXTUB(u32 m, u32 n) -> void {
  R[n] = (u8)R[m];
}

//EXTU.W Rm,Rn
auto SH2::EXTUW(u32 m, u32 n) -> void {
  R[n] = (u16)R[m];
}

//ILLEGAL
auto SH2::ILLEGAL() -> void {
  illegalInstruction();
}

//JMP @Rm
auto SH2::JMP(u32 m) -> void {
  delaySlot(R[m] + 4);
}

//JSR @Rm
auto SH2::JSR(u32 m) -> void {
  PR = PC;
  delaySlot(R[m] + 4);
}

//LDC Rm,SR
auto SH2::LDCSR(u32 m) -> void {
  SR = R[m];
  ID = 1;
}

//LDC Rm,GBR
auto SH2::LDCGBR(u32 m) -> void {
  GBR = R[m];
  ID = 1;
}

//LDC Rm,VBR
auto SH2::LDCVBR(u32 m) -> void {
  VBR = R[m];
  ID = 1;
}

//LDC.L @Rm+,SR
auto SH2::LDCMSR(u32 m) -> void {
  SR = readLong(R[m]);
  R[m] += 4;
  ID = 1;
}

//LDC.L @Rm+,GBR
auto SH2::LDCMGBR(u32 m) -> void {
  GBR = readLong(R[m]);
  R[m] += 4;
  ID = 1;
}

//LDC.L @Rm+,VBR
auto SH2::LDCMVBR(u32 m) -> void {
  VBR = readLong(R[m]);
  R[m] += 4;
  ID = 1;
}

//LDS Rm,MACH
auto SH2::LDSMACH(u32 m) -> void {
  MACH = R[m];
  ID = 1;
}

//LDS Rm,MACL
auto SH2::LDSMACL(u32 m) -> void {
  MACL = R[m];
  ID = 1;
}

//LDS Rm,PR
auto SH2::LDSPR(u32 m) -> void {
  PR = R[m];
  ID = 1;
}

//LDS.L @Rm+,MACH
auto SH2::LDSMMACH(u32 m) -> void {
  MACH = readLong(R[m]);
  R[m] += 4;
  ID = 1;
}

//LDS.L @Rm+,MACL
auto SH2::LDSMMACL(u32 m) -> void {
  MACL = readLong(R[m]);
  R[m] += 4;
  ID = 1;
}

//LDS.L @Rm+,PR
auto SH2::LDSMPR(u32 m) -> void {
  PR = readLong(R[m]);
  R[m] += 4;
  ID = 1;
}

//MAC.W @Rm+,@Rn+
auto SH2::MACW(u32 m, u32 n) -> void {
  s16 a = readWord(R[n]);
  R[n] += 2;
  s16 b = readWord(R[m]);
  R[m] += 2;
  if(!SR.S) {
    MAC = MAC + (s64)a * (s64)b;
  } else {
    u64 p = (u64)MACL + (s64)a * (s64)b;
    if(p != sclamp<32>(p)) MACH |= 1;
    MACL = sclamp<32>(p);
  }
}

//MAC.L @Rm+,@Rn+
auto SH2::MACL_(u32 m, u32 n) -> void {
  s32 a = readLong(R[n]);
  R[n] += 4;
  s32 b = readLong(R[m]);
  R[m] += 4;
  MAC = MAC + (s64)a * (s64)b;
  if(SR.S) MAC = sclamp<48>(MAC);
}

//MOV Rm,Rn
auto SH2::MOV(u32 m, u32 n) -> void {
  R[n] = R[m];
}

//MOV.B @Rm,Rn
auto SH2::MOVBL(u32 m, u32 n) -> void {
  R[n] = (s8)readByte(R[m]);
}

//MOV.B @(R0,Rm),Rn
auto SH2::MOVBL0(u32 m, u32 n) -> void {
  R[n] = (s8)readByte(R[m] + R[0]);
}

//MOV.B @(disp,Rm),R0
auto SH2::MOVBL4(u32 m, u32 d) -> void {
  R[0] = (s8)readByte(R[m] + d);
}

//MOV.B @(disp,GBR),R0
auto SH2::MOVBLG(u32 d) -> void {
  R[0] = (s8)readByte(GBR + d);
}

//MOV.B Rm,@-Rn
auto SH2::MOVBM(u32 m, u32 n) -> void {
  R[n] -= 1;
  writeByte(R[n], R[m]);
}

//MOV.B @Rm+,Rn
auto SH2::MOVBP(u32 m, u32 n) -> void {
  R[n] = (s8)readByte(R[m]);
  if(n != m) R[m] += 1;
}

//MOV.B Rm,@Rn
auto SH2::MOVBS(u32 m, u32 n) -> void {
  writeByte(R[n], R[m]);
}

//MOV.B Rm,@(R0,Rn)
auto SH2::MOVBS0(u32 m, u32 n) -> void {
  writeByte(R[n] + R[0], R[m]);
}

//MOV.B R0,@(disp,Rn)
auto SH2::MOVBS4(u32 d, u32 n) -> void {
  writeByte(R[n] + d, R[0]);
}

//MOV.B R0,@(disp,GBR)
auto SH2::MOVBSG(u32 d) -> void {
  writeByte(GBR + d, R[0]);
}

//MOV.W @Rm,Rn
auto SH2::MOVWL(u32 m, u32 n) -> void {
  R[n] = (s16)readWord(R[m]);
}

//MOV.W @(R0,Rm),Rn
auto SH2::MOVWL0(u32 m, u32 n) -> void {
  R[n] = (s16)readWord(R[m] + R[0]);
}

//MOV.W @(disp,Rm),R0
auto SH2::MOVWL4(u32 m, u32 d) -> void {
  R[0] = (s16)readWord(R[m] + d * 2);
}

//MOV.W @(disp,GBR),R0
auto SH2::MOVWLG(u32 d) -> void {
  R[0] = (s16)readWord(GBR + d * 2);
}

//MOV.W Rm,@-Rn
auto SH2::MOVWM(u32 m, u32 n) -> void {
  R[n] -= 2;
  writeWord(R[n], R[m]);
}

//MOV.W @Rm+,Rn
auto SH2::MOVWP(u32 m, u32 n) -> void {
  R[n] = (s16)readWord(R[m]);
  if(n != m) R[m] += 2;
}

//MOV.W Rm,@Rn
auto SH2::MOVWS(u32 m, u32 n) -> void {
  writeWord(R[n], R[m]);
}

//MOV.W Rm,@(R0,Rn)
auto SH2::MOVWS0(u32 m, u32 n) -> void {
  writeWord(R[n] + R[0], R[m]);
}

//MOV.W R0,@(disp,Rn)
auto SH2::MOVWS4(u32 d, u32 n) -> void {
  writeWord(R[n] + d * 2, R[0]);
}

//MOV.W R0,@(disp,GBR)
auto SH2::MOVWSG(u32 d) -> void {
  writeWord(GBR + d * 2, R[0]);
}

//MOV.L @Rm,Rn
auto SH2::MOVLL(u32 m, u32 n) -> void {
  R[n] = readLong(R[m]);
}

//MOV.L @(R0,Rm),Rn
auto SH2::MOVLL0(u32 m, u32 n) -> void {
  R[n] = readLong(R[m] + R[0]);
}

//MOV.L @(disp,Rm),Rn
auto SH2::MOVLL4(u32 m, u32 d, u32 n) -> void {
  R[n] = readLong(R[m] + d * 4);
}

//MOV.L @(disp,GBR),R0
auto SH2::MOVLLG(u32 d) -> void {
  R[0] = readLong(GBR + d * 4);
}

//MOV.L Rm,@-Rn
auto SH2::MOVLM(u32 m, u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], R[m]);
}

//MOV.L @Rm+,Rn
auto SH2::MOVLP(u32 m, u32 n) -> void {
  R[n] = readLong(R[m]);
  if(n != m) R[m] += 4;
}

//MOV.L Rm,@Rn
auto SH2::MOVLS(u32 m, u32 n) -> void {
  writeLong(R[n], R[m]);
}

//MOV.L Rm,@(R0,Rn)
auto SH2::MOVLS0(u32 m, u32 n) -> void {
  writeLong(R[n] + R[0], R[m]);
}

//MOV.L Rm,@(disp,Rn)
auto SH2::MOVLS4(u32 m, u32 d, u32 n) -> void {
  writeLong(R[n] + d * 4, R[m]);
}

//MOV.L R0,@(disp,GBR)
auto SH2::MOVLSG(u32 d) -> void {
  writeLong(GBR + d * 4, R[0]);
}

//MOV #imm,Rn
auto SH2::MOVI(u32 i, u32 n) -> void {
  R[n] = (s8)i;
}

//MOV.W @(disp,PC),Rn
auto SH2::MOVWI(u32 d, u32 n) -> void {
  R[n] = (s16)readWord(PC + d * 2);
}

//MOV.L @(disp,PC),Rn
auto SH2::MOVLI(u32 d, u32 n) -> void {
  R[n] = readLong((PC & ~3) + d * 4);
}

//MOVA @(disp,PC),R0
auto SH2::MOVA(u32 d) -> void {
  R[0] = (PC & ~3) + d * 4 - inDelaySlot() * 2;
}

//MOVT Rn
auto SH2::MOVT(u32 n) -> void {
  R[n] = SR.T;
}

//MUL.L Rm,Rn
auto SH2::MULL(u32 m, u32 n) -> void {
  MACL = R[n] * R[m];
}

//MULS Rm,Rn
auto SH2::MULS(u32 m, u32 n) -> void {
  MACL = (s16)R[n] * (s16)R[m];
}

//MULU Rm,Rn
auto SH2::MULU(u32 m, u32 n) -> void {
  MACL = (u16)R[n] * (u16)R[m];
}

//NEG Rm,Rn
auto SH2::NEG(u32 m, u32 n) -> void {
  R[n] = 0 - R[m];
}

//NEGC Rm,Rn
auto SH2::NEGC(u32 m, u32 n) -> void {
  u32 t = 0 - R[m];
  R[n] = t - SR.T;
  SR.T = (s32)t >= 0 || t < R[n];
}

//NOP
auto SH2::NOP() -> void {
}

//NOT
auto SH2::NOT(u32 m, u32 n) -> void {
  R[n] = ~R[m];
}

//OR Rm,Rn
auto SH2::OR(u32 m, u32 n) -> void {
  R[n] |= R[m];
}

//OR #imm,R0
auto SH2::ORI(u32 i) -> void {
  R[0] |= i;
}

//OR.B #imm,@(R0,GBR)
auto SH2::ORM(u32 i) -> void {
  u8 b = readByte(GBR + R[0]);
  writeByte(GBR + R[0], b | i);
}

//ROTCL Rn
auto SH2::ROTCL(u32 n) -> void {
  bool c = R[n] >> 31;
  R[n] = R[n] << 1 | SR.T;
  SR.T = c;
}

//ROTCR Rn
auto SH2::ROTCR(u32 n) -> void {
  bool c = R[n] & 1;
  R[n] = R[n] >> 1 | SR.T << 31;
  SR.T = c;
}

//ROTL Rn
auto SH2::ROTL(u32 n) -> void {
  SR.T = R[n] >> 31;
  R[n] = R[n] << 1 | R[n] >> 31;
}

//ROTR Rn
auto SH2::ROTR(u32 n) -> void {
  SR.T = R[n] & 1;
  R[n] = R[n] >> 1 | R[n] << 31;
}

//RTE
auto SH2::RTE() -> void {
  delaySlot(readLong(SP + 0));
  SR  = readLong(SP + 4);
  SP += 8;
}

//RTS
auto SH2::RTS() -> void {
  delaySlot(PR + 4);
}

//SETT
auto SH2::SETT() -> void {
  SR.T = 1;
}

//SHAL Rn
auto SH2::SHAL(u32 n) -> void {
  SR.T = R[n] >> 31;
  R[n] <<= 1;
}

//SHAR Rn
auto SH2::SHAR(u32 n) -> void {
  SR.T = R[n] & 1;
  R[n] = (s32)R[n] >> 1;
}

//SHLL Rn
auto SH2::SHLL(u32 n) -> void {
  SR.T = R[n] >> 31;
  R[n] <<= 1;
}

//SHLL2 Rn
auto SH2::SHLL2(u32 n) -> void {
  R[n] <<= 2;
}

//SHLL8 Rn
auto SH2::SHLL8(u32 n) -> void {
  R[n] <<= 8;
}

//SHLL16 Rn
auto SH2::SHLL16(u32 n) -> void {
  R[n] <<= 16;
}

//SHLR Rn
auto SH2::SHLR(u32 n) -> void {
  SR.T = R[n] & 1;
  R[n] >>= 1;
}

//SHLR2 Rn
auto SH2::SHLR2(u32 n) -> void {
  R[n] >>= 2;
}

//SHLR8 Rn
auto SH2::SHLR8(u32 n) -> void {
  R[n] >>= 8;
}

//SHLR16 Rn
auto SH2::SHLR16(u32 n) -> void {
  R[n] >>= 16;
}

//SLEEP
auto SH2::SLEEP() -> void {
  if(!ET) {
    PC -= 2;
  } else {
    ET = 0;
  }
}

//STC SR,Rn
auto SH2::STCSR(u32 n) -> void {
  R[n] = SR;
  ID = 1;
}

//STC GBR,Rn
auto SH2::STCGBR(u32 n) -> void {
  R[n] = GBR;
  ID = 1;
}

//STC VBR,Rn
auto SH2::STCVBR(u32 n) -> void {
  R[n] = VBR;
  ID = 1;
}

//STC.L SR,@-Rn
auto SH2::STCMSR(u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], SR);
  ID = 1;
}

//STC.L GBR,@-Rn
auto SH2::STCMGBR(u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], GBR);
  ID = 1;
}

//STC.L VBR,@-Rn
auto SH2::STCMVBR(u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], VBR);
  ID = 1;
}

//STS MACH,Rn
auto SH2::STSMACH(u32 n) -> void {
  R[n] = MACH;
  ID = 1;
}

//STS MACL,Rn
auto SH2::STSMACL(u32 n) -> void {
  R[n] = MACL;
  ID = 1;
}

//STS PR,Rn
auto SH2::STSPR(u32 n) -> void {
  R[n] = PR;
  ID = 1;
}

//STS.L MACH,@-Rn
auto SH2::STSMMACH(u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], MACH);
  ID = 1;
}

//STS.L MACL,@-Rn
auto SH2::STSMMACL(u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], MACL);
  ID = 1;
}

//STS.L PR,@-Rn
auto SH2::STSMPR(u32 n) -> void {
  R[n] -= 4;
  writeLong(R[n], PR);
  ID = 1;
}

//SUB Rm,Rn
auto SH2::SUB(u32 m, u32 n) -> void {
  R[n] -= R[m];
}

//SUBC Rm,Rn
auto SH2::SUBC(u32 m, u32 n) -> void {
  u32 a = R[n];
  u32 b = a - R[m];
  R[n] = b - SR.T;
  SR.T = a < b || b < R[n];
}

//SUBV Rm,Rn
auto SH2::SUBV(u32 m, u32 n) -> void {
  u32 d = ((s32)R[n] < 0);
  u32 s = ((s32)R[m] < 0) + d;
  R[n] -= R[m];
  u32 r = ((s32)R[n] < 0) + d;
  SR.T = s == 1 ? r == 1 : 0;
}

//SWAP.B Rm,Rn
auto SH2::SWAPB(u32 m, u32 n) -> void {
  R[n] = R[m] & ~0xffff | R[m] >> 8 & 0x00ff | R[m] << 8 & 0xff00;
}

//SWAP.W Rm,Rn
auto SH2::SWAPW(u32 m, u32 n) -> void {
  R[n] = R[m] << 16 | R[m] >> 16;
}

//TAS.B @Rn
auto SH2::TAS(u32 n) -> void {
  if constexpr(Accuracy::AddressErrors) {
    if(R[n] >> 29 == Area::Purge
    || R[n] >> 29 == Area::Address
    || R[n] >> 29 == Area::Data
    || R[n] >> 29 == Area::IO
    ) return (void)(exceptions |= AddressErrorCPU);
  }

  auto cacheEnable = cache.enable;
  cache.enable = 0;
  u8 b = readByte(R[n]);
  cache.enable = cacheEnable;
  SR.T = b == 0;
  writeByte(R[n], b | 0x80);
}

//TRAPA #imm
auto SH2::TRAPA(u32 i) -> void {
  push(SR);
  push(PC + 2);
  branch(readLong(VBR + i * 4) + 4);
}

//TST Rm,Rn
auto SH2::TST(u32 m, u32 n) -> void {
  SR.T = (R[n] & R[m]) == 0;
}

//TST #imm,R0
auto SH2::TSTI(u32 i) -> void {
  SR.T = (R[0] & i) == 0;
}

//TST.B #imm,@(R0,GBR)
auto SH2::TSTM(u32 i) -> void {
  u8 b = readByte(GBR + R[0]);
  SR.T = (b & i) == 0;
}

//XOR Rm,Rn
auto SH2::XOR(u32 m, u32 n) -> void {
  R[n] ^= R[m];
}

//XOR #imm,R0
auto SH2::XORI(u32 i) -> void {
  R[0] ^= i;
}

//XOR.B #imm,@(R0,GBR)
auto SH2::XORM(u32 i) -> void {
  u8 b = readByte(GBR + R[0]);
  writeByte(GBR + R[0], b ^ i);
}

//XTRCT Rm,Rn
auto SH2::XTRCT(u32 m, u32 n) -> void {
  R[n] = R[n] >> 16 | R[m] << 16;
}
