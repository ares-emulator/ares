auto M68HC05::disassembleInstruction() -> string {
  string s;

  n8 opcode = read(PC);
  n8 lo = read(PC + 1);
  n8 hi = read(PC + 2);

  auto absolute = [&]() -> string {
    return {"$", hex(hi, 2L), hex(lo, 2L)};
  };

  auto branchBit = [&](n3 bit) -> string {
    return {bit, ",$", hex(lo, 2L), ",$", hex(PC + 3 + (i8)hi, 4L)};
  };

  auto bit = [&](n3 bit) -> string {
    return {bit, ",$", hex(lo, 2L)};
  };

  auto branch = [&]() -> string {
    return {"$", hex(PC + 2 + (i8)lo, 4L)};
  };

  auto direct = [&]() -> string {
    return {"$", hex(lo, 2L)};
  };

  auto illegal = [&]() -> string {
    return {"$", hex(opcode, 2L)};
  };

  auto immediate = [&]() -> string {
    return {"#$", hex(lo, 2L)};
  };

  auto implied = [&]() -> string {
    return {};
  };

  auto indexed = [&]() -> string {
    return {"(x)"};
  };

  auto indexed8 = [&]() -> string {
    return {"(x+$", hex(lo, 2L)};
  };

  auto indexed16 = [&]() -> string {
    return {"(x+$", hex(hi, 2L), hex(lo, 2L)};
  };

  auto inherent = [&](string r) -> string {
    return r;
  };

  auto multiply = [&]() -> string {
    return {"a,x"};
  };

  #define op(id, prefix, mode, ...) \
    case id: s.append(prefix, " ", mode(__VA_ARGS__)); \
    break;
  switch(opcode) {
  op(0x00, "brset", branchBit, 0);
  op(0x01, "brclr", branchBit, 0);
  op(0x02, "brset", branchBit, 1);
  op(0x03, "brclr", branchBit, 1);
  op(0x04, "brset", branchBit, 2);
  op(0x05, "brclr", branchBit, 2);
  op(0x06, "brset", branchBit, 3);
  op(0x07, "brclr", branchBit, 3);
  op(0x08, "brset", branchBit, 4);
  op(0x09, "brclr", branchBit, 4);
  op(0x0a, "brset", branchBit, 5);
  op(0x0b, "brclr", branchBit, 5);
  op(0x0c, "brset", branchBit, 6);
  op(0x0d, "brclr", branchBit, 6);
  op(0x0e, "brset", branchBit, 7);
  op(0x0f, "brclr", branchBit, 7);
  op(0x10, "bset ", bit, 0);
  op(0x11, "bclr ", bit, 0);
  op(0x12, "bset ", bit, 1);
  op(0x13, "bclr ", bit, 1);
  op(0x14, "bset ", bit, 2);
  op(0x15, "bclr ", bit, 2);
  op(0x16, "bset ", bit, 3);
  op(0x17, "bclr ", bit, 3);
  op(0x18, "bset ", bit, 4);
  op(0x19, "bclr ", bit, 4);
  op(0x1a, "bset ", bit, 5);
  op(0x1b, "bclr ", bit, 5);
  op(0x1c, "bset ", bit, 6);
  op(0x1d, "bclr ", bit, 6);
  op(0x1e, "bset ", bit, 7);
  op(0x1f, "bclr ", bit, 7);
  op(0x20, "bra  ", branch);
  op(0x21, "brn  ", branch);
  op(0x22, "bhi  ", branch);
  op(0x23, "bls  ", branch);
  op(0x24, "bcc  ", branch);
  op(0x25, "bcs  ", branch);
  op(0x26, "bne  ", branch);
  op(0x27, "beq  ", branch);
  op(0x28, "bhcc ", branch);
  op(0x29, "bhcs ", branch);
  op(0x2a, "bpl  ", branch);
  op(0x2b, "bmi  ", branch);
  op(0x2c, "bmc  ", branch);
  op(0x2d, "bms  ", branch);
  op(0x2e, "bil  ", branch);
  op(0x2f, "bih  ", branch);
  op(0x30, "neg  ", direct);
  op(0x31, "ill  ", illegal);
  op(0x32, "ill  ", illegal);
  op(0x33, "com  ", direct);
  op(0x34, "lsr  ", direct);
  op(0x35, "ill  ", illegal);
  op(0x36, "ror  ", direct);
  op(0x37, "asr  ", direct);
  op(0x38, "lsl  ", direct);
  op(0x39, "rol  ", direct);
  op(0x3a, "dec  ", direct);
  op(0x3b, "ill  ", illegal);
  op(0x3c, "inc  ", direct);
  op(0x3d, "tst  ", direct);
  op(0x3e, "ill  ", illegal);
  op(0x3f, "clr  ", direct);
  op(0x40, "neg  ", inherent, "a");
  op(0x41, "ill  ", illegal);
  op(0x42, "mul  ", multiply);
  op(0x43, "com  ", inherent, "a");
  op(0x44, "lsr  ", inherent, "a");
  op(0x45, "ill  ", illegal);
  op(0x46, "ror  ", inherent, "a");
  op(0x47, "asr  ", inherent, "a");
  op(0x48, "lsl  ", inherent, "a");
  op(0x49, "rol  ", inherent, "a");
  op(0x4a, "dec  ", inherent, "a");
  op(0x4b, "ill  ", illegal);
  op(0x4c, "inc  ", inherent, "a");
  op(0x4d, "tst  ", inherent, "a");
  op(0x4e, "ill  ", illegal);
  op(0x4f, "clr  ", inherent, "a");
  op(0x50, "neg  ", inherent, "x");
  op(0x51, "ill  ", illegal);
  op(0x52, "ill  ", illegal);
  op(0x53, "com  ", inherent, "x");
  op(0x54, "lsr  ", inherent, "x");
  op(0x55, "ill  ", illegal);
  op(0x56, "ror  ", inherent, "x");
  op(0x57, "asr  ", inherent, "x");
  op(0x58, "lsl  ", inherent, "x");
  op(0x59, "rol  ", inherent, "x");
  op(0x5a, "dec  ", inherent, "x");
  op(0x5b, "ill  ", illegal);
  op(0x5c, "inc  ", inherent, "x");
  op(0x5d, "tst  ", inherent, "x");
  op(0x5e, "ill  ", illegal);
  op(0x5f, "clr  ", inherent, "x");
  op(0x60, "neg  ", indexed8);
  op(0x61, "ill  ", illegal);
  op(0x62, "ill  ", illegal);
  op(0x63, "com  ", indexed8);
  op(0x64, "lsr  ", indexed8);
  op(0x65, "ill  ", illegal);
  op(0x66, "ror  ", indexed8);
  op(0x67, "asr  ", indexed8);
  op(0x68, "lsl  ", indexed8);
  op(0x69, "rol  ", indexed8);
  op(0x6a, "dec  ", indexed8);
  op(0x6b, "ill  ", illegal);
  op(0x6c, "inc  ", indexed8);
  op(0x6d, "tst  ", indexed8);
  op(0x6e, "ill  ", illegal);
  op(0x6f, "clr  ", indexed8);
  op(0x70, "neg  ", indexed);
  op(0x71, "ill  ", illegal);
  op(0x72, "ill  ", illegal);
  op(0x73, "com  ", indexed);
  op(0x74, "lsr  ", indexed);
  op(0x75, "ill  ", illegal);
  op(0x76, "ror  ", indexed);
  op(0x77, "asr  ", indexed);
  op(0x78, "lsl  ", indexed);
  op(0x79, "rol  ", indexed);
  op(0x7a, "dec  ", indexed);
  op(0x7b, "ill  ", illegal);
  op(0x7c, "inc  ", indexed);
  op(0x7d, "tst  ", indexed);
  op(0x7e, "ill  ", illegal);
  op(0x7f, "clr  ", indexed);
  op(0x80, "rti  ", implied);
  op(0x81, "rts  ", implied);
  op(0x82, "ill  ", illegal);
  op(0x83, "swi  ", implied);
  op(0x84, "ill  ", illegal);
  op(0x85, "ill  ", illegal);
  op(0x86, "ill  ", illegal);
  op(0x87, "ill  ", illegal);
  op(0x88, "ill  ", illegal);
  op(0x89, "ill  ", illegal);
  op(0x8a, "ill  ", illegal);
  op(0x8b, "ill  ", illegal);
  op(0x8c, "ill  ", illegal);
  op(0x8d, "ill  ", illegal);
  op(0x8e, "stop ", implied);
  op(0x8f, "wait ", implied);
  op(0x90, "ill  ", illegal);
  op(0x91, "ill  ", illegal);
  op(0x92, "ill  ", illegal);
  op(0x93, "ill  ", illegal);
  op(0x94, "ill  ", illegal);
  op(0x95, "ill  ", illegal);
  op(0x96, "ill  ", illegal);
  op(0x97, "tax  ", implied);
  op(0x98, "clc  ", implied);
  op(0x99, "sec  ", implied);
  op(0x9a, "cli  ", implied);
  op(0x9b, "sei  ", implied);
  op(0x9c, "rsp  ", implied);
  op(0x9d, "nop  ", implied);
  op(0x9e, "ill  ", illegal);
  op(0x9f, "txa  ", implied);
  op(0xa0, "sub  ", immediate);
  op(0xa1, "cmp  ", immediate);
  op(0xa2, "sbc  ", immediate);
  op(0xa3, "cpx  ", immediate);
  op(0xa4, "and  ", immediate);
  op(0xa5, "bit  ", immediate);
  op(0xa6, "lda  ", immediate);
  op(0xa7, "ill  ", illegal);
  op(0xa8, "eor  ", immediate);
  op(0xa9, "adc  ", immediate);
  op(0xaa, "ora  ", immediate);
  op(0xab, "add  ", immediate);
  op(0xac, "ill  ", illegal);
  op(0xad, "bsr  ", branch);
  op(0xae, "ldx  ", immediate);
  op(0xaf, "ill  ", illegal);
  op(0xb0, "lda  ", direct);
  op(0xb1, "cmp  ", direct);
  op(0xb2, "sbc  ", direct);
  op(0xb3, "cpx  ", direct);
  op(0xb4, "and  ", direct);
  op(0xb5, "bit  ", direct);
  op(0xb6, "lda  ", direct);
  op(0xb7, "sta  ", direct);
  op(0xb8, "eor  ", direct);
  op(0xb9, "adc  ", direct);
  op(0xba, "ora  ", direct);
  op(0xbb, "add  ", direct);
  op(0xbc, "jmp  ", direct);
  op(0xbd, "jsr  ", direct);
  op(0xbe, "ldx  ", direct);
  op(0xbf, "stx  ", direct);
  op(0xc0, "sub  ", absolute);
  op(0xc1, "cmp  ", absolute);
  op(0xc2, "sbc  ", absolute);
  op(0xc3, "cpx  ", absolute);
  op(0xc4, "and  ", absolute);
  op(0xc5, "bit  ", absolute);
  op(0xc6, "lda  ", absolute);
  op(0xc7, "sta  ", absolute);
  op(0xc8, "eor  ", absolute);
  op(0xc9, "adc  ", absolute);
  op(0xca, "ora  ", absolute);
  op(0xcb, "add  ", absolute);
  op(0xcc, "jmp  ", absolute);
  op(0xcd, "jsr  ", absolute);
  op(0xce, "ldx  ", absolute);
  op(0xcf, "stx  ", absolute);
  op(0xd0, "sub  ", indexed16);
  op(0xd1, "cmp  ", indexed16);
  op(0xd2, "sbc  ", indexed16);
  op(0xd3, "cpx  ", indexed16);
  op(0xd4, "and  ", indexed16);
  op(0xd5, "bit  ", indexed16);
  op(0xd6, "lda  ", indexed16);
  op(0xd7, "sta  ", indexed16);
  op(0xd8, "eor  ", indexed16);
  op(0xd9, "adc  ", indexed16);
  op(0xda, "ora  ", indexed16);
  op(0xdb, "add  ", indexed16);
  op(0xdc, "jmp  ", indexed16);
  op(0xdd, "jsr  ", indexed16);
  op(0xde, "ldx  ", indexed16);
  op(0xdf, "stx  ", indexed16);
  op(0xe0, "sub  ", indexed8);
  op(0xe1, "cmp  ", indexed8);
  op(0xe2, "sbc  ", indexed8);
  op(0xe3, "cpx  ", indexed8);
  op(0xe4, "and  ", indexed8);
  op(0xe5, "bit  ", indexed8);
  op(0xe6, "lda  ", indexed8);
  op(0xe7, "sta  ", indexed8);
  op(0xe8, "eor  ", indexed8);
  op(0xe9, "adc  ", indexed8);
  op(0xea, "ora  ", indexed8);
  op(0xeb, "add  ", indexed8);
  op(0xec, "jmp  ", indexed8);
  op(0xed, "jsr  ", indexed8);
  op(0xee, "ldx  ", indexed8);
  op(0xef, "stx  ", indexed8);
  op(0xf0, "sub  ", indexed);
  op(0xf1, "cmp  ", indexed);
  op(0xf2, "sbc  ", indexed);
  op(0xf3, "cpx  ", indexed);
  op(0xf4, "and  ", indexed);
  op(0xf5, "bit  ", indexed);
  op(0xf6, "lda  ", indexed);
  op(0xf7, "sta  ", indexed);
  op(0xf8, "eor  ", indexed);
  op(0xf9, "adc  ", indexed);
  op(0xfa, "ora  ", indexed);
  op(0xfb, "add  ", indexed);
  op(0xfc, "jmp  ", indexed);
  op(0xfd, "jsr  ", indexed);
  op(0xfe, "ldx  ", indexed);
  op(0xff, "stx  ", indexed);
  }
  #undef op

  return s;
}

auto M68HC05::disassembleContext() -> string {
  string s;
  s.append("A:", hex(A, 2L), " ");
  s.append("X:", hex(X, 2L), " ");
  s.append("S:", hex(0xc0 | SP, 2L), " ");
  s.append(CCR.C ? "C" : "c");
  s.append(CCR.Z ? "Z" : "z");
  s.append(CCR.N ? "N" : "n");
  s.append(CCR.I ? "I" : "i");
  s.append(CCR.H ? "H" : "h");
  return s;
}
