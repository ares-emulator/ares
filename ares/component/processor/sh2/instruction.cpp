auto SH2::branch(u32 pc) -> void {
  PPC = pc;
  PPM = Branch::Take;
}

auto SH2::delaySlot(u32 pc) -> void {
  PPC = pc;
  PPM = Branch::Slot;
}

auto SH2::interrupt(u32 level, u32 vector) -> void {
  writeLong(SP - 4, SR);
  writeLong(SP - 8, PC);
  SP -= 8;
  PC  = 4 + readLong(VBR + vector * 4);
  PPM = Branch::Step;
  SR.I = level;
}

auto SH2::instruction() -> void {
  u16 opcode = readWord(PC - 4);
  execute(opcode);

  switch(PPM) {
  case Branch::Step: PC = PC + 2; break;
  case Branch::Slot: PC = PC + 2; PPM = Branch::Take; break;
  case Branch::Take: PC = PPC;    PPM = Branch::Step; break;
  }
}

auto SH2::execute(u16 opcode) -> void {
  #define n   (opcode >> 8 & 0x00f)
  #define m   (opcode >> 4 & 0x00f)
  #define i   (opcode >> 0 & 0x0ff)
  #define d4  (opcode >> 0 & 0x00f)
  #define d8  (opcode >> 0 & 0x0ff)
  #define d12 (opcode >> 0 & 0xfff)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {
  case 0x04: return MOVBS0(m, n);               //MOV.B   Rm,@(R0,Rn)
  case 0x05: return MOVWS0(m, n);               //MOV.W   Rm,@(R0,Rn)
  case 0x06: return MOVLS0(m, n);               //MOV.L   Rm,@(R0,Rn)
  case 0x07: return MULL(m, n);                 //MUL.L   Rm,Rn
  case 0x0c: return MOVBL0(m, n);               //MOV.B   @(R0,Rm),Rn
  case 0x0d: return MOVWL0(m, n);               //MOV.W   @(R0,Rm),Rn
  case 0x0e: return MOVLL0(m, n);               //MOV.L   @(R0,Rm),Rn
  case 0x0f: return MACL_(m, n);                //MAC.L   @Rm+,@Rn+
  case 0x10 ... 0x1f: return MOVLS4(m, d4, n);  //MOV.L   Rm,@(disp,Rn)
  case 0x20: return MOVBS(m, n);                //MOV.B   Rm,@Rn
  case 0x21: return MOVWS(m, n);                //MOV.W   Rm,@Rn
  case 0x22: return MOVLS(m, n);                //MOV.L   Rm,@Rn
  case 0x24: return MOVBM(m, n);                //MOV.B   Rm,@-Rn
  case 0x25: return MOVWM(m, n);                //MOV.W   Rm,@-Rn
  case 0x26: return MOVLM(m, n);                //MOV.L   Rm,@-Rn
  case 0x27: return DIV0S(m, n);                //DIV0S   Rm,Rn
  case 0x28: return TST(m, n);                  //TST     Rm,Rn
  case 0x29: return AND(m, n);                  //AND     Rm,Rn
  case 0x2a: return XOR(m, n);                  //XOR     Rm,Rn
  case 0x2b: return OR(m, n);                   //OR      Rm,Rn
  case 0x2c: return CMPSTR(m, n);               //CMP/STR Rm,Rn
  case 0x2d: return XTRCT(m, n);                //XTRCT   Rm,Rn
  case 0x2e: return MULU(m, n);                 //MULU    Rm,Rn
  case 0x2f: return MULS(m, n);                 //MULS    Rm,Rn
  case 0x30: return CMPEQ(m, n);                //CMP/EQ  Rm,Rn
  case 0x32: return CMPHS(m, n);                //CMP/HS  Rm,Rn
  case 0x33: return CMPGE(m, n);                //CMP/GE  Rm,Rn
  case 0x34: return DIV1(m, n);                 //DIV1    Rm,Rn
  case 0x35: return DMULU(m, n);                //DMULU.L RM,Rn
  case 0x36: return CMPHI(m, n);                //CMP/HI  Rm,Rn
  case 0x37: return CMPGT(m, n);                //CMP/GT  Rm,Rn
  case 0x38: return SUB(m, n);                  //SUB     Rm,Rn
  case 0x3a: return SUBC(m, n);                 //SUBC    Rm,Rn
  case 0x3b: return SUBV(m, n);                 //SUBV    Rm,Rn
  case 0x3c: return ADD(m, n);                  //ADD     Rm,Rn
  case 0x3d: return DMULS(m, n);                //DMULS.L Rm,Rn
  case 0x3e: return ADDC(m, n);                 //ADDC    Rm,Rn
  case 0x3f: return ADDV(m, n);                 //ADDV    Rm,Rn
  case 0x4f: return MACW(m, n);                 //MAC.W   @Rm+,@Rn+
  case 0x50 ... 0x5f: return MOVLL4(m, d4, n);  //MOV.L   @(disp,Rm),Rn
  case 0x60: return MOVBL(m, n);                //MOV.B   @Rm,Rn
  case 0x61: return MOVWL(m, n);                //MOV.W   @Rm,Rn
  case 0x62: return MOVLL(m, n);                //MOV.L   @Rm,Rn
  case 0x63: return MOV(m, n);                  //MOV     Rm,Rn
  case 0x64: return MOVBP(m, n);                //MOV.B   @Rm+,Rn
  case 0x65: return MOVWP(m, n);                //MOV.W   @Rm+,Rn
  case 0x66: return MOVLP(m, n);                //MOV.L   @Rm+,Rn
  case 0x67: return NOT(m, n);                  //NOT     Rm,Rn
  case 0x68: return SWAPB(m, n);                //SWAP.B  Rm,Rn
  case 0x69: return SWAPW(m, n);                //SWAP.W  Rm,Rn
  case 0x6a: return NEGC(m, n);                 //NEGC    Rm,Rn
  case 0x6b: return NEG(m, n);                  //NEG     Rm,Rn
  case 0x6c: return EXTUB(m, n);                //EXTU.B  Rm,Rn
  case 0x6d: return EXTUW(m, n);                //EXTU.W  Rm,Rn
  case 0x6e: return EXTSB(m, n);                //EXTS.B  Rm,Rn
  case 0x6f: return EXTSW(m, n);                //EXTS.W  Rm,Rn
  case 0x70 ... 0x7f: return ADDI(i, n);        //ADD     #imm,Rn
  case 0x90 ... 0x9f: return MOVWI(d8, n);      //MOV.W   @(disp,PC),Rn
  case 0xa0 ... 0xaf: return BRA(d12);          //BRA     disp
  case 0xb0 ... 0xbf: return BSR(d12);          //BSR     disp
  case 0xd0 ... 0xdf: return MOVLI(d8, n);      //MOV.L   @(disp.PC),Rn
  case 0xe0 ... 0xef: return MOVI(i, n);        //MOV     #imm,Rn
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
  case 0x80: return MOVBS4(d4, m);     //MOV.B  R0,@(disp,Rn)
  case 0x81: return MOVWS4(d4, m);     //MOV.W  R0,@(disp,Rn)
  case 0x84: return MOVBL4(m, d4);     //MOV.B  @(disp,Rm),R0
  case 0x85: return MOVWL4(m, d4);     //MOV.W  @(disp,Rm),R0
  case 0x88: return CMPIM(i);          //CMP/EQ #imm,R0
  case 0x89: return BT(d8);            //BT     disp
  case 0x8b: return BF(d8);            //BF     disp
  case 0x8d: return BTS(d8);           //BT/S   disp
  case 0x8f: return BFS(d8);           //BF/S   disp
  case 0xc0: return MOVBSG(d8);        //MOV.B  R0,@(disp,GBR)
  case 0xc1: return MOVWSG(d8);        //MOV.W  R0,@(disp,GBR)
  case 0xc2: return MOVLSG(d8);        //MOV.L  R0,@(disp,GBR)
  case 0xc3: return TRAPA(i);          //TRAPA  #imm
  case 0xc4: return MOVBLG(d8);        //MOV.B  @(disp,GBR),R0
  case 0xc5: return MOVWLG(d8);        //MOV.W  @(disp,GBR),R0
  case 0xc6: return MOVLLG(d8);        //MOV.L  @(disp,GBR),R0
  case 0xc7: return MOVA(d8);          //MOVA   @(disp,PC),R0
  case 0xc8: return TSTI(i);           //TST    #imm,R0
  case 0xc9: return ANDI(i);           //AND    #imm,R0
  case 0xca: return XORI(i);           //XOR    #imm,R0
  case 0xcb: return ORI(i);            //OR     #imm,R0
  case 0xcc: return TSTM(i);           //TST.B  #imm,@(R0,GBR)
  case 0xcd: return ANDM(i);           //AND.B  #imm,@(R0,GBR)
  case 0xce: return XORM(i);           //XOR.B  #imm,@(R0,GBR)
  case 0xcf: return ORM(i);            //OR.B   #imm,@(R0,GBR)
  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8

  #define n (opcode >> 8 & 0xf)
  #define m (opcode >> 8 & 0xf)
  switch(opcode >> 4 & 0x0f00 | opcode & 0x00ff) {
  case 0x002: return STCSR(n);     //STC    SR,Rn
  case 0x003: return BSRF(m);      //BSRF   Rm
  case 0x00a: return STSMACH(n);   //STS    MACH,Rn
  case 0x012: return STCGBR(n);    //STC    GBR,Rn
  case 0x01a: return STSMACL(n);   //STS    MACL,Rn
  case 0x022: return STCVBR(n);    //STC    VBR,Rn
  case 0x023: return BRAF(m);      //BRAF   Rm
  case 0x029: return MOVT(n);      //MOVT   Rn
  case 0x02a: return STSPR(m);     //STS    PR,Rn
  case 0x400: return SHLL(n);      //SHLL   Rn
  case 0x401: return SHLR(n);      //SHLR   Rn
  case 0x402: return STSMMACH(n);  //STS.L  MACH,@-Rn
  case 0x403: return STCMSR(n);    //STC.L  SR,@-Rn
  case 0x404: return ROTL(n);      //ROTL   Rn
  case 0x405: return ROTR(n);      //ROTR   Rn
  case 0x406: return LDSMMACH(m);  //LDS.L  @Rm+,MACH
  case 0x407: return LDCMSR(m);    //LDC.L  @Rm+,SR
  case 0x408: return SHLL2(n);     //SHLL2  Rn
  case 0x409: return SHLR2(n);     //SHLR2  Rn
  case 0x40a: return LDSMACH(m);   //LDS    Rm,MACH
  case 0x40b: return JSR(m);       //JSR    @Rm
  case 0x40e: return LDCSR(m);     //LDC    Rm,SR
  case 0x410: return DT(n);        //DT     Rn
  case 0x411: return CMPPZ(n);     //CMP/PZ Rn
  case 0x412: return STSMMACL(n);  //STS.L  MACL,@-Rn
  case 0x413: return STCMGBR(n);   //STC.L  GBR,@-Rn
  case 0x415: return CMPPL(n);     //CMP/PL Rn
  case 0x416: return LDSMMACH(m);  //LDS.L  @Rm+,MACL
  case 0x417: return LDCMGBR(m);   //LDC.L  @Rm+,GBR
  case 0x418: return SHLL8(n);     //SHLL8  Rn
  case 0x419: return SHLR8(n);     //SHLR8  Rn
  case 0x41a: return LDSMACL(m);   //LDS    Rm,MACL
  case 0x41b: return TAS(n);       //TAS    @Rn
  case 0x41e: return LDCGBR(m);    //LDC    Rm,GBR
  case 0x420: return SHAL(n);      //SHAL   Rn
  case 0x421: return SHAR(n);      //SHAR   Rn
  case 0x422: return STSMPR(n);    //STS.L  PR,@-Rn
  case 0x423: return STCMVBR(n);   //STC.L  VBR,@-Rn
  case 0x424: return ROTCL(n);     //ROTCL  Rn
  case 0x425: return ROTCR(n);     //ROTCR  Rn
  case 0x426: return LDSMPR(m);    //LDS.L  @Rm+,PR
  case 0x427: return LDCMVBR(m);   //LDC.L  @Rm+,VBR
  case 0x428: return SHLL16(n);    //SHLL16 Rn
  case 0x429: return SHLR16(n);    //SHLR16 Rn
  case 0x42a: return LDSPR(m);     //LDS    Rm,PR
  case 0x42b: return JMP(m);       //JMP    @Rm
  case 0x42e: return LDCVBR(m);    //LDC    Rm,VBR
  }
  #undef n
  #undef m

  switch(opcode) {
  case 0x0008: return CLRT();    //CLRT
  case 0x0009: return NOP();     //NOP
  case 0x000b: return RTS();     //RTS
  case 0x0018: return SETT();    //SETT
  case 0x0019: return DIV0U();   //DIV0U
  case 0x001b: return SLEEP();   //SLEEP
  case 0x0028: return CLRMAC();  //CLRMAC
  case 0x002b: return RTE();     //RTE
  }

//interrupt(15, 4);
}
