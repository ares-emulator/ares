#define jp(id, name, ...) case id: return decoder##name##Info(instruction)
#define op(id, name, ...) case id: { OpInfo info = {}; __VA_ARGS__; return info; }

#define Branch                            info.flags |= OpInfo::Branch
#define LikelyBranch                      info.flags |= OpInfo::LikelyBranch
#define JitStateKeyMayChange              info.flags |= OpInfo::JitStateKeyMayChange
#define CountCompareWrite                 info.flags |= OpInfo::CountCompareWrite
#define UnconditionalJump                 info.flags |= OpInfo::UnconditionalJump
#define UnconditionalJumpAndLink          info.flags |= OpInfo::UnconditionalJumpAndLink
#define LikelyIf(x)                       if(x) LikelyBranch
#define WritesGpSp(n)                     (((n) == 28 || (n) == 29) \
                                            ? (JitStateKeyMayChange) : info.flags)
#define WritesGpSpRt                      WritesGpSp(instruction >> 16 & 31)
#define WritesGpSpRtExceptSpSelf          (((instruction >> 16 & 31) == 28 \
                                            || ((instruction >> 16 & 31) == 29 \
                                            && (instruction >> 21 & 31) != 29)) \
                                            ? (JitStateKeyMayChange) : info.flags)
#define WritesGpSpRd                      WritesGpSp(instruction >> 11 & 31)
#define WritesGpSpXrt                     WritesGpSp(instruction >> 15 & 31)
#define CountCompareWriteRd               ((((instruction >> 11) & 31) == 9 || ((instruction >> 11) & 31) == 11) \
                                            ? (CountCompareWrite) : 0)

auto CPU::decoderEXECUTEInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 26) {
  jp(0x00, SPECIAL);
  jp(0x01, REGIMM);
  op(0x02, J, Branch, UnconditionalJump);
  op(0x03, JAL, Branch, UnconditionalJump, UnconditionalJumpAndLink);
  op(0x04, BEQ, Branch);
  op(0x05, BNE, Branch);
  op(0x06, BLEZ, Branch);
  op(0x07, BGTZ, Branch);
  op(0x08, ADDI, WritesGpSpRt);
  op(0x09, ADDIU, WritesGpSpRtExceptSpSelf);
  op(0x0a, SLTI, WritesGpSpRt);
  op(0x0b, SLTIU, WritesGpSpRt);
  op(0x0c, ANDI, WritesGpSpRt);
  op(0x0d, ORI, WritesGpSpRt);
  op(0x0e, XORI, WritesGpSpRt);
  op(0x0f, LUI, WritesGpSpRt);
  jp(0x10, SCC);
  jp(0x11, FPU);
  jp(0x12, COP2);
  op(0x13, COP3);
  op(0x14, BEQL, Branch, LikelyBranch);
  op(0x15, BNEL, Branch, LikelyBranch);
  op(0x16, BLEZL, Branch, LikelyBranch);
  op(0x17, BGTZL, Branch, LikelyBranch);
  op(0x18, DADDI, WritesGpSpRt);
  op(0x19, DADDIU, WritesGpSpRtExceptSpSelf);
  op(0x1a, LDL, WritesGpSpRt);
  op(0x1b, LDR, WritesGpSpRt);
  op(0x1c, INVALID);
  op(0x1d, INVALID);
  op(0x1e, INVALID);
  op(0x1f, INVALID);
  op(0x20, LB, WritesGpSpRt);
  op(0x21, LH, WritesGpSpRt);
  op(0x22, LWL, WritesGpSpRt);
  op(0x23, LW, WritesGpSpRt);
  op(0x24, LBU, WritesGpSpRt);
  op(0x25, LHU, WritesGpSpRt);
  op(0x26, LWR, WritesGpSpRt);
  op(0x27, LWU, WritesGpSpRt);
  op(0x28, SB);
  op(0x29, SH);
  op(0x2a, SWL);
  op(0x2b, SW);
  op(0x2c, SDL);
  op(0x2d, SDR);
  op(0x2e, SWR);
  op(0x2f, CACHE);
  op(0x30, LL, WritesGpSpRt);
  op(0x31, LWC1);
  op(0x32, LWC2);
  op(0x33, LWC3);
  op(0x34, LLD, WritesGpSpRt);
  op(0x35, LDC1);
  op(0x36, LDC2);
  op(0x37, LD, WritesGpSpRt);
  op(0x38, SC, WritesGpSpRt);
  op(0x39, SWC1);
  op(0x3a, SWC2);
  op(0x3b, SWC3);
  op(0x3c, SCD, WritesGpSpRt);
  op(0x3d, SDC1);
  op(0x3e, SDC2);
  op(0x3f, SD);
  }
  return {};
}

