#define jp(id, name, ...) case id: return decoder##name##Info(instruction)
#define op(id, name, ...) case id: { OpInfo info = {}; __VA_ARGS__; return info; }

#define Branch                            info.flags |= OpInfo::Branch
#define EndBlock                          info.flags |= OpInfo::EndBlock
#define LikelyBranch                      info.flags |= OpInfo::LikelyBranch
#define MayTrap                           info.flags |= OpInfo::MayTrap
#define MayException                      info.flags |= OpInfo::MayException
#define MayFault                          info.flags |= OpInfo::MayFault
#define Load                              info.flags |= OpInfo::Load
#define Store                             info.flags |= OpInfo::Store
#define Cop0                              info.flags |= OpInfo::Cop0
#define Cop1                              info.flags |= OpInfo::Cop1
#define Cop2                              info.flags |= OpInfo::Cop2
#define ReadsHiLo                         info.flags |= OpInfo::ReadsHiLo
#define WritesHiLo                        info.flags |= OpInfo::WritesHiLo
#define Privileged                        info.flags |= OpInfo::Privileged
#define IsInvalid                         info.flags |= OpInfo::IsInvalid
#define JitMayCallf                       info.flags |= OpInfo::JitMayCallf
#define JitMustFlushBeforeCall            info.flags |= OpInfo::JitMustFlushBeforeCall
#define JitAddsExtraCyclesInternally      info.flags |= OpInfo::JitAddsExtraCyclesInternally
#define JitStateKeyMayChange              info.flags |= OpInfo::JitStateKeyMayChange
#define LikelyIf(x)                       if(x) LikelyBranch
#define FPUCall                           Cop1, MayException, JitMayCallf, JitAddsExtraCyclesInternally
#define FPUBranchCall                     \
  Cop1, Branch, EndBlock, MayException, JitMayCallf, JitAddsExtraCyclesInternally
#define FPUInvalid                        Cop1, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall
#define COP2Call                          Cop2, MayException, JitMayCallf, JitMustFlushBeforeCall
#define COP2Invalid                       Cop2, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall

auto CPU::decoderEXECUTEInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 26) {
  jp(0x00, SPECIAL);
  jp(0x01, REGIMM);
  op(0x02, J, Branch, EndBlock);
  op(0x03, JAL, Branch, EndBlock);
  op(0x04, BEQ, Branch, EndBlock);
  op(0x05, BNE, Branch, EndBlock);
  op(0x06, BLEZ, Branch, EndBlock);
  op(0x07, BGTZ, Branch, EndBlock);
  op(0x08, ADDI, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x09, ADDIU);
  op(0x0a, SLTI);
  op(0x0b, SLTIU);
  op(0x0c, ANDI);
  op(0x0d, ORI);
  op(0x0e, XORI);
  op(0x0f, LUI);
  jp(0x10, SCC);
  jp(0x11, FPU);
  jp(0x12, COP2);
  op(0x13, COP3, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x14, BEQL, Branch, LikelyBranch, EndBlock);
  op(0x15, BNEL, Branch, LikelyBranch, EndBlock);
  op(0x16, BLEZL, Branch, LikelyBranch, EndBlock);
  op(0x17, BGTZL, Branch, LikelyBranch, EndBlock);
  op(0x18, DADDI, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x19, DADDIU, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1a, LDL, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1b, LDR, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1c, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1d, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1e, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1f, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x20, LB, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x21, LH, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x22, LWL, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x23, LW, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x24, LBU, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x25, LHU, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x26, LWR, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x27, LWU, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x28, SB, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x29, SH, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2a, SWL, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2b, SW, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2c, SDL, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2d, SDR, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2e, SWR, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2f, CACHE, Store, MayException, MayFault, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x30, LL, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x31, LWC1, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x32, LWC2, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x33, LWC3, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x34, LLD, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x35, LDC1, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x36, LDC2, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x37, LD, Load, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x38, SC, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x39, SWC1, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3a, SWC2, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3b, SWC3, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3c, SCD, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3d, SDC1, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3e, SDC2, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3f, SD, Store, MayException, MayFault, JitMayCallf, JitMustFlushBeforeCall);
  }
  return {};
}

