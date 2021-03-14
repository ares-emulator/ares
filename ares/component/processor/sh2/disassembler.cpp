auto SH2::disassembleInstruction() -> string {
  vector<string> s;

  auto register = [&](u32 r) -> string {
    return {"r", r};
  };
  auto indirectRegister = [&](u32 r) -> string {
    return {"@r", r};
  };
  auto predecrementIndirectRegister = [&](u32 r) -> string {
    return {"@-r", r};
  };
  auto postincrementIndirectRegister = [&](u32 r) -> string {
    return {"@r", r, "+"};
  };
  auto indirectIndexedRegister = [&](u32 r) -> string {
    return {"@(R0,R", r, ")"};
  };
  auto immediate = [&](u32 i) -> string {
    return {"#0x", hex(i, 2L)};
  };
  auto pcRelativeDisplacement = [&](u32 d) -> string {
    return {"@(0x", hex(d, 2L), ",PC)"};
  };
  auto gbrIndirectDisplacement = [&](u32 d) -> string {
    return {"@(0x", hex(d, 2L), ",GBR)"};
  };
  auto gbrIndirectIndexed = [&]() -> string {
    return {"@(R0,GBR)"};
  };
  auto indirectRegisterDisplacement = [&](u32 d, u32 r) -> string {
    return {"@(0x", hex(d, 1L), ",R", r, ")"};
  };
  auto displacement8 = [&](u32 d) -> string {
    return {"0x", hex(d, 2L)};
  };
  auto displacement12 = [&](u32 d) -> string {
    return {"0x", hex(d, 3L)};
  };

  u16 opcode = readWord(PC);

  #define n   (opcode >> 8 & 0x00f)
  #define m   (opcode >> 4 & 0x00f)
  #define i   (opcode >> 0 & 0x0ff)
  #define d4  (opcode >> 0 & 0x00f)
  #define d8  (opcode >> 0 & 0x0ff)
  #define d12 (opcode >> 0 & 0xfff)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {
  case 0x04:  //MOV.B Rm,@(R0,Rn)
    s = {"mov.b", register(m), indirectIndexedRegister(n)};
    break;
  case 0x05:  //MOV.W Rm,@(R0,Rn)
    s = {"mov.w", register(m), indirectIndexedRegister(n)};
    break;
  case 0x06:  //MOV.L Rm,@(R0,Rn)
    s = {"mov.l", register(m), indirectIndexedRegister(n)};
    break;
  case 0x07:  //MUL.L Rm,Rn
    s = {"mul.l", register(m), register(n)};
    break;
  case 0x0c:  //MOV.B @(R0,Rm),Rn
    s = {"mov.b", indirectIndexedRegister(m), register(n)};
    break;
  case 0x0d:  //MOV.W @(R0,Rm),Rn
    s = {"mov.w", indirectIndexedRegister(m), register(n)};
    break;
  case 0x0e:  //MOV.L @(R0,Rm),Rn
    s = {"mov.l", indirectIndexedRegister(m), register(n)};
    break;
  case 0x0f:  //MAC.L @Rm+,@Rn+
    s = {"mac.l", postincrementIndirectRegister(m), postincrementIndirectRegister(n)};
    break;
  case 0x10 ... 0x1f:  //MOV.L Rm,@(disp,Rn)
    s = {"mov.l", register(m), indirectRegisterDisplacement(d4, n)};
    break;
  case 0x20:  //MOV.B Rm,@Rn
    s = {"mov.b", register(m), indirectRegister(n)};
    break;
  case 0x21:  //MOV.W Rm,@Rn
    s = {"mov.w", register(m), indirectRegister(n)};
    break;
  case 0x22:  //MOV.L Rm,@Rn
    s = {"mov.l", register(m), indirectRegister(n)};
    break;
  case 0x24:  //MOV.B Rm,@-Rn
    s = {"mov.b", register(m), predecrementIndirectRegister(n)};
    break;
  case 0x25:  //MOV.W Rm,@-Rn
    s = {"mov.w", register(m), predecrementIndirectRegister(n)};
    break;
  case 0x26:  //MOV.L Rm,@-Rn
    s = {"mov.l", register(m), predecrementIndirectRegister(n)};
    break;
  case 0x27:  //DIV0S Rm,Rn
    s = {"div0s", register(m), register(n)};
    break;
  case 0x28:  //TST Rm,Rn
    s = {"tst", register(m), register(n)};
    break;
  case 0x29:  //AND Rm,Rn
    s = {"and", register(m), register(n)};
    break;
  case 0x2a:  //XOR Rm,Rn
    s = {"xor", register(m), register(n)};
    break;
  case 0x2b:  //OR Rm,Rn
    s = {"or", register(m), register(n)};
    break;
  case 0x2c:  //CMP/STR Rm,Rn
    s = {"cmp/str", register(m), register(n)};
    break;
  case 0x2d:  //XTRCT Rm,Rn
    s = {"xtrct", register(m), register(n)};
    break;
  case 0x2e:  //MULU Rm,Rn
    s = {"mulu", register(m), register(n)};
    break;
  case 0x2f:  //MULS Rm,Rn
    s = {"muls", register(m), register(n)};
    break;
  case 0x30:  //CMP/EQ Rm,Rn
    s = {"cmp/eq", register(m), register(n)};
    break;
  case 0x32:  //CMP/HS Rm,Rn
    s = {"cmp/hs", register(m), register(n)};
    break;
  case 0x33:  //CMP/GE Rm,Rn
    s = {"cmp/ge", register(m), register(n)};
    break;
  case 0x34:  //DIV1 Rm,Rn
    s = {"div1", register(m), register(n)};
    break;
  case 0x35:  //DMULU.L Rm,Rn
    s = {"dmulu.l", register(m), register(n)};
    break;
  case 0x36:  //CMP/HI Rm,Rn
    s = {"cmp/hi", register(m), register(n)};
    break;
  case 0x37:  //CMP/GT Rm,Rn
    s = {"cmp/gt", register(m), register(n)};
    break;
  case 0x38:  //SUB Rm,Rn
    s = {"sub", register(m), register(n)};
    break;
  case 0x3a:  //SUBC Rm,Rn
    s = {"subc", register(m), register(n)};
    break;
  case 0x3b:  //SUBV Rm,Rn
    s = {"subv", register(m), register(n)};
    break;
  case 0x3c:  //ADD Rm,Rn
    s = {"add", register(m), register(n)};
    break;
  case 0x3d:  //DMULS.L Rm,Rn
    s = {"dmuls.l", register(m), register(n)};
    break;
  case 0x3e:  //ADDC Rm,Rn
    s = {"addc", register(m), register(n)};
    break;
  case 0x3f:  //ADDV Rm,Rn
    s = {"addv", register(m), register(n)};
    break;
  case 0x4f:  //MAC.W @Rm+,@Rn+
    s = {"mac.w", postincrementIndirectRegister(m), postincrementIndirectRegister(n)};
    break;
  case 0x50 ... 0x5f:  //MOV.L @(disp,Rm),Rn
    s = {"mov.l", indirectRegisterDisplacement(d4, m), register(n)};
    break;
  case 0x60:  //MOV.B @Rm,Rn
    s = {"mov.b", indirectRegister(m), register(n)};
    break;
  case 0x61:  //MOV.W @Rm,Rn
    s = {"mov.w", indirectRegister(m), register(n)};
    break;
  case 0x62:  //MOV.L @Rm,Rn
    s = {"mov.l", indirectRegister(m), register(n)};
    break;
  case 0x63:  //MOV Rm,Rn
    s = {"mov", register(m), register(n)};
    break;
  case 0x64:  //MOV.B @Rm+,Rn
    s = {"mov.b", postincrementIndirectRegister(m), register(n)};
    break;
  case 0x65:  //MOV.W @Rm+,Rn
    s = {"mov.w", postincrementIndirectRegister(m), register(n)};
    break;
  case 0x66:  //MOV.L @Rm+,Rn
    s = {"mov.l", postincrementIndirectRegister(m), register(n)};
    break;
  case 0x67:  //NOT Rm,Rn
    s = {"not", register(m), register(n)};
    break;
  case 0x68:  //SWAP.B Rm,Rn
    s = {"swap.b", register(m), register(n)};
    break;
  case 0x69:  //SWAP.W Rm,Rn
    s = {"swap.w", register(m), register(n)};
    break;
  case 0x6a:  //NEGC Rm,Rn
    s = {"negc", register(m), register(n)};
    break;
  case 0x6b:  //NEG Rm,Rn
    s = {"neg", register(m), register(n)};
    break;
  case 0x6c:  //EXTU.B Rm,Rn
    s = {"extu.b", register(m), register(n)};
    break;
  case 0x6d:  //EXTU.W Rm,Rn
    s = {"extu.w", register(m), register(n)};
    break;
  case 0x6e:  //EXTS.B Rm,Rn
    s = {"exts.b", register(m), register(n)};
    break;
  case 0x6f:  //EXTS.W Rm,Rn
    s = {"exts.w", register(m), register(n)};
    break;
  case 0x70 ... 0x7f:  //ADD #imm,Rn
    s = {"add", immediate(i), register(n)};
    break;
  case 0x90 ... 0x9f:  //MOV.W @(disp,PC),Rn
    s = {"mov.w", pcRelativeDisplacement(d8), register(n)};
    break;
  case 0xa0 ... 0xaf:  //BRA disp
    s = {"bra", displacement12(d12)};
    break;
  case 0xb0 ... 0xbf:  //BSR disp
    s = {"bsr", displacement12(d12)};
    break;
  case 0xd0 ... 0xdf:  //MOV.L @(disp,PC),Rn
    s = {"mov.l", pcRelativeDisplacement(d8), register(n)};
    break;
  case 0xe0 ... 0xef:  //MOV #imm,Rn
    s = {"mov", immediate(i), register(n)};
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
  case 0x00 ... 0x0f:  //MOVT Rn
    s = {"movt", register(n)};
    break;
  case 0x80:  //MOV.B R0,@(disp,Rn)
    s = {"mov.b", register(0), indirectRegisterDisplacement(d4, m)};
    break;
  case 0x81:  //MOV.W R0,@(disp,Rn)
    s = {"mov.w", register(0), indirectRegisterDisplacement(d4, m)};
    break;
  case 0x84:  //MOV.B @(disp,Rm),R0
    s = {"mov.b", indirectRegisterDisplacement(m, d4), register(0)};
    break;
  case 0x85:  //MOV.W @(disp,Rm),R0
    s = {"mov.w", indirectRegisterDisplacement(m, d4), register(0)};
    break;
  case 0x87:  //MOVA @(disp,PC),R0
    s = {"mova", pcRelativeDisplacement(d8), register(0)};
    break;
  case 0x88:  //CMP/EQ #imm,R0
    s = {"cmp/eq", immediate(i), register(0)};
    break;
  case 0x89:  //BT disp
    s = {"bt", displacement8(d8)};
    break;
  case 0x8b:  //BF disp
    s = {"bf", displacement8(d8)};
    break;
  case 0x8d:  //BT/S disp
    s = {"bt/s", displacement8(d8)};
    break;
  case 0x8f:  //BF/S disp
    s = {"bf/s", displacement8(d8)};
    break;
  case 0xc0:  //MOV.B R0,@(disp,GBR)
    s = {"mov.b", register(0), gbrIndirectDisplacement(d8)};
    break;
  case 0xc1:  //MOV.W R0,@(disp,GBR)
    s = {"mov.w", register(0), gbrIndirectDisplacement(d8)};
    break;
  case 0xc2:  //MOV.L R0,@(disp,GBR)
    s = {"mov.l", register(0), gbrIndirectDisplacement(d8)};
    break;
  case 0xc3:  //TRAPA #imm
    s = {"trapa", immediate(i)};
    break;
  case 0xc4:  //MOV.B @(disp,GBR),R0
    s = {"mov.b", gbrIndirectDisplacement(d8), register(0)};
    break;
  case 0xc5:  //MOV.W @(disp,GBR),R0
    s = {"mov.w", gbrIndirectDisplacement(d8), register(0)};
    break;
  case 0xc6:  //MOV.L @(disp,GBR),R0
    s = {"mov.l", gbrIndirectDisplacement(d8), register(0)};
    break;
  case 0xc8:  //TST #imm,R0
    s = {"tst", immediate(i), register(0)};
    break;
  case 0xc9:  //AND #imm,R0
    s = {"and", immediate(i), register(0)};
    break;
  case 0xca:  //XOR #imm,R0
    s = {"xor", immediate(i), register(0)};
    break;
  case 0xcb:  //OR #imm,R0
    s = {"or", immediate(i), register(0)};
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
    s = {"stc", "SR", register(n)};
    break;
  case 0x003:  //BSRF Rm
    s = {"bsrf", register(m)};
    break;
  case 0x00a:  //STS MACH,Rn
    s = {"sts", "MACH", register(n)};
    break;
  case 0x012:  //STC GBR,Rn
    s = {"stc", "GBR", register(n)};
    break;
  case 0x01a:  //STS MACL,Rn
    s = {"sts", "MACL", register(n)};
    break;
  case 0x022:  //STC VBR,Rn
    s = {"stc", "VBR", register(n)};
    break;
  case 0x023:  //BRAF Rm
    s = {"braf", register(m)};
    break;
  case 0x02a:  //STS PR,Rn
    s = {"sts", "PR", register(n)};
    break;
  case 0x400:  //SHLL Rn
    s = {"shll", register(n)};
    break;
  case 0x401:  //SHLR Rn
    s = {"shlr", register(n)};
    break;
  case 0x402:  //STS.L MACH,@-Rn
    s = {"sts.l", "MACH", predecrementIndirectRegister(n)};
    break;
  case 0x403:  //STC.L SR,@-Rn
    s = {"stc.l", "SR", predecrementIndirectRegister(n)};
    break;
  case 0x404:  //ROTL Rn
    s = {"rotl", register(n)};
    break;
  case 0x405:  //ROTR Rn
    s = {"rotr", register(n)};
    break;
  case 0x406:  //LDS.L @Rm+,MACH
    s = {"lds.l", postincrementIndirectRegister(m), "MACH"};
    break;
  case 0x407:  //LDC.L @Rm+,SR
    s = {"ldc.l", postincrementIndirectRegister(m), "SR"};
    break;
  case 0x408:  //SHLL2 Rn
    s = {"shll2", register(n)};
    break;
  case 0x409:  //SHLR2 Rn
    s = {"shlr2", register(n)};
    break;
  case 0x40a:  //LDS Rm,MACH
    s = {"lds", register(m), "MACH"};
    break;
  case 0x40b:  //JSR @Rm
    s = {"jsr", indirectRegister(m)};
    break;
  case 0x40e:  //LDC Rm,SR
    s = {"ldc", register(m), "SR"};
    break;
  case 0x410:  //DT Rn
    s = {"dt", register(n)};
    break;
  case 0x411:  //CMP/PZ Rn
    s = {"cmp/pz", register(n)};
    break;
  case 0x412:  //STS.L MACL,@-Rn
    s = {"sts.l", "MACL", predecrementIndirectRegister(n)};
    break;
  case 0x413:  //STC.L GBR,@-Rn
    s = {"stc.l", "GBR", predecrementIndirectRegister(n)};
    break;
  case 0x415:  //CMP/PL Rn
    s = {"cmp/pl", register(n)};
    break;
  case 0x416:  //LDS.L @Rm+,MACL
    s = {"lds.l", postincrementIndirectRegister(m), "MACL"};
    break;
  case 0x417:  //LDC.L @Rm+,GBR
    s = {"ldc.l", postincrementIndirectRegister(m), "GBR"};
    break;
  case 0x418:  //SHLL8 Rn
    s = {"shll8", register(n)};
    break;
  case 0x419:  //SHLR8 Rn
    s = {"shlr8", register(n)};
    break;
  case 0x41a:  //LDS Rm,MACL
    s = {"lds", register(m), "MACL"};
    break;
  case 0x41b:  //TAS.B @Rn
    s = {"tas.b", indirectRegister(n)};
    break;
  case 0x41e:  //LDC Rm,GBR
    s = {"ldc", register(m), "GBR"};
    break;
  case 0x420:  //SHAL Rn
    s = {"shal", register(n)};
    break;
  case 0x421:  //SHAR Rn
    s = {"shar", register(n)};
    break;
  case 0x422:  //STS.L PR,@-Rn
    s = {"sts.l", "PR", predecrementIndirectRegister(n)};
    break;
  case 0x423:  //STC.L VBR,@-Rn
    s = {"stc.l", "VBR", predecrementIndirectRegister(n)};
    break;
  case 0x424:  //ROTCL Rn
    s = {"rotcl", register(n)};
    break;
  case 0x425:  //ROTCR Rn
    s = {"rotcr", register(n)};
    break;
  case 0x426:  //LDS.L @Rm+,PR
    s = {"lds.l", postincrementIndirectRegister(m), "PR"};
    break;
  case 0x427:  //LDC.L @Rm+,VBR
    s = {"ldc.l", postincrementIndirectRegister(m), "VBR"};
    break;
  case 0x428:  //SHLL16 Rn
    s = {"shll16", register(n)};
    break;
  case 0x429:  //SHLR16 Rn
    s = {"shlr16", register(n)};
    break;
  case 0x42a:  //LDS Rm,PR
    s = {"lds", register(m), "PR"};
    break;
  case 0x42b:  //JMP @Rm
    s = {"jmp", indirectRegister(m)};
    break;
  case 0x42e:  //LDC Rm,VBR
    s = {"ldc", register(m), "VBR"};
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

  if(!s) s.append(hex(opcode, 4L));
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
  s.append("sr:",   hex(SR, 8L), " ");
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