auto CPU::decoderSPECIALInfo(u32 instruction) const -> OpInfo {
  switch(instruction & 0x3f) {
  op(0x00, SLL, WritesGpSpRd);
  op(0x01, INVALID);
  op(0x02, SRL, WritesGpSpRd);
  op(0x03, SRA, WritesGpSpRd);
  op(0x04, SLLV, WritesGpSpRd);
  op(0x05, INVALID);
  op(0x06, SRLV, WritesGpSpRd);
  op(0x07, SRAV, WritesGpSpRd);
  op(0x08, JR, Branch, UnconditionalJump);
  op(0x09, JALR, Branch, UnconditionalJump, UnconditionalJumpAndLink, WritesGpSpRd);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  op(0x0c, SYSCALL);
  op(0x0d, BREAK);
  op(0x0e, INVALID);
  op(0x0f, SYNC);
  op(0x10, MFHI, WritesGpSpRd);
  op(0x11, MTHI);
  op(0x12, MFLO, WritesGpSpRd);
  op(0x13, MTLO);
  op(0x14, DSLLV, WritesGpSpRd);
  op(0x15, INVALID);
  op(0x16, DSRLV, WritesGpSpRd);
  op(0x17, DSRAV, WritesGpSpRd);
  op(0x18, MULT);
  op(0x19, MULTU);
  op(0x1a, DIV);
  op(0x1b, DIVU);
  op(0x1c, DMULT);
  op(0x1d, DMULTU);
  op(0x1e, DDIV);
  op(0x1f, DDIVU);
  op(0x20, ADD, WritesGpSpRd);
  op(0x21, ADDU, WritesGpSpRd);
  op(0x22, SUB, WritesGpSpRd);
  op(0x23, SUBU, WritesGpSpRd);
  op(0x24, AND, WritesGpSpRd);
  op(0x25, OR, WritesGpSpRd);
  op(0x26, XOR, WritesGpSpRd);
  op(0x27, NOR, WritesGpSpRd);
  op(0x28, INVALID);
  op(0x29, INVALID);
  op(0x2a, SLT, WritesGpSpRd);
  op(0x2b, SLTU, WritesGpSpRd);
  op(0x2c, DADD, WritesGpSpRd);
  op(0x2d, DADDU, WritesGpSpRd);
  op(0x2e, DSUB, WritesGpSpRd);
  op(0x2f, DSUBU, WritesGpSpRd);
  op(0x30, TGE);
  op(0x31, TGEU);
  op(0x32, TLT);
  op(0x33, TLTU);
  op(0x34, TEQ);
  op(0x35, INVALID);
  op(0x36, TNE);
  op(0x37, INVALID);
  op(0x38, DSLL, WritesGpSpRd);
  op(0x39, INVALID);
  op(0x3a, DSRL, WritesGpSpRd);
  op(0x3b, DSRA, WritesGpSpRd);
  op(0x3c, DSLL32, WritesGpSpRd);
  op(0x3d, INVALID);
  op(0x3e, DSRL32, WritesGpSpRd);
  op(0x3f, DSRA32, WritesGpSpRd);
  }
  return {};
}

auto CPU::decoderREGIMMInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 16 & 0x1f) {
  op(0x00, BLTZ, Branch);
  op(0x01, BGEZ, Branch);
  op(0x02, BLTZL, Branch, LikelyBranch);
  op(0x03, BGEZL, Branch, LikelyBranch);
  op(0x04, INVALID);
  op(0x05, INVALID);
  op(0x06, INVALID);
  op(0x07, INVALID);
  op(0x08, TGEI);
  op(0x09, TGEIU);
  op(0x0a, TLTI);
  op(0x0b, TLTIU);
  op(0x0c, TEQI);
  op(0x0d, INVALID);
  op(0x0e, TNEI);
  op(0x0f, INVALID);
  op(0x10, BLTZAL, Branch);
  op(0x11, BGEZAL, Branch);
  op(0x12, BLTZALL, Branch, LikelyBranch);
  op(0x13, BGEZALL, Branch, LikelyBranch);
  op(0x14, INVALID);
  op(0x15, INVALID);
  op(0x16, INVALID);
  op(0x17, INVALID);
  op(0x18, INVALID);
  op(0x19, INVALID);
  op(0x1a, INVALID);
  op(0x1b, INVALID);
  op(0x1c, INVALID);
  op(0x1d, INVALID);
  op(0x1e, INVALID);
  op(0x1f, INVALID);
  }
  return {};
}