auto CPU::decoderSPECIALInfo(u32 instruction) const -> OpInfo {
  switch(instruction & 0x3f) {
  op(0x00, SLL);
  op(0x01, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x02, SRL);
  op(0x03, SRA);
  op(0x04, SLLV);
  op(0x05, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x06, SRLV);
  op(0x07, SRAV);
  op(0x08, JR, Branch, EndBlock);
  op(0x09, JALR, Branch, EndBlock);
  op(0x0a, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0b, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0c, SYSCALL, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0d, BREAK, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0e, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0f, SYNC, JitMayCallf, JitMustFlushBeforeCall);
  op(0x10, MFHI, ReadsHiLo);
  op(0x11, MTHI, WritesHiLo);
  op(0x12, MFLO, ReadsHiLo);
  op(0x13, MTLO, WritesHiLo);
  op(0x14, DSLLV, JitMayCallf, JitMustFlushBeforeCall);
  op(0x15, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x16, DSRLV, JitMayCallf, JitMustFlushBeforeCall);
  op(0x17, DSRAV, JitMayCallf, JitMustFlushBeforeCall);
  op(0x18, MULT, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x19, MULTU, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x1a, DIV, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x1b, DIVU, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x1c, DMULT, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x1d, DMULTU, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x1e, DDIV, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x1f, DDIVU, WritesHiLo, JitMayCallf, JitAddsExtraCyclesInternally);
  op(0x20, ADD, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x21, ADDU);
  op(0x22, SUB, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x23, SUBU);
  op(0x24, AND);
  op(0x25, OR);
  op(0x26, XOR);
  op(0x27, NOR);
  op(0x28, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x29, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2a, SLT);
  op(0x2b, SLTU);
  op(0x2c, DADD, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2d, DADDU, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2e, DSUB, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2f, DSUBU, JitMayCallf, JitMustFlushBeforeCall);
  op(0x30, TGE, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x31, TGEU, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x32, TLT, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x33, TLTU, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x34, TEQ, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x35, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x36, TNE, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x37, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x38, DSLL, JitMayCallf, JitMustFlushBeforeCall);
  op(0x39, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3a, DSRL, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3b, DSRA, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3c, DSLL32, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3d, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3e, DSRL32, JitMayCallf, JitMustFlushBeforeCall);
  op(0x3f, DSRA32, JitMayCallf, JitMustFlushBeforeCall);
  }
  return {};
}

auto CPU::decoderREGIMMInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 16 & 0x1f) {
  op(0x00, BLTZ, Branch, EndBlock);
  op(0x01, BGEZ, Branch, EndBlock);
  op(0x02, BLTZL, Branch, LikelyBranch, EndBlock);
  op(0x03, BGEZL, Branch, LikelyBranch, EndBlock);
  op(0x04, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x05, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x06, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x07, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x08, TGEI, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x09, TGEIU, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0a, TLTI, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0b, TLTIU, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0c, TEQI, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0d, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0e, TNEI, MayTrap, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0f, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x10, BLTZAL, Branch, EndBlock);
  op(0x11, BGEZAL, Branch, EndBlock);
  op(0x12, BLTZALL, Branch, LikelyBranch, EndBlock);
  op(0x13, BGEZALL, Branch, LikelyBranch, EndBlock);
  op(0x14, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x15, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x16, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x17, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x18, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x19, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1a, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1b, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1c, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1d, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1e, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x1f, INVALID, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  }
  return {};
}

auto CPU::decoderSCCInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 21 & 0x1f) {
  op(0x00, MFC0, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x01, DMFC0, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x02, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x03, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x04, MTC0, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall,
     JitStateKeyMayChange);
  op(0x05, DMTC0, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall,
     JitStateKeyMayChange);
  op(0x06, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x07, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x08, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x09, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0a, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0b, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0c, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0d, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0e, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x0f, INVALID, Cop0, Privileged, IsInvalid, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall);
  }

  switch(instruction & 0x3f) {
  op(0x01, TLBR, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x02, TLBWI, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x06, TLBWR, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x08, TLBP, Cop0, Privileged, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x18, ERET, Cop0, Privileged, EndBlock, MayException, JitMayCallf, JitMustFlushBeforeCall,
     JitStateKeyMayChange);
  op(0x20, XDETECT, JitMayCallf, JitMustFlushBeforeCall);
  op(0x25, XLOG, JitMayCallf, JitMustFlushBeforeCall);
  op(0x27, XHEXDUMP, JitMayCallf, JitMustFlushBeforeCall);
  op(0x28, XPROF, JitMayCallf, JitMustFlushBeforeCall);
  op(0x29, XPROFREAD, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2a, XEXCEPTION, MayException, JitMayCallf, JitMustFlushBeforeCall);
  op(0x2c, XIOCTL, JitMayCallf, JitMustFlushBeforeCall);
  }

  return {};
}

