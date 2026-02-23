#define op(id, name, ...) case id: return instruction##name(__VA_ARGS__);
#define fp(name) &M68HC05::algorithm##name

auto M68HC05::interrupt() -> void {
  push<n16>(PC);
  push<n8>(CCR);
  step(6);
  CCR.I = 1;
  PC = 0;
}

auto M68HC05::instruction() -> void {
  auto opcode = fetch();
  switch(opcode) {
  op(0x00, BRSET, 0);
  op(0x01, BRCLR, 0);
  op(0x02, BRSET, 1);
  op(0x03, BRCLR, 1);
  op(0x04, BRSET, 2);
  op(0x05, BRCLR, 2);
  op(0x06, BRSET, 3);
  op(0x07, BRCLR, 3);
  op(0x08, BRSET, 4);
  op(0x09, BRCLR, 4);
  op(0x0a, BRSET, 5);
  op(0x0b, BRCLR, 5);
  op(0x0c, BRSET, 6);
  op(0x0d, BRCLR, 6);
  op(0x0e, BRSET, 7);
  op(0x0f, BRCLR, 7);
  op(0x10, BSET, 0);
  op(0x11, BCLR, 0);
  op(0x12, BSET, 1);
  op(0x13, BCLR, 1);
  op(0x14, BSET, 2);
  op(0x15, BCLR, 2);
  op(0x16, BSET, 3);
  op(0x17, BCLR, 3);
  op(0x18, BSET, 4);
  op(0x19, BCLR, 4);
  op(0x1a, BSET, 5);
  op(0x1b, BCLR, 5);
  op(0x1c, BSET, 6);
  op(0x1d, BCLR, 6);
  op(0x1e, BSET, 7);
  op(0x1f, BCLR, 7);
  op(0x20, BRA, 1);  //BRA
  op(0x21, BRA, 0);  //BRN
  op(0x22, BRA, CCR.C == 0 && CCR.Z == 0);  //BHI
  op(0x23, BRA, CCR.C == 1 || CCR.Z == 1);  //BLS
  op(0x24, BRA, CCR.C == 0);  //BCC
  op(0x25, BRA, CCR.C == 1);  //BCS
  op(0x26, BRA, CCR.Z == 0);  //BNE
  op(0x27, BRA, CCR.Z == 1);  //BEQ
  op(0x28, BRA, CCR.H == 0);  //BHCC
  op(0x29, BRA, CCR.H == 1);  //BHCS
  op(0x2a, BRA, CCR.N == 0);  //BPL
  op(0x2b, BRA, CCR.N == 1);  //BMI
  op(0x2c, BRA, CCR.I == 0);  //BMC
  op(0x2d, BRA, CCR.I == 1);  //BMS
  op(0x2e, BRA, IRQ == 0);  //BIL
  op(0x2f, BRA, IRQ == 1);  //BIH
  op(0x30, RMWd, fp(NEG));
  op(0x31, ILL);
  op(0x32, ILL);
  op(0x33, RMWd, fp(COM));
  op(0x34, RMWd, fp(LSR));
  op(0x35, ILL);
  op(0x36, RMWd, fp(ROR));
  op(0x37, RMWd, fp(ASR));
  op(0x38, RMWd, fp(LSL));
  op(0x39, RMWd, fp(ROL));
  op(0x3a, RMWd, fp(DEC));
  op(0x3b, ILL);
  op(0x3c, RMWd, fp(INC));
  op(0x3d, TSTd, fp(TST));
  op(0x3e, ILL);
  op(0x3f, RMWd, fp(CLR));
  op(0x40, RMWr, fp(NEG), A);
  op(0x41, ILL);
  op(0x42, MUL);
  op(0x43, RMWr, fp(COM), A);
  op(0x44, RMWr, fp(LSR), A);
  op(0x45, ILL);
  op(0x46, RMWr, fp(ROR), A);
  op(0x47, RMWr, fp(ASR), A);
  op(0x48, RMWr, fp(LSL), A);
  op(0x49, RMWr, fp(ROL), A);
  op(0x4a, RMWr, fp(DEC), A);
  op(0x4b, ILL);
  op(0x4c, RMWr, fp(INC), A);
  op(0x4d, TSTr, fp(TST), A);
  op(0x4e, ILL);
  op(0x4f, RMWr, fp(CLR), A);
  op(0x50, RMWr, fp(NEG), X);
  op(0x51, ILL);
  op(0x52, ILL);
  op(0x53, RMWr, fp(COM), X);
  op(0x54, RMWr, fp(LSR), X);
  op(0x55, ILL);
  op(0x56, RMWr, fp(ROR), X);
  op(0x57, RMWr, fp(ASR), X);
  op(0x58, RMWr, fp(LSL), X);
  op(0x59, RMWr, fp(ROL), X);
  op(0x5a, RMWr, fp(DEC), X);
  op(0x5b, ILL);
  op(0x5c, RMWr, fp(INC), X);
  op(0x5d, TSTr, fp(TST), X);
  op(0x5e, ILL);
  op(0x5f, RMWr, fp(CLR), X);
  op(0x60, RMWo, fp(NEG));
  op(0x61, ILL);
  op(0x62, ILL);
  op(0x63, RMWo, fp(COM));
  op(0x64, RMWo, fp(LSR));
  op(0x65, ILL);
  op(0x66, RMWo, fp(ROR));
  op(0x67, RMWo, fp(ASR));
  op(0x68, RMWo, fp(LSL));
  op(0x69, RMWo, fp(ROL));
  op(0x6a, RMWo, fp(DEC));
  op(0x6b, ILL);
  op(0x6c, RMWo, fp(INC));
  op(0x6d, TSTo, fp(TST));
  op(0x6e, ILL);
  op(0x6f, RMWo, fp(CLR));
  op(0x70, RMWi, fp(NEG));
  op(0x71, ILL);
  op(0x72, ILL);
  op(0x73, RMWi, fp(COM));
  op(0x74, RMWi, fp(LSR));
  op(0x75, ILL);
  op(0x76, RMWi, fp(ROR));
  op(0x77, RMWi, fp(ASR));
  op(0x78, RMWi, fp(LSL));
  op(0x79, RMWi, fp(ROL));
  op(0x7a, RMWi, fp(DEC));
  op(0x7b, ILL);
  op(0x7c, RMWi, fp(INC));
  op(0x7d, TSTi, fp(TST));
  op(0x7e, ILL);
  op(0x7f, RMWi, fp(CLR));
  op(0x80, RTI);
  op(0x81, RTS);
  op(0x82, ILL);
  op(0x83, SWI);
  op(0x84, ILL);
  op(0x85, ILL);
  op(0x86, ILL);
  op(0x87, ILL);
  op(0x88, ILL);
  op(0x89, ILL);
  op(0x8a, ILL);
  op(0x8b, ILL);
  op(0x8c, ILL);
  op(0x8d, ILL);
  op(0x8e, STOP);
  op(0x8f, WAIT);
  op(0x90, ILL);
  op(0x91, ILL);
  op(0x92, ILL);
  op(0x93, ILL);
  op(0x94, ILL);
  op(0x95, ILL);
  op(0x96, ILL);
  op(0x97, TAX);
  op(0x98, CLR, CCR.C);  //CLC
  op(0x99, SET, CCR.C);  //SEC
  op(0x9a, CLR, CCR.I);  //CLI
  op(0x9b, SET, CCR.I);  //SEI
  op(0x9c, RSP);
  op(0x9d, NOP);
  op(0x9e, ILL);
  op(0x9f, TXA);
  op(0xa0, LDr, fp(SUB), A);
  op(0xa1, LDr, fp(CMP), A);
  op(0xa2, LDr, fp(SBC), A);
  op(0xa3, LDr, fp(CMP), X);
  op(0xa4, LDr, fp(AND), A);
  op(0xa5, LDr, fp(BIT), A);
  op(0xa6, LDr, fp(LD ), A);
  op(0xa7, ILL);
  op(0xa8, LDr, fp(EOR), A);
  op(0xa9, LDr, fp(ADC), A);
  op(0xaa, LDr, fp(OR ), A);
  op(0xab, LDr, fp(ADD), A);
  op(0xac, ILL);
  op(0xad, BSR);
  op(0xae, LDr, fp(LD ), X);
  op(0xaf, ILL);
  op(0xb0, LDd, fp(SUB), A);
  op(0xb1, LDd, fp(CMP), A);
  op(0xb2, LDd, fp(SBC), A);
  op(0xb3, LDd, fp(CMP), X);
  op(0xb4, LDd, fp(AND), A);
  op(0xb5, LDd, fp(BIT), A);
  op(0xb6, LDd, fp(LD ), A);
  op(0xb7, STd, A);
  op(0xb8, LDd, fp(EOR), A);
  op(0xb9, LDd, fp(ADC), A);
  op(0xba, LDd, fp(OR ), A);
  op(0xbb, LDd, fp(ADD), A);
  op(0xbc, JMPd);
  op(0xbd, JSRd);
  op(0xbe, LDd, fp(LD ), X);
  op(0xbf, STd, X);
  op(0xc0, LDa, fp(SUB), A);
  op(0xc1, LDa, fp(CMP), A);
  op(0xc2, LDa, fp(SBC), A);
  op(0xc3, LDa, fp(CMP), X);
  op(0xc4, LDa, fp(AND), A);
  op(0xc5, LDa, fp(BIT), A);
  op(0xc6, LDa, fp(LD ), A);
  op(0xc7, STa, A);
  op(0xc8, LDa, fp(EOR), A);
  op(0xc9, LDa, fp(ADC), A);
  op(0xca, LDa, fp(OR ), A);
  op(0xcb, LDa, fp(ADD), A);
  op(0xcc, JMPa);
  op(0xcd, JSRa);
  op(0xce, LDa, fp(LD ), X);
  op(0xcf, STa, X);
  op(0xd0, LDw, fp(SUB), A);
  op(0xd1, LDw, fp(CMP), A);
  op(0xd2, LDw, fp(SBC), A);
  op(0xd3, LDw, fp(CMP), X);
  op(0xd4, LDw, fp(AND), A);
  op(0xd5, LDw, fp(BIT), A);
  op(0xd6, LDw, fp(LD ), A);
  op(0xd7, STw, A);
  op(0xd8, LDw, fp(EOR), A);
  op(0xd9, LDw, fp(ADC), A);
  op(0xda, LDw, fp(OR ), A);
  op(0xdb, LDw, fp(ADD), A);
  op(0xdc, JMPw);
  op(0xdd, JSRw);
  op(0xde, LDw, fp(LD ), X);
  op(0xdf, STw, X);
  op(0xe0, LDo, fp(SUB), A);
  op(0xe1, LDo, fp(CMP), A);
  op(0xe2, LDo, fp(SBC), A);
  op(0xe3, LDo, fp(CMP), X);
  op(0xe4, LDo, fp(AND), A);
  op(0xe5, LDo, fp(BIT), A);
  op(0xe6, LDo, fp(LD ), A);
  op(0xe7, STo, A);
  op(0xe8, LDo, fp(EOR), A);
  op(0xe9, LDo, fp(ADC), A);
  op(0xea, LDo, fp(OR ), A);
  op(0xeb, LDo, fp(ADD), A);
  op(0xec, JMPo);
  op(0xed, JSRo);
  op(0xee, LDo, fp(LD ), X);
  op(0xef, STo, X);
  op(0xf0, LDi, fp(SUB), A);
  op(0xf1, LDi, fp(CMP), A);
  op(0xf2, LDi, fp(SBC), A);
  op(0xf3, LDi, fp(CMP), X);
  op(0xf4, LDi, fp(AND), A);
  op(0xf5, LDi, fp(BIT), A);
  op(0xf6, LDi, fp(LD ), A);
  op(0xf7, STi, A);
  op(0xf8, LDi, fp(EOR), A);
  op(0xf9, LDi, fp(ADC), A);
  op(0xfa, LDi, fp(OR ), A);
  op(0xfb, LDi, fp(ADD), A);
  op(0xfc, JMPi);
  op(0xfd, JSRi);
  op(0xfe, LDi, fp(LD ), X);
  op(0xff, STi, X);
  }
}

#undef op
#undef fp