auto CPU::decoderSCCInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 21 & 0x1f) {
  op(0x00, MFC0, WritesGpSpRt);
  op(0x01, DMFC0, WritesGpSpRt);
  op(0x02, INVALID);
  op(0x03, INVALID);
  op(0x04, MTC0, JitStateKeyMayChange, CountCompareWriteRd);
  op(0x05, DMTC0, JitStateKeyMayChange, CountCompareWriteRd);
  op(0x06, INVALID);
  op(0x07, INVALID);
  op(0x08, INVALID);
  op(0x09, INVALID);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  op(0x0c, INVALID);
  op(0x0d, INVALID);
  op(0x0e, INVALID);
  op(0x0f, INVALID);
  }

  switch(instruction & 0x3f) {
  op(0x01, TLBR);
  op(0x02, TLBWI);
  op(0x06, TLBWR);
  op(0x08, TLBP);
  op(0x18, ERET, JitStateKeyMayChange);
  op(0x20, XDETECT);
  op(0x25, XLOG);
  op(0x27, XHEXDUMP);
  op(0x28, XPROF);
  op(0x29, XPROFREAD);
  op(0x2a, XEXCEPTION);
  op(0x2c, XIOCTL);
  }

  return {};
}

auto CPU::decoderFPUInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 21 & 0x1f) {
  op(0x00, MFC1, WritesGpSpRt);
  op(0x01, DMFC1, WritesGpSpRt);
  op(0x02, CFC1, WritesGpSpRt);
  op(0x03, DCFC1);
  op(0x04, MTC1);
  op(0x05, DMTC1);
  op(0x06, CTC1, JitStateKeyMayChange);
  op(0x07, DCTC1);
  case 0x08: {
    OpInfo info = {};
    Branch;
    LikelyIf(instruction >> 17 & 1);
    return info;
  }
  op(0x09, INVALID);
  op(0x0a, INVALID);
  op(0x0b, INVALID);
  op(0x0c, INVALID);
  op(0x0d, INVALID);
  op(0x0e, INVALID);
  op(0x0f, INVALID);
  }

  if((instruction >> 21 & 31) == 16)
  switch(instruction & 0x3f) {
  op(0x00, FADD_S);
  op(0x01, FSUB_S);
  op(0x02, FMUL_S);
  op(0x03, FDIV_S);
  op(0x04, FSQRT_S);
  op(0x05, FABS_S);
  op(0x06, FMOV_S);
  op(0x07, FNEG_S);
  op(0x08, FROUND_L_S);
  op(0x09, FTRUNC_L_S);
  op(0x0a, FCEIL_L_S);
  op(0x0b, FFLOOR_L_S);
  op(0x0c, FROUND_W_S);
  op(0x0d, FTRUNC_W_S);
  op(0x0e, FCEIL_W_S);
  op(0x0f, FFLOOR_W_S);
  op(0x20, FCVT_S_S);
  op(0x21, FCVT_D_S);
  op(0x24, FCVT_W_S);
  op(0x25, FCVT_L_S);
  op(0x30, FC_F_S);
  op(0x31, FC_UN_S);
  op(0x32, FC_EQ_S);
  op(0x33, FC_UEQ_S);
  op(0x34, FC_OLT_S);
  op(0x35, FC_ULT_S);
  op(0x36, FC_OLE_S);
  op(0x37, FC_ULE_S);
  op(0x38, FC_SF_S);
  op(0x39, FC_NGLE_S);
  op(0x3a, FC_SEQ_S);
  op(0x3b, FC_NGL_S);
  op(0x3c, FC_LT_S);
  op(0x3d, FC_NGE_S);
  op(0x3e, FC_LE_S);
  op(0x3f, FC_NGT_S);
  }

  if((instruction >> 21 & 31) == 17)
  switch(instruction & 0x3f) {
  op(0x00, FADD_D);
  op(0x01, FSUB_D);
  op(0x02, FMUL_D);
  op(0x03, FDIV_D);
  op(0x04, FSQRT_D);
  op(0x05, FABS_D);
  op(0x06, FMOV_D);
  op(0x07, FNEG_D);
  op(0x08, FROUND_L_D);
  op(0x09, FTRUNC_L_D);
  op(0x0a, FCEIL_L_D);
  op(0x0b, FFLOOR_L_D);
  op(0x0c, FROUND_W_D);
  op(0x0d, FTRUNC_W_D);
  op(0x0e, FCEIL_W_D);
  op(0x0f, FFLOOR_W_D);
  op(0x20, FCVT_S_D);
  op(0x21, FCVT_D_D);
  op(0x24, FCVT_W_D);
  op(0x25, FCVT_L_D);
  op(0x30, FC_F_D);
  op(0x31, FC_UN_D);
  op(0x32, FC_EQ_D);
  op(0x33, FC_UEQ_D);
  op(0x34, FC_OLT_D);
  op(0x35, FC_ULT_D);
  op(0x36, FC_OLE_D);
  op(0x37, FC_ULE_D);
  op(0x38, FC_SF_D);
  op(0x39, FC_NGLE_D);
  op(0x3a, FC_SEQ_D);
  op(0x3b, FC_NGL_D);
  op(0x3c, FC_LT_D);
  op(0x3d, FC_NGE_D);
  op(0x3e, FC_LE_D);
  op(0x3f, FC_NGT_D);
  }

  if((instruction >> 21 & 31) == 20)
  switch(instruction & 0x3f) {
  op(0x08, FROUND_L_W);
  op(0x09, FTRUNC_L_W);
  op(0x0a, FCEIL_L_W);
  op(0x0b, FFLOOR_L_W);
  op(0x0c, FROUND_W_W);
  op(0x0d, FTRUNC_W_W);
  op(0x0e, FCEIL_W_W);
  op(0x0f, FFLOOR_W_W);
  op(0x20, FCVT_S_W);
  op(0x21, FCVT_D_W);
  op(0x24, FCVT_W_W);
  op(0x25, FCVT_L_W);
  }

  if((instruction >> 21 & 31) == 21)
  switch(instruction & 0x3f) {
  op(0x08, FROUND_L_L);
  op(0x09, FTRUNC_L_L);
  op(0x0a, FCEIL_L_L);
  op(0x0b, FFLOOR_L_L);
  op(0x0c, FROUND_W_L);
  op(0x0d, FTRUNC_W_L);
  op(0x0e, FCEIL_W_L);
  op(0x0f, FFLOOR_W_L);
  op(0x20, FCVT_S_L);
  op(0x21, FCVT_D_L);
  op(0x24, FCVT_W_L);
  op(0x25, FCVT_L_L);
  }

  return {};
}