auto CPU::decoderFPUInfo(u32 instruction) const -> OpInfo {
  switch(instruction >> 21 & 0x1f) {
  op(0x00, MFC1, FPUCall);
  op(0x01, DMFC1, FPUCall);
  op(0x02, CFC1, FPUCall);
  op(0x03, DCFC1, FPUInvalid);
  op(0x04, MTC1, FPUCall);
  op(0x05, DMTC1, FPUCall);
  op(0x06, CTC1, FPUCall, JitStateKeyMayChange);
  op(0x07, DCTC1, FPUInvalid);
  case 0x08: {
    OpInfo info = {};
    FPUBranchCall;
    LikelyIf(instruction >> 17 & 1);
    return info;
  }
  op(0x09, INVALID, FPUInvalid);
  op(0x0a, INVALID, FPUInvalid);
  op(0x0b, INVALID, FPUInvalid);
  op(0x0c, INVALID, FPUInvalid);
  op(0x0d, INVALID, FPUInvalid);
  op(0x0e, INVALID, FPUInvalid);
  op(0x0f, INVALID, FPUInvalid);
  }

  if((instruction >> 21 & 31) == 16)
  switch(instruction & 0x3f) {
  op(0x00, FADD_S, FPUCall);
  op(0x01, FSUB_S, FPUCall);
  op(0x02, FMUL_S, FPUCall);
  op(0x03, FDIV_S, FPUCall);
  op(0x04, FSQRT_S, FPUCall);
  op(0x05, FABS_S, FPUCall);
  op(0x06, FMOV_S, FPUCall);
  op(0x07, FNEG_S, FPUCall);
  op(0x08, FROUND_L_S, FPUCall);
  op(0x09, FTRUNC_L_S, FPUCall);
  op(0x0a, FCEIL_L_S, FPUCall);
  op(0x0b, FFLOOR_L_S, FPUCall);
  op(0x0c, FROUND_W_S, FPUCall);
  op(0x0d, FTRUNC_W_S, FPUCall);
  op(0x0e, FCEIL_W_S, FPUCall);
  op(0x0f, FFLOOR_W_S, FPUCall);
  op(0x20, FCVT_S_S, FPUCall);
  op(0x21, FCVT_D_S, FPUCall);
  op(0x24, FCVT_W_S, FPUCall);
  op(0x25, FCVT_L_S, FPUCall);
  op(0x30, FC_F_S, FPUCall);
  op(0x31, FC_UN_S, FPUCall);
  op(0x32, FC_EQ_S, FPUCall);
  op(0x33, FC_UEQ_S, FPUCall);
  op(0x34, FC_OLT_S, FPUCall);
  op(0x35, FC_ULT_S, FPUCall);
  op(0x36, FC_OLE_S, FPUCall);
  op(0x37, FC_ULE_S, FPUCall);
  op(0x38, FC_SF_S, FPUCall);
  op(0x39, FC_NGLE_S, FPUCall);
  op(0x3a, FC_SEQ_S, FPUCall);
  op(0x3b, FC_NGL_S, FPUCall);
  op(0x3c, FC_LT_S, FPUCall);
  op(0x3d, FC_NGE_S, FPUCall);
  op(0x3e, FC_LE_S, FPUCall);
  op(0x3f, FC_NGT_S, FPUCall);
  }

  if((instruction >> 21 & 31) == 17)
  switch(instruction & 0x3f) {
  op(0x00, FADD_D, FPUCall);
  op(0x01, FSUB_D, FPUCall);
  op(0x02, FMUL_D, FPUCall);
  op(0x03, FDIV_D, FPUCall);
  op(0x04, FSQRT_D, FPUCall);
  op(0x05, FABS_D, FPUCall);
  op(0x06, FMOV_D, FPUCall);
  op(0x07, FNEG_D, FPUCall);
  op(0x08, FROUND_L_D, FPUCall);
  op(0x09, FTRUNC_L_D, FPUCall);
  op(0x0a, FCEIL_L_D, FPUCall);
  op(0x0b, FFLOOR_L_D, FPUCall);
  op(0x0c, FROUND_W_D, FPUCall);
  op(0x0d, FTRUNC_W_D, FPUCall);
  op(0x0e, FCEIL_W_D, FPUCall);
  op(0x0f, FFLOOR_W_D, FPUCall);
  op(0x20, FCVT_S_D, FPUCall);
  op(0x21, FCVT_D_D, FPUCall);
  op(0x24, FCVT_W_D, FPUCall);
  op(0x25, FCVT_L_D, FPUCall);
  op(0x30, FC_F_D, FPUCall);
  op(0x31, FC_UN_D, FPUCall);
  op(0x32, FC_EQ_D, FPUCall);
  op(0x33, FC_UEQ_D, FPUCall);
  op(0x34, FC_OLT_D, FPUCall);
  op(0x35, FC_ULT_D, FPUCall);
  op(0x36, FC_OLE_D, FPUCall);
  op(0x37, FC_ULE_D, FPUCall);
  op(0x38, FC_SF_D, FPUCall);
  op(0x39, FC_NGLE_D, FPUCall);
  op(0x3a, FC_SEQ_D, FPUCall);
  op(0x3b, FC_NGL_D, FPUCall);
  op(0x3c, FC_LT_D, FPUCall);
  op(0x3d, FC_NGE_D, FPUCall);
  op(0x3e, FC_LE_D, FPUCall);
  op(0x3f, FC_NGT_D, FPUCall);
  }

  if((instruction >> 21 & 31) == 20)
  switch(instruction & 0x3f) {
  op(0x08, FROUND_L_W, FPUInvalid);
  op(0x09, FTRUNC_L_W, FPUInvalid);
  op(0x0a, FCEIL_L_W, FPUInvalid);
  op(0x0b, FFLOOR_L_W, FPUInvalid);
  op(0x0c, FROUND_W_W, FPUInvalid);
  op(0x0d, FTRUNC_W_W, FPUInvalid);
  op(0x0e, FCEIL_W_W, FPUInvalid);
  op(0x0f, FFLOOR_W_W, FPUInvalid);
  op(0x20, FCVT_S_W, FPUCall);
  op(0x21, FCVT_D_W, FPUCall);
  op(0x24, FCVT_W_W, FPUInvalid);
  op(0x25, FCVT_L_W, FPUInvalid);
  }

  if((instruction >> 21 & 31) == 21)
  switch(instruction & 0x3f) {
  op(0x08, FROUND_L_L, FPUInvalid);
  op(0x09, FTRUNC_L_L, FPUInvalid);
  op(0x0a, FCEIL_L_L, FPUInvalid);
  op(0x0b, FFLOOR_L_L, FPUInvalid);
  op(0x0c, FROUND_W_L, FPUInvalid);
  op(0x0d, FTRUNC_W_L, FPUInvalid);
  op(0x0e, FCEIL_W_L, FPUInvalid);
  op(0x0f, FFLOOR_W_L, FPUInvalid);
  op(0x20, FCVT_S_L, FPUCall);
  op(0x21, FCVT_D_L, FPUCall);
  op(0x24, FCVT_W_L, FPUInvalid);
  op(0x25, FCVT_L_L, FPUInvalid);
  }

  return {};
}

