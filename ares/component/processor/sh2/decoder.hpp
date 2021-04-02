{
  #define n   (opcode >> 8 & 0x00f)
  #define m   (opcode >> 4 & 0x00f)
  #define i   (opcode >> 0 & 0x0ff)
  #define d4  (opcode >> 0 & 0x00f)
  #define d8  (opcode >> 0 & 0x0ff)
  #define d12 (opcode >> 0 & 0xfff)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {
  op(0x04, MOVBS0, m, n);               //MOV.B   Rm,@(R0,Rn)
  op(0x05, MOVWS0, m, n);               //MOV.W   Rm,@(R0,Rn)
  op(0x06, MOVLS0, m, n);               //MOV.L   Rm,@(R0,Rn)
  op(0x07, MULL, m, n);                 //MUL.L   Rm,Rn
  op(0x0c, MOVBL0, m, n);               //MOV.B   @(R0,Rm),Rn
  op(0x0d, MOVWL0, m, n);               //MOV.W   @(R0,Rm),Rn
  op(0x0e, MOVLL0, m, n);               //MOV.L   @(R0,Rm),Rn
  op(0x0f, MACL_, m, n);                //MAC.L   @Rm+,@Rn+
  op(0x10 ... 0x1f, MOVLS4, m, d4, n);  //MOV.L   Rm,@(disp,Rn)
  op(0x20, MOVBS, m, n);                //MOV.B   Rm,@Rn
  op(0x21, MOVWS, m, n);                //MOV.W   Rm,@Rn
  op(0x22, MOVLS, m, n);                //MOV.L   Rm,@Rn
  op(0x24, MOVBM, m, n);                //MOV.B   Rm,@-Rn
  op(0x25, MOVWM, m, n);                //MOV.W   Rm,@-Rn
  op(0x26, MOVLM, m, n);                //MOV.L   Rm,@-Rn
  op(0x27, DIV0S, m, n);                //DIV0S   Rm,Rn
  op(0x28, TST, m, n);                  //TST     Rm,Rn
  op(0x29, AND, m, n);                  //AND     Rm,Rn
  op(0x2a, XOR, m, n);                  //XOR     Rm,Rn
  op(0x2b, OR, m, n);                   //OR      Rm,Rn
  op(0x2c, CMPSTR, m, n);               //CMP/STR Rm,Rn
  op(0x2d, XTRCT, m, n);                //XTRCT   Rm,Rn
  op(0x2e, MULU, m, n);                 //MULU    Rm,Rn
  op(0x2f, MULS, m, n);                 //MULS    Rm,Rn
  op(0x30, CMPEQ, m, n);                //CMP/EQ  Rm,Rn
  op(0x32, CMPHS, m, n);                //CMP/HS  Rm,Rn
  op(0x33, CMPGE, m, n);                //CMP/GE  Rm,Rn
  op(0x34, DIV1, m, n);                 //DIV1    Rm,Rn
  op(0x35, DMULU, m, n);                //DMULU.L Rm,Rn
  op(0x36, CMPHI, m, n);                //CMP/HI  Rm,Rn
  op(0x37, CMPGT, m, n);                //CMP/GT  Rm,Rn
  op(0x38, SUB, m, n);                  //SUB     Rm,Rn
  op(0x3a, SUBC, m, n);                 //SUBC    Rm,Rn
  op(0x3b, SUBV, m, n);                 //SUBV    Rm,Rn
  op(0x3c, ADD, m, n);                  //ADD     Rm,Rn
  op(0x3d, DMULS, m, n);                //DMULS.L Rm,Rn
  op(0x3e, ADDC, m, n);                 //ADDC    Rm,Rn
  op(0x3f, ADDV, m, n);                 //ADDV    Rm,Rn
  op(0x4f, MACW, m, n);                 //MAC.W   @Rm+,@Rn+
  op(0x50 ... 0x5f, MOVLL4, m, d4, n);  //MOV.L   @(disp,Rm),Rn
  op(0x60, MOVBL, m, n);                //MOV.B   @Rm,Rn
  op(0x61, MOVWL, m, n);                //MOV.W   @Rm,Rn
  op(0x62, MOVLL, m, n);                //MOV.L   @Rm,Rn
  op(0x63, MOV, m, n);                  //MOV     Rm,Rn
  op(0x64, MOVBP, m, n);                //MOV.B   @Rm+,Rn
  op(0x65, MOVWP, m, n);                //MOV.W   @Rm+,Rn
  op(0x66, MOVLP, m, n);                //MOV.L   @Rm+,Rn
  op(0x67, NOT, m, n);                  //NOT     Rm,Rn
  op(0x68, SWAPB, m, n);                //SWAP.B  Rm,Rn
  op(0x69, SWAPW, m, n);                //SWAP.W  Rm,Rn
  op(0x6a, NEGC, m, n);                 //NEGC    Rm,Rn
  op(0x6b, NEG, m, n);                  //NEG     Rm,Rn
  op(0x6c, EXTUB, m, n);                //EXTU.B  Rm,Rn
  op(0x6d, EXTUW, m, n);                //EXTU.W  Rm,Rn
  op(0x6e, EXTSB, m, n);                //EXTS.B  Rm,Rn
  op(0x6f, EXTSW, m, n);                //EXTS.W  Rm,Rn
  op(0x70 ... 0x7f, ADDI, i, n);        //ADD     #imm,Rn
  op(0x90 ... 0x9f, MOVWI, d8, n);      //MOV.W   @(disp,PC),Rn
  br(0xa0 ... 0xaf, BRA, d12);          //BRA     disp
  br(0xb0 ... 0xbf, BSR, d12);          //BSR     disp
  op(0xd0 ... 0xdf, MOVLI, d8, n);      //MOV.L   @(disp.PC),Rn
  op(0xe0 ... 0xef, MOVI, i, n);        //MOV     #imm,Rn
  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8
  #undef d12

  #define n  (opcode >> 8 & 0x0f)
  #define m  (opcode >> 4 & 0x0f)  //n for 0x80,0x81,0x84,0x85
  #define i  (opcode >> 0 & 0xff)
  #define d4 (opcode >> 0 & 0x0f)
  #define d8 (opcode >> 0 & 0xff)
  switch(opcode >> 8) {
  op(0x80, MOVBS4, d4, m);  //MOV.B  R0,@(disp,Rn)
  op(0x81, MOVWS4, d4, m);  //MOV.W  R0,@(disp,Rn)
  op(0x84, MOVBL4, m, d4);  //MOV.B  @(disp,Rm),R0
  op(0x85, MOVWL4, m, d4);  //MOV.W  @(disp,Rm),R0
  op(0x88, CMPIM, i);       //CMP/EQ #imm,R0
  br(0x89, BT, d8);         //BT     disp
  br(0x8b, BF, d8);         //BF     disp
  br(0x8d, BTS, d8);        //BT/S   disp
  br(0x8f, BFS, d8);        //BF/S   disp
  op(0xc0, MOVBSG, d8);     //MOV.B  R0,@(disp,GBR)
  op(0xc1, MOVWSG, d8);     //MOV.W  R0,@(disp,GBR)
  op(0xc2, MOVLSG, d8);     //MOV.L  R0,@(disp,GBR)
  op(0xc3, TRAPA, i);       //TRAPA  #imm
  op(0xc4, MOVBLG, d8);     //MOV.B  @(disp,GBR),R0
  op(0xc5, MOVWLG, d8);     //MOV.W  @(disp,GBR),R0
  op(0xc6, MOVLLG, d8);     //MOV.L  @(disp,GBR),R0
  op(0xc7, MOVA, d8);       //MOVA   @(disp,PC),R0
  op(0xc8, TSTI, i);        //TST    #imm,R0
  op(0xc9, ANDI, i);        //AND    #imm,R0
  op(0xca, XORI, i);        //XOR    #imm,R0
  op(0xcb, ORI, i);         //OR     #imm,R0
  op(0xcc, TSTM, i);        //TST.B  #imm,@(R0,GBR)
  op(0xcd, ANDM, i);        //AND.B  #imm,@(R0,GBR)
  op(0xce, XORM, i);        //XOR.B  #imm,@(R0,GBR)
  op(0xcf, ORM, i);         //OR.B   #imm,@(R0,GBR)
  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8

  #define n (opcode >> 8 & 0xf)
  #define m (opcode >> 8 & 0xf)
  switch(opcode >> 4 & 0x0f00 | opcode & 0x00ff) {
  op(0x002, STCSR, n);     //STC    SR,Rn
  br(0x003, BSRF, m);      //BSRF   Rm
  op(0x00a, STSMACH, n);   //STS    MACH,Rn
  op(0x012, STCGBR, n);    //STC    GBR,Rn
  op(0x01a, STSMACL, n);   //STS    MACL,Rn
  op(0x022, STCVBR, n);    //STC    VBR,Rn
  br(0x023, BRAF, m);      //BRAF   Rm
  op(0x029, MOVT, n);      //MOVT   Rn
  op(0x02a, STSPR, m);     //STS    PR,Rn
  op(0x400, SHLL, n);      //SHLL   Rn
  op(0x401, SHLR, n);      //SHLR   Rn
  op(0x402, STSMMACH, n);  //STS.L  MACH,@-Rn
  op(0x403, STCMSR, n);    //STC.L  SR,@-Rn
  op(0x404, ROTL, n);      //ROTL   Rn
  op(0x405, ROTR, n);      //ROTR   Rn
  op(0x406, LDSMMACH, m);  //LDS.L  @Rm+,MACH
  op(0x407, LDCMSR, m);    //LDC.L  @Rm+,SR
  op(0x408, SHLL2, n);     //SHLL2  Rn
  op(0x409, SHLR2, n);     //SHLR2  Rn
  op(0x40a, LDSMACH, m);   //LDS    Rm,MACH
  br(0x40b, JSR, m);       //JSR    @Rm
  op(0x40e, LDCSR, m);     //LDC    Rm,SR
  op(0x410, DT, n);        //DT     Rn
  op(0x411, CMPPZ, n);     //CMP/PZ Rn
  op(0x412, STSMMACL, n);  //STS.L  MACL,@-Rn
  op(0x413, STCMGBR, n);   //STC.L  GBR,@-Rn
  op(0x415, CMPPL, n);     //CMP/PL Rn
  op(0x416, LDSMMACL, m);  //LDS.L  @Rm+,MACL
  op(0x417, LDCMGBR, m);   //LDC.L  @Rm+,GBR
  op(0x418, SHLL8, n);     //SHLL8  Rn
  op(0x419, SHLR8, n);     //SHLR8  Rn
  op(0x41a, LDSMACL, m);   //LDS    Rm,MACL
  op(0x41b, TAS, n);       //TAS    @Rn
  op(0x41e, LDCGBR, m);    //LDC    Rm,GBR
  op(0x420, SHAL, n);      //SHAL   Rn
  op(0x421, SHAR, n);      //SHAR   Rn
  op(0x422, STSMPR, n);    //STS.L  PR,@-Rn
  op(0x423, STCMVBR, n);   //STC.L  VBR,@-Rn
  op(0x424, ROTCL, n);     //ROTCL  Rn
  op(0x425, ROTCR, n);     //ROTCR  Rn
  op(0x426, LDSMPR, m);    //LDS.L  @Rm+,PR
  op(0x427, LDCMVBR, m);   //LDC.L  @Rm+,VBR
  op(0x428, SHLL16, n);    //SHLL16 Rn
  op(0x429, SHLR16, n);    //SHLR16 Rn
  op(0x42a, LDSPR, m);     //LDS    Rm,PR
  br(0x42b, JMP, m);       //JMP    @Rm
  op(0x42e, LDCVBR, m);    //LDC    Rm,VBR
  }
  #undef n
  #undef m

  switch(opcode) {
  op(0x0008, CLRT);    //CLRT
  op(0x0009, NOP);     //NOP
  br(0x000b, RTS);     //RTS
  op(0x0018, SETT);    //SETT
  op(0x0019, DIV0U);   //DIV0U
  op(0x001b, SLEEP);   //SLEEP
  op(0x0028, CLRMAC);  //CLRMAC
  br(0x002b, RTE);     //RTE
  }

  switch(0) {
  br(0, ILLEGAL);
  }
}
