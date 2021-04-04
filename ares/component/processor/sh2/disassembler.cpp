template<typename... P>
auto SH2::hint(P&&... p) const -> string {
  if(1) return {};
  if(1) return {"\e[0m\e[37m", forward<P>(p)..., "\e[0m"};
  return {forward<P>(p)...};
}

auto SH2::disassembleInstruction() -> string {
  vector<string> s;

  auto registerName = [&](u32 r) -> string {
    return {"r", r};
  };
  auto registerValue = [&](u32 r) -> string {
    return {"r", r, hint("{0x", hex(R[r], 8L), "}")};
  };
  auto indirectRegister = [&](u32 r) -> string {
    return {"@r", r, hint("{0x", hex(R[r], 8L), "}")};
  };
  auto predecrementIndirectRegister = [&](u32 r, u32 d) -> string {
    return {"@-r", r, hint("{0x", hex(R[r] - d, 8L), "}")};
  };
  auto postincrementIndirectRegister = [&](u32 r) -> string {
    return {"@r", r, "+", hint("{0x", hex(R[r], 8L), "}")};
  };
  auto indirectIndexedRegister = [&](u32 r) -> string {
    return {"@(r0,r", r, ")", hint("{0x", hex(R[0] + R[r], 8L), "}")};
  };
  auto immediate = [&](u32 i) -> string {
    return {"#0x", hex(i, 2L)};
  };
  auto pcRelativeDisplacement = [&](u32 d, u32 s) -> string {
    return {"@(0x", hex(d * s, 3L), ",pc)", hint("{0x", hex(PC + d * s, 8L), "}")};
  };
  auto gbrIndirectDisplacement = [&](u32 d, u32 s) -> string {
    return {"@(0x", hex(d * s, 3L), ",gbr)", hint("{0x", hex(GBR + d * s, 8L), "}")};
  };
  auto gbrIndirectIndexed = [&]() -> string {
    return {"@(r0,gbr)", hint("{0x", hex(R[0] + GBR, 8L), "}")};
  };
  auto indirectRegisterDisplacement = [&](u32 r, u32 d, u32 s) -> string {
    return {"@(0x", hex(d * s, 2L), ",r", r, ")", hint("{0x", hex(R[r] + d * s, 8L), "}")};
  };
  auto branch8 = [&](u32 d) -> string {
    return {"0x", hex(PC + (s8)d * 2, 8L)};
  };
  auto branch12 = [&](u32 d) -> string {
    return {"0x", hex(PC + (i12)d * 2, 8L)};
  };

  u16 opcode = readWord(PC - 4);

  #define n   (opcode >> 8 & 0x00f)
  #define m   (opcode >> 4 & 0x00f)
  #define i   (opcode >> 0 & 0x0ff)
  #define d4  (opcode >> 0 & 0x00f)
  #define d8  (opcode >> 0 & 0x0ff)
  #define d12 (opcode >> 0 & 0xfff)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {
  case 0x04:  //MOV.B Rm,@(R0,Rn)
    s = {"mov.b", registerValue(m), indirectIndexedRegister(n)};
    break;
  case 0x05:  //MOV.W Rm,@(R0,Rn)
    s = {"mov.w", registerValue(m), indirectIndexedRegister(n)};
    break;
  case 0x06:  //MOV.L Rm,@(R0,Rn)
    s = {"mov.l", registerValue(m), indirectIndexedRegister(n)};
    break;
  case 0x07:  //MUL.L Rm,Rn
    s = {"mul.l", registerValue(m), registerName(n)};
    break;
  case 0x0c:  //MOV.B @(R0,Rm),Rn
    s = {"mov.b", indirectIndexedRegister(m), registerName(n)};
    break;
  case 0x0d:  //MOV.W @(R0,Rm),Rn
    s = {"mov.w", indirectIndexedRegister(m), registerName(n)};
    break;
  case 0x0e:  //MOV.L @(R0,Rm),Rn
    s = {"mov.l", indirectIndexedRegister(m), registerName(n)};
    break;
  case 0x0f:  //MAC.L @Rm+,@Rn+
    s = {"mac.l", postincrementIndirectRegister(m), postincrementIndirectRegister(n)};
    break;
  case 0x10 ... 0x1f:  //MOV.L Rm,@(disp,Rn)
    s = {"mov.l", registerValue(m), indirectRegisterDisplacement(n, d4, 4)};
    break;
  case 0x20:  //MOV.B Rm,@Rn
    s = {"mov.b", registerValue(m), indirectRegister(n)};
    break;
  case 0x21:  //MOV.W Rm,@Rn
    s = {"mov.w", registerValue(m), indirectRegister(n)};
    break;
  case 0x22:  //MOV.L Rm,@Rn
    s = {"mov.l", registerValue(m), indirectRegister(n)};
    break;
  case 0x24:  //MOV.B Rm,@-Rn
    s = {"mov.b", registerValue(m), predecrementIndirectRegister(n, 1)};
    break;
  case 0x25:  //MOV.W Rm,@-Rn
    s = {"mov.w", registerValue(m), predecrementIndirectRegister(n, 2)};
    break;
  case 0x26:  //MOV.L Rm,@-Rn
    s = {"mov.l", registerValue(m), predecrementIndirectRegister(n, 4)};
    break;
  case 0x27:  //DIV0S Rm,Rn
    s = {"div0s", registerValue(m), registerValue(n)};
    break;
  case 0x28:  //TST Rm,Rn
    s = {"tst", registerValue(m), registerValue(n)};
    break;
  case 0x29:  //AND Rm,Rn
    s = {"and", registerValue(m), registerValue(n)};
    break;
  case 0x2a:  //XOR Rm,Rn
    s = {"xor", registerValue(m), registerValue(n)};
    break;
  case 0x2b:  //OR Rm,Rn
    s = {"or", registerValue(m), registerValue(n)};
    break;
  case 0x2c:  //CMP/STR Rm,Rn
    s = {"cmp/str", registerValue(m), registerValue(n)};
    break;
  case 0x2d:  //XTRCT Rm,Rn
    s = {"xtrct", registerValue(m), registerValue(n)};
    break;
  case 0x2e:  //MULU Rm,Rn
    s = {"mulu", registerValue(m), registerValue(n)};
    break;
  case 0x2f:  //MULS Rm,Rn
    s = {"muls", registerValue(m), registerValue(n)};
    break;
  case 0x30:  //CMP/EQ Rm,Rn
    s = {"cmp/eq", registerValue(m), registerValue(n)};
    break;
  case 0x32:  //CMP/HS Rm,Rn
    s = {"cmp/hs", registerValue(m), registerValue(n)};
    break;
  case 0x33:  //CMP/GE Rm,Rn
    s = {"cmp/ge", registerValue(m), registerValue(n)};
    break;
  case 0x34:  //DIV1 Rm,Rn
    s = {"div1", registerValue(m), registerValue(n)};
    break;
  case 0x35:  //DMULU.L Rm,Rn
    s = {"dmulu.l", registerValue(m), registerValue(n)};
    break;
  case 0x36:  //CMP/HI Rm,Rn
    s = {"cmp/hi", registerValue(m), registerValue(n)};
    break;
  case 0x37:  //CMP/GT Rm,Rn
    s = {"cmp/gt", registerValue(m), registerValue(n)};
    break;
  case 0x38:  //SUB Rm,Rn
    s = {"sub", registerValue(m), registerValue(n)};
    break;
  case 0x3a:  //SUBC Rm,Rn
    s = {"subc", registerValue(m), registerValue(n)};
    break;
  case 0x3b:  //SUBV Rm,Rn
    s = {"subv", registerValue(m), registerValue(n)};
    break;
  case 0x3c:  //ADD Rm,Rn
    s = {"add", registerValue(m), registerValue(n)};
    break;
  case 0x3d:  //DMULS.L Rm,Rn
    s = {"dmuls.l", registerValue(m), registerValue(n)};
    break;
  case 0x3e:  //ADDC Rm,Rn
    s = {"addc", registerValue(m), registerValue(n)};
    break;
  case 0x3f:  //ADDV Rm,Rn
    s = {"addv", registerValue(m), registerValue(n)};
    break;
  case 0x4f:  //MAC.W @Rm+,@Rn+
    s = {"mac.w", postincrementIndirectRegister(m), postincrementIndirectRegister(n)};
    break;
  case 0x50 ... 0x5f:  //MOV.L @(disp,Rm),Rn
    s = {"mov.l", indirectRegisterDisplacement(m, d4, 4), registerName(n)};
    break;
  case 0x60:  //MOV.B @Rm,Rn
    s = {"mov.b", indirectRegister(m), registerName(n)};
    break;
  case 0x61:  //MOV.W @Rm,Rn
    s = {"mov.w", indirectRegister(m), registerName(n)};
    break;
  case 0x62:  //MOV.L @Rm,Rn
    s = {"mov.l", indirectRegister(m), registerName(n)};
    break;
  case 0x63:  //MOV Rm,Rn
    s = {"mov", registerValue(m), registerName(n)};
    break;
  case 0x64:  //MOV.B @Rm+,Rn
    s = {"mov.b", postincrementIndirectRegister(m), registerName(n)};
    break;
  case 0x65:  //MOV.W @Rm+,Rn
    s = {"mov.w", postincrementIndirectRegister(m), registerName(n)};
    break;
  case 0x66:  //MOV.L @Rm+,Rn
    s = {"mov.l", postincrementIndirectRegister(m), registerName(n)};
    break;
  case 0x67:  //NOT Rm,Rn
    s = {"not", registerValue(m), registerName(n)};
    break;
  case 0x68:  //SWAP.B Rm,Rn
    s = {"swap.b", registerValue(m), registerValue(n)};
    break;
  case 0x69:  //SWAP.W Rm,Rn
    s = {"swap.w", registerValue(m), registerValue(n)};
    break;
  case 0x6a:  //NEGC Rm,Rn
    s = {"negc", registerValue(m), registerName(n)};
    break;
  case 0x6b:  //NEG Rm,Rn
    s = {"neg", registerValue(m), registerName(n)};
    break;
  case 0x6c:  //EXTU.B Rm,Rn
    s = {"extu.b", registerValue(m), registerName(n)};
    break;
  case 0x6d:  //EXTU.W Rm,Rn
    s = {"extu.w", registerValue(m), registerName(n)};
    break;
  case 0x6e:  //EXTS.B Rm,Rn
    s = {"exts.b", registerValue(m), registerName(n)};
    break;
  case 0x6f:  //EXTS.W Rm,Rn
    s = {"exts.w", registerValue(m), registerName(n)};
    break;
  case 0x70 ... 0x7f:  //ADD #imm,Rn
    s = {"add", immediate(i), registerValue(n)};
    break;
  case 0x90 ... 0x9f:  //MOV.W @(disp,PC),Rn
    s = {"mov.w", pcRelativeDisplacement(d8, 2), registerName(n)};
    break;
  case 0xa0 ... 0xaf:  //BRA disp
    s = {"bra", branch12(d12)};
    break;
  case 0xb0 ... 0xbf:  //BSR disp
    s = {"bsr", branch12(d12)};
    break;
  case 0xd0 ... 0xdf:  //MOV.L @(disp,PC),Rn
    s = {"mov.l", pcRelativeDisplacement(d8, 4), registerName(n)};
    break;
  case 0xe0 ... 0xef:  //MOV #imm,Rn
    s = {"mov", immediate(i), registerName(n)};
    break;
  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8
  #undef d12

  #define n  (opcode >> 8 & 0x0f)
  #define m  (opcode >> 4 & 0x0f)  //n for MOVBS4, MOVWS4
  #define i  (opcode >> 0 & 0xff)
  #define d4 (opcode >> 0 & 0x0f)
  #define d8 (opcode >> 0 & 0xff)
  switch(opcode >> 8) {
  case 0x80:  //MOV.B R0,@(disp,Rn)
    s = {"mov.b", registerValue(0), indirectRegisterDisplacement(m, d4, 1)};
    break;
  case 0x81:  //MOV.W R0,@(disp,Rn)
    s = {"mov.w", registerValue(0), indirectRegisterDisplacement(m, d4, 2)};
    break;
  case 0x84:  //MOV.B @(disp,Rm),R0
    s = {"mov.b", indirectRegisterDisplacement(m, d4, 1), registerName(0)};
    break;
  case 0x85:  //MOV.W @(disp,Rm),R0
    s = {"mov.w", indirectRegisterDisplacement(m, d4, 2), registerName(0)};
    break;
  case 0x88:  //CMP/EQ #imm,R0
    s = {"cmp/eq", immediate(i), registerValue(0)};
    break;
  case 0x89:  //BT disp
    s = {"bt", branch8(d8)};
    break;
  case 0x8b:  //BF disp
    s = {"bf", branch8(d8)};
    break;
  case 0x8d:  //BT/S disp
    s = {"bt/s", branch8(d8)};
    break;
  case 0x8f:  //BF/S disp
    s = {"bf/s", branch8(d8)};
    break;
  case 0xc0:  //MOV.B R0,@(disp,GBR)
    s = {"mov.b", registerValue(0), gbrIndirectDisplacement(d8, 1)};
    break;
  case 0xc1:  //MOV.W R0,@(disp,GBR)
    s = {"mov.w", registerValue(0), gbrIndirectDisplacement(d8, 2)};
    break;
  case 0xc2:  //MOV.L R0,@(disp,GBR)
    s = {"mov.l", registerValue(0), gbrIndirectDisplacement(d8, 4)};
    break;
  case 0xc3:  //TRAPA #imm
    s = {"trapa", immediate(i)};
    break;
  case 0xc4:  //MOV.B @(disp,GBR),R0
    s = {"mov.b", gbrIndirectDisplacement(d8, 1), registerName(0)};
    break;
  case 0xc5:  //MOV.W @(disp,GBR),R0
    s = {"mov.w", gbrIndirectDisplacement(d8, 2), registerName(0)};
    break;
  case 0xc6:  //MOV.L @(disp,GBR),R0
    s = {"mov.l", gbrIndirectDisplacement(d8, 4), registerName(0)};
    break;
  case 0xc7:  //MOVA @(disp,PC),R0
    s = {"mova", pcRelativeDisplacement(d8, 4), registerName(0)};
    break;
  case 0xc8:  //TST #imm,R0
    s = {"tst", immediate(i), registerValue(0)};
    break;
  case 0xc9:  //AND #imm,R0
    s = {"and", immediate(i), registerValue(0)};
    break;
  case 0xca:  //XOR #imm,R0
    s = {"xor", immediate(i), registerValue(0)};
    break;
  case 0xcb:  //OR #imm,R0
    s = {"or", immediate(i), registerValue(0)};
    break;
  case 0xcc:  //TST.B #imm,@(R0,GBR)
    s = {"tst.b", immediate(i), gbrIndirectIndexed()};
    break;
  case 0xcd:  //AND.B #imm,@(R0,GBR)
    s = {"and.b", immediate(i), gbrIndirectIndexed()};
    break;
  case 0xce:  //XOR.B #imm,@(R0,GBR)
    s = {"xor.b", immediate(i), gbrIndirectIndexed()};
    break;
  case 0xcf:  //OR.B #imm,@(R0,GBR)
    s = {"or.b", immediate(i), gbrIndirectIndexed()};
    break;
  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8

  #define n (opcode >> 8 & 0xf)
  #define m (opcode >> 8 & 0xf)
  switch(opcode >> 4 & 0x0f00 | opcode & 0x00ff) {
  case 0x002:  //STC SR,Rn
    s = {"stc", "sr", registerName(n)};
    break;
  case 0x003:  //BSRF Rm
    s = {"bsrf", registerValue(m)};
    break;
  case 0x00a:  //STS MACH,Rn
    s = {"sts", "mach", registerName(n)};
    break;
  case 0x012:  //STC GBR,Rn
    s = {"stc", "gbr", registerName(n)};
    break;
  case 0x01a:  //STS MACL,Rn
    s = {"sts", "macl", registerName(n)};
    break;
  case 0x022:  //STC VBR,Rn
    s = {"stc", "vbr", registerName(n)};
    break;
  case 0x023:  //BRAF Rm
    s = {"braf", registerValue(m)};
    break;
  case 0x029:  //MOVT Rn
    s = {"movt", registerName(n)};
    break;
  case 0x02a:  //STS PR,Rn
    s = {"sts", "pr", registerName(n)};
    break;
  case 0x400:  //SHLL Rn
    s = {"shll", registerValue(n)};
    break;
  case 0x401:  //SHLR Rn
    s = {"shlr", registerValue(n)};
    break;
  case 0x402:  //STS.L MACH,@-Rn
    s = {"sts.l", "mach", predecrementIndirectRegister(n, 4)};
    break;
  case 0x403:  //STC.L SR,@-Rn
    s = {"stc.l", "sr", predecrementIndirectRegister(n, 4)};
    break;
  case 0x404:  //ROTL Rn
    s = {"rotl", registerValue(n)};
    break;
  case 0x405:  //ROTR Rn
    s = {"rotr", registerValue(n)};
    break;
  case 0x406:  //LDS.L @Rm+,MACH
    s = {"lds.l", postincrementIndirectRegister(m), "mach"};
    break;
  case 0x407:  //LDC.L @Rm+,SR
    s = {"ldc.l", postincrementIndirectRegister(m), "sr"};
    break;
  case 0x408:  //SHLL2 Rn
    s = {"shll2", registerValue(n)};
    break;
  case 0x409:  //SHLR2 Rn
    s = {"shlr2", registerValue(n)};
    break;
  case 0x40a:  //LDS Rm,MACH
    s = {"lds", registerValue(m), "mach"};
    break;
  case 0x40b:  //JSR @Rm
    s = {"jsr", indirectRegister(m)};
    break;
  case 0x40e:  //LDC Rm,SR
    s = {"ldc", registerValue(m), "sr"};
    break;
  case 0x410:  //DT Rn
    s = {"dt", registerValue(n)};
    break;
  case 0x411:  //CMP/PZ Rn
    s = {"cmp/pz", registerValue(n)};
    break;
  case 0x412:  //STS.L MACL,@-Rn
    s = {"sts.l", "macl", predecrementIndirectRegister(n, 4)};
    break;
  case 0x413:  //STC.L GBR,@-Rn
    s = {"stc.l", "gbr", predecrementIndirectRegister(n, 4)};
    break;
  case 0x415:  //CMP/PL Rn
    s = {"cmp/pl", registerValue(n)};
    break;
  case 0x416:  //LDS.L @Rm+,MACL
    s = {"lds.l", postincrementIndirectRegister(m), "macl"};
    break;
  case 0x417:  //LDC.L @Rm+,GBR
    s = {"ldc.l", postincrementIndirectRegister(m), "gbr"};
    break;
  case 0x418:  //SHLL8 Rn
    s = {"shll8", registerValue(n)};
    break;
  case 0x419:  //SHLR8 Rn
    s = {"shlr8", registerValue(n)};
    break;
  case 0x41a:  //LDS Rm,MACL
    s = {"lds", registerValue(m), "macl"};
    break;
  case 0x41b:  //TAS.B @Rn
    s = {"tas.b", indirectRegister(n)};
    break;
  case 0x41e:  //LDC Rm,GBR
    s = {"ldc", registerValue(m), "gbr"};
    break;
  case 0x420:  //SHAL Rn
    s = {"shal", registerValue(n)};
    break;
  case 0x421:  //SHAR Rn
    s = {"shar", registerValue(n)};
    break;
  case 0x422:  //STS.L PR,@-Rn
    s = {"sts.l", "pr", predecrementIndirectRegister(n, 4)};
    break;
  case 0x423:  //STC.L VBR,@-Rn
    s = {"stc.l", "vbr", predecrementIndirectRegister(n, 4)};
    break;
  case 0x424:  //ROTCL Rn
    s = {"rotcl", registerValue(n)};
    break;
  case 0x425:  //ROTCR Rn
    s = {"rotcr", registerValue(n)};
    break;
  case 0x426:  //LDS.L @Rm+,PR
    s = {"lds.l", postincrementIndirectRegister(m), "pr"};
    break;
  case 0x427:  //LDC.L @Rm+,VBR
    s = {"ldc.l", postincrementIndirectRegister(m), "vbr"};
    break;
  case 0x428:  //SHLL16 Rn
    s = {"shll16", registerValue(n)};
    break;
  case 0x429:  //SHLR16 Rn
    s = {"shlr16", registerValue(n)};
    break;
  case 0x42a:  //LDS Rm,PR
    s = {"lds", registerValue(m), "pr"};
    break;
  case 0x42b:  //JMP @Rm
    s = {"jmp", indirectRegister(m)};
    break;
  case 0x42e:  //LDC Rm,VBR
    s = {"ldc", registerValue(m), "vbr"};
    break;
  }
  #undef n
  #undef m

  switch(opcode) {
  case 0x0008:  //CLRT
    s = {"clrt"};
    break;
  case 0x0009:  //NOP
    s = {"nop"};
    break;
  case 0x000b:  //RTS
    s = {"rts"};
    break;
  case 0x0018:  //SETT
    s = {"sett"};
    break;
  case 0x0019:  //DIV0U
    s = {"div0u"};
    break;
  case 0x001b:  //SLEEP
    s = {"sleep"};
    break;
  case 0x0028:  //CLRMAC
    s = {"clrmac"};
    break;
  case 0x002b:  //RTE
    s = {"rte"};
    break;
  }

  if(!s) s = {"illegal", {"0x", hex(opcode, 4L)}};
  auto name = s.takeFirst();
  while(name.size() < 8) name.append(" ");
  string r = {name, s.merge(",")};
  while(r.size() < 30) r.append(" ");
  return r;
}

auto SH2::disassembleContext() -> string {
  string s;
  s.append("r0:",   hex(R[0], 8L), " ");
  s.append("r1:",   hex(R[1], 8L), " ");
  s.append("r2:",   hex(R[2], 8L), " ");
  s.append("r3:",   hex(R[3], 8L), " ");
  s.append("r4:",   hex(R[4], 8L), " ");
  s.append("r5:",   hex(R[5], 8L), " ");
  s.append("r6:",   hex(R[6], 8L), " ");
  s.append("r7:",   hex(R[7], 8L), " ");
  s.append("r8:",   hex(R[8], 8L), " ");
  s.append("r9:",   hex(R[9], 8L), " ");
  s.append("r10:",  hex(R[10], 8L), " ");
  s.append("r11:",  hex(R[11], 8L), " ");
  s.append("r12:",  hex(R[12], 8L), " ");
  s.append("r13:",  hex(R[13], 8L), " ");
  s.append("r14:",  hex(R[14], 8L), " ");
  s.append("r15:",  hex(R[15], 8L), " ");
  s.append("sr:",   hex((u32)SR, 8L), " ");
  s.append("gbr:",  hex(GBR, 8L), " ");
  s.append("vbr:",  hex(VBR, 8L), " ");
  s.append("mach:", hex(MACH, 8L), " ");
  s.append("macl:", hex(MACL, 8L), " ");
  s.append("pr:",   hex(PR, 8L), " ");
  s.append(SR.M ? "M" : "m");
  s.append(SR.Q ? "Q" : "q");
  s.append(hex(SR.I, 1L));
  s.append(SR.S ? "S" : "s");
  s.append(SR.T ? "T" : "t");
  return s;
}