auto CPU::decoderCOP2Info(u32 instruction) const -> OpInfo {
  switch(instruction >> 21 & 0x1f) {
  op(0x00, MFC2, COP2Call);
  op(0x01, DMFC2, COP2Call);
  op(0x02, CFC2, COP2Call);
  op(0x03, COP2INVALID, COP2Invalid);
  op(0x04, MTC2, COP2Call);
  op(0x05, DMTC2, COP2Call);
  op(0x06, CTC2, COP2Call);
  op(0x07, COP2INVALID, COP2Invalid);
  op(0x08, COP2INVALID, COP2Invalid);
  op(0x09, COP2INVALID, COP2Invalid);
  op(0x0a, COP2INVALID, COP2Invalid);
  op(0x0b, COP2INVALID, COP2Invalid);
  op(0x0c, COP2INVALID, COP2Invalid);
  op(0x0d, COP2INVALID, COP2Invalid);
  op(0x0e, COP2INVALID, COP2Invalid);
  op(0x0f, COP2INVALID, COP2Invalid);
  }

  return {};
}

#undef jp
#undef op
#undef Branch
#undef EndBlock
#undef LikelyBranch
#undef MayTrap
#undef MayException
#undef MayFault
#undef Load
#undef Store
#undef Cop0
#undef Cop1
#undef Cop2
#undef ReadsHiLo
#undef WritesHiLo
#undef Privileged
#undef IsInvalid
#undef JitMayCallf
#undef JitMustFlushBeforeCall
#undef JitAddsExtraCyclesInternally
#undef JitStateKeyMayChange
#undef LikelyIf
#undef FPUCall
#undef FPUBranchCall
#undef FPUInvalid
#undef COP2Call
#undef COP2Invalid