auto CPU::decoderCOP2Info(u32 instruction) const -> OpInfo {
  switch(instruction >> 21 & 0x1f) {
  op(0x00, MFC2, WritesGpSpXrt);
  op(0x01, DMFC2, WritesGpSpXrt);
  op(0x02, CFC2, WritesGpSpXrt);
  op(0x03, COP2INVALID);
  op(0x04, MTC2);
  op(0x05, DMTC2);
  op(0x06, CTC2);
  op(0x07, COP2INVALID);
  op(0x08, COP2INVALID);
  op(0x09, COP2INVALID);
  op(0x0a, COP2INVALID);
  op(0x0b, COP2INVALID);
  op(0x0c, COP2INVALID);
  op(0x0d, COP2INVALID);
  op(0x0e, COP2INVALID);
  op(0x0f, COP2INVALID);
  }

  return {};
}

#undef jp
#undef op
#undef Branch
#undef LikelyBranch
#undef JitStateKeyMayChange
#undef CountCompareWrite
#undef UnconditionalJump
#undef UnconditionalJumpAndLink
#undef LikelyIf
#undef WritesGpSp
#undef WritesGpSpRt
#undef WritesGpSpRtExceptSpSelf
#undef WritesGpSpRd
#undef WritesGpSpXrt
#undef CountCompareWriteRd
