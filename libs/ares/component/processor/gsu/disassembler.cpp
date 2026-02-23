auto GSU::disassembleInstruction() -> string {
  char s[256];
  disassembleOpcode(s);
  return pad(s, -14);
}

auto GSU::disassembleContext() -> string {
  string s;

  for(u32 n : range(16)) {
    s.append("r", n, ":", hex(regs.r[n].data, 4L), " ");
  }

  s.append("...");  //todo: add SFR flags

  return s;
}

auto GSU::disassembleOpcode(char* output) -> void {
  *output = 0;

  switch(regs.sfr.alt2 << 1 | regs.sfr.alt1 << 0) {
  case 0: disassembleALT0(output); break;
  case 1: disassembleALT1(output); break;
  case 2: disassembleALT2(output); break;
  case 3: disassembleALT3(output); break;
  }

  u32 length = strlen(output);
  while(length++ < 20) strcat(output, " ");
}

#define op0 regs.pipeline
#define op1 read((regs.pbr << 16) + regs.r[15] + 0)
#define op2 read((regs.pbr << 16) + regs.r[15] + 1)

auto GSU::disassembleALT0(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00):snprintf(t, sizeof(t), "stop"); break;
    case  (0x01):snprintf(t, sizeof(t), "nop"); break;
    case  (0x02):snprintf(t, sizeof(t), "cache"); break;
    case  (0x03):snprintf(t, sizeof(t), "lsr"); break;
    case  (0x04):snprintf(t, sizeof(t), "rol"); break;
    case  (0x05):snprintf(t, sizeof(t), "bra %+d", (s8)op1); break;
    case  (0x06):snprintf(t, sizeof(t), "blt %+d", (s8)op1); break;
    case  (0x07):snprintf(t, sizeof(t), "bge %+d", (s8)op1); break;
    case  (0x08):snprintf(t, sizeof(t), "bne %+d", (s8)op1); break;
    case  (0x09):snprintf(t, sizeof(t), "beq %+d", (s8)op1); break;
    case  (0x0a):snprintf(t, sizeof(t), "bpl %+d", (s8)op1); break;
    case  (0x0b):snprintf(t, sizeof(t), "bmi %+d", (s8)op1); break;
    case  (0x0c):snprintf(t, sizeof(t), "bcc %+d", (s8)op1); break;
    case  (0x0d):snprintf(t, sizeof(t), "bcs %+d", (s8)op1); break;
    case  (0x0e):snprintf(t, sizeof(t), "bvc %+d", (s8)op1); break;
    case  (0x0f):snprintf(t, sizeof(t), "bvs %+d", (s8)op1); break;
    case16(0x10):snprintf(t, sizeof(t), "to r%u", op0 & 15); break;
    case16(0x20):snprintf(t, sizeof(t), "with r%u", op0 & 15); break;
    case12(0x30):snprintf(t, sizeof(t), "stw (r%u)", op0 & 15); break;
    case  (0x3c):snprintf(t, sizeof(t), "loop"); break;
    case  (0x3d):snprintf(t, sizeof(t), "alt1"); break;
    case  (0x3e):snprintf(t, sizeof(t), "alt2"); break;
    case  (0x3f):snprintf(t, sizeof(t), "alt3"); break;
    case12(0x40):snprintf(t, sizeof(t), "ldw (r%u)", op0 & 15); break;
    case  (0x4c):snprintf(t, sizeof(t), "plot"); break;
    case  (0x4d):snprintf(t, sizeof(t), "swap"); break;
    case  (0x4e):snprintf(t, sizeof(t), "color"); break;
    case  (0x4f):snprintf(t, sizeof(t), "not"); break;
    case16(0x50):snprintf(t, sizeof(t), "add r%u", op0 & 15); break;
    case16(0x60):snprintf(t, sizeof(t), "sub r%u", op0 & 15); break;
    case  (0x70):snprintf(t, sizeof(t), "merge"); break;
    case15(0x71):snprintf(t, sizeof(t), "and r%u", op0 & 15); break;
    case16(0x80):snprintf(t, sizeof(t), "mult r%u", op0 & 15); break;
    case  (0x90):snprintf(t, sizeof(t), "sbk"); break;
    case4 (0x91):snprintf(t, sizeof(t), "link #%u", op0 & 15); break;
    case  (0x95):snprintf(t, sizeof(t), "sex"); break;
    case  (0x96):snprintf(t, sizeof(t), "asr"); break;
    case  (0x97):snprintf(t, sizeof(t), "ror"); break;
    case6 (0x98):snprintf(t, sizeof(t), "jmp r%u", op0 & 15); break;
    case  (0x9e):snprintf(t, sizeof(t), "lob"); break;
    case  (0x9f):snprintf(t, sizeof(t), "fmult"); break;
    case16(0xa0):snprintf(t, sizeof(t), "ibt r%u,#$%.2x", op0 & 15, op1 & 255); break;
    case16(0xb0):snprintf(t, sizeof(t), "from r%u", op0 & 15); break;
    case  (0xc0):snprintf(t, sizeof(t), "hib"); break;
    case15(0xc1):snprintf(t, sizeof(t), "or r%u", op0 & 15); break;
    case15(0xd0):snprintf(t, sizeof(t), "inc r%u", op0 & 15); break;
    case  (0xdf):snprintf(t, sizeof(t), "getc"); break;
    case15(0xe0):snprintf(t, sizeof(t), "dec r%u", op0 & 15); break;
    case  (0xef):snprintf(t, sizeof(t), "getb"); break;
    case16(0xf0):snprintf(t, sizeof(t), "iwt r%u,#$%.2x%.2x", op0 & 15, op2 & 255, op1 & 255); break;
  }
  strcat(output, t);
}

auto GSU::disassembleALT1(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00):snprintf(t, sizeof(t), "stop"); break;
    case  (0x01):snprintf(t, sizeof(t), "nop"); break;
    case  (0x02):snprintf(t, sizeof(t), "cache"); break;
    case  (0x03):snprintf(t, sizeof(t), "lsr"); break;
    case  (0x04):snprintf(t, sizeof(t), "rol"); break;
    case  (0x05):snprintf(t, sizeof(t), "bra %+d", (s8)op1); break;
    case  (0x06):snprintf(t, sizeof(t), "blt %+d", (s8)op1); break;
    case  (0x07):snprintf(t, sizeof(t), "bge %+d", (s8)op1); break;
    case  (0x08):snprintf(t, sizeof(t), "bne %+d", (s8)op1); break;
    case  (0x09):snprintf(t, sizeof(t), "beq %+d", (s8)op1); break;
    case  (0x0a):snprintf(t, sizeof(t), "bpl %+d", (s8)op1); break;
    case  (0x0b):snprintf(t, sizeof(t), "bmi %+d", (s8)op1); break;
    case  (0x0c):snprintf(t, sizeof(t), "bcc %+d", (s8)op1); break;
    case  (0x0d):snprintf(t, sizeof(t), "bcs %+d", (s8)op1); break;
    case  (0x0e):snprintf(t, sizeof(t), "bvc %+d", (s8)op1); break;
    case  (0x0f):snprintf(t, sizeof(t), "bvs %+d", (s8)op1); break;
    case16(0x10):snprintf(t, sizeof(t), "to r%u", op0 & 15); break;
    case16(0x20):snprintf(t, sizeof(t), "with r%u", op0 & 15); break;
    case12(0x30):snprintf(t, sizeof(t), "stb (r%u)", op0 & 15); break;
    case  (0x3c):snprintf(t, sizeof(t), "loop"); break;
    case  (0x3d):snprintf(t, sizeof(t), "alt1"); break;
    case  (0x3e):snprintf(t, sizeof(t), "alt2"); break;
    case  (0x3f):snprintf(t, sizeof(t), "alt3"); break;
    case12(0x40):snprintf(t, sizeof(t), "ldb (r%u)", op0 & 15); break;
    case  (0x4c):snprintf(t, sizeof(t), "rpix"); break;
    case  (0x4d):snprintf(t, sizeof(t), "swap"); break;
    case  (0x4e):snprintf(t, sizeof(t), "cmode"); break;
    case  (0x4f):snprintf(t, sizeof(t), "not"); break;
    case16(0x50):snprintf(t, sizeof(t), "adc r%u", op0 & 15); break;
    case16(0x60):snprintf(t, sizeof(t), "sbc r%u", op0 & 15); break;
    case  (0x70):snprintf(t, sizeof(t), "merge"); break;
    case15(0x71):snprintf(t, sizeof(t), "bic r%u", op0 & 15); break;
    case16(0x80):snprintf(t, sizeof(t), "umult r%u", op0 & 15); break;
    case  (0x90):snprintf(t, sizeof(t), "sbk"); break;
    case4 (0x91):snprintf(t, sizeof(t), "link #%u", op0 & 15); break;
    case  (0x95):snprintf(t, sizeof(t), "sex"); break;
    case  (0x96):snprintf(t, sizeof(t), "div2"); break;
    case  (0x97):snprintf(t, sizeof(t), "ror"); break;
    case6 (0x98):snprintf(t, sizeof(t), "ljmp r%u", op0 & 15); break;
    case  (0x9e):snprintf(t, sizeof(t), "lob"); break;
    case  (0x9f):snprintf(t, sizeof(t), "lmult"); break;
    case16(0xa0):snprintf(t, sizeof(t), "lms r%u,(#$%.4x)", op0 & 15, op1 << 1); break;
    case16(0xb0):snprintf(t, sizeof(t), "from r%u", op0 & 15); break;
    case  (0xc0):snprintf(t, sizeof(t), "hib"); break;
    case15(0xc1):snprintf(t, sizeof(t), "xor r%u", op0 & 15); break;
    case15(0xd0):snprintf(t, sizeof(t), "inc r%u", op0 & 15); break;
    case  (0xdf):snprintf(t, sizeof(t), "getc"); break;
    case15(0xe0):snprintf(t, sizeof(t), "dec r%u", op0 & 15); break;
    case  (0xef):snprintf(t, sizeof(t), "getbh"); break;
    case16(0xf0):snprintf(t, sizeof(t), "lm r%u", op0 & 15); break;
  }
  strcat(output, t);
}

auto GSU::disassembleALT2(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00):snprintf(t, sizeof(t), "stop"); break;
    case  (0x01):snprintf(t, sizeof(t), "nop"); break;
    case  (0x02):snprintf(t, sizeof(t), "cache"); break;
    case  (0x03):snprintf(t, sizeof(t), "lsr"); break;
    case  (0x04):snprintf(t, sizeof(t), "rol"); break;
    case  (0x05):snprintf(t, sizeof(t), "bra %+d", (s8)op1); break;
    case  (0x06):snprintf(t, sizeof(t), "blt %+d", (s8)op1); break;
    case  (0x07):snprintf(t, sizeof(t), "bge %+d", (s8)op1); break;
    case  (0x08):snprintf(t, sizeof(t), "bne %+d", (s8)op1); break;
    case  (0x09):snprintf(t, sizeof(t), "beq %+d", (s8)op1); break;
    case  (0x0a):snprintf(t, sizeof(t), "bpl %+d", (s8)op1); break;
    case  (0x0b):snprintf(t, sizeof(t), "bmi %+d", (s8)op1); break;
    case  (0x0c):snprintf(t, sizeof(t), "bcc %+d", (s8)op1); break;
    case  (0x0d):snprintf(t, sizeof(t), "bcs %+d", (s8)op1); break;
    case  (0x0e):snprintf(t, sizeof(t), "bvc %+d", (s8)op1); break;
    case  (0x0f):snprintf(t, sizeof(t), "bvs %+d", (s8)op1); break;
    case16(0x10):snprintf(t, sizeof(t), "to r%u", op0 & 15); break;
    case16(0x20):snprintf(t, sizeof(t), "with r%u", op0 & 15); break;
    case12(0x30):snprintf(t, sizeof(t), "stw (r%u)", op0 & 15); break;
    case  (0x3c):snprintf(t, sizeof(t), "loop"); break;
    case  (0x3d):snprintf(t, sizeof(t), "alt1"); break;
    case  (0x3e):snprintf(t, sizeof(t), "alt2"); break;
    case  (0x3f):snprintf(t, sizeof(t), "alt3"); break;
    case12(0x40):snprintf(t, sizeof(t), "ldw (r%u)", op0 & 15); break;
    case  (0x4c):snprintf(t, sizeof(t), "plot"); break;
    case  (0x4d):snprintf(t, sizeof(t), "swap"); break;
    case  (0x4e):snprintf(t, sizeof(t), "color"); break;
    case  (0x4f):snprintf(t, sizeof(t), "not"); break;
    case16(0x50):snprintf(t, sizeof(t), "add #%u", op0 & 15); break;
    case16(0x60):snprintf(t, sizeof(t), "sub #%u", op0 & 15); break;
    case  (0x70):snprintf(t, sizeof(t), "merge"); break;
    case15(0x71):snprintf(t, sizeof(t), "and #%u", op0 & 15); break;
    case16(0x80):snprintf(t, sizeof(t), "mult #%u", op0 & 15); break;
    case  (0x90):snprintf(t, sizeof(t), "sbk"); break;
    case4 (0x91):snprintf(t, sizeof(t), "link #%u", op0 & 15); break;
    case  (0x95):snprintf(t, sizeof(t), "sex"); break;
    case  (0x96):snprintf(t, sizeof(t), "asr"); break;
    case  (0x97):snprintf(t, sizeof(t), "ror"); break;
    case6 (0x98):snprintf(t, sizeof(t), "jmp r%u", op0 & 15); break;
    case  (0x9e):snprintf(t, sizeof(t), "lob"); break;
    case  (0x9f):snprintf(t, sizeof(t), "fmult"); break;
    case16(0xa0):snprintf(t, sizeof(t), "sms r%u,(#$%.4x)", op0 & 15, op1 << 1); break;
    case16(0xb0):snprintf(t, sizeof(t), "from r%u", op0 & 15); break;
    case  (0xc0):snprintf(t, sizeof(t), "hib"); break;
    case15(0xc1):snprintf(t, sizeof(t), "or #%u", op0 & 15); break;
    case15(0xd0):snprintf(t, sizeof(t), "inc r%u", op0 & 15); break;
    case  (0xdf):snprintf(t, sizeof(t), "ramb"); break;
    case15(0xe0):snprintf(t, sizeof(t), "dec r%u", op0 & 15); break;
    case  (0xef):snprintf(t, sizeof(t), "getbl"); break;
    case16(0xf0):snprintf(t, sizeof(t), "sm r%u", op0 & 15); break;
  }
  strcat(output, t);
}

auto GSU::disassembleALT3(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00):snprintf(t, sizeof(t), "stop"); break;
    case  (0x01):snprintf(t, sizeof(t), "nop"); break;
    case  (0x02):snprintf(t, sizeof(t), "cache"); break;
    case  (0x03):snprintf(t, sizeof(t), "lsr"); break;
    case  (0x04):snprintf(t, sizeof(t), "rol"); break;
    case  (0x05):snprintf(t, sizeof(t), "bra %+d", (s8)op1); break;
    case  (0x06):snprintf(t, sizeof(t), "blt %+d", (s8)op1); break;
    case  (0x07):snprintf(t, sizeof(t), "bge %+d", (s8)op1); break;
    case  (0x08):snprintf(t, sizeof(t), "bne %+d", (s8)op1); break;
    case  (0x09):snprintf(t, sizeof(t), "beq %+d", (s8)op1); break;
    case  (0x0a):snprintf(t, sizeof(t), "bpl %+d", (s8)op1); break;
    case  (0x0b):snprintf(t, sizeof(t), "bmi %+d", (s8)op1); break;
    case  (0x0c):snprintf(t, sizeof(t), "bcc %+d", (s8)op1); break;
    case  (0x0d):snprintf(t, sizeof(t), "bcs %+d", (s8)op1); break;
    case  (0x0e):snprintf(t, sizeof(t), "bvc %+d", (s8)op1); break;
    case  (0x0f):snprintf(t, sizeof(t), "bvs %+d", (s8)op1); break;
    case16(0x10):snprintf(t, sizeof(t), "to r%u", op0 & 15); break;
    case16(0x20):snprintf(t, sizeof(t), "with r%u", op0 & 15); break;
    case12(0x30):snprintf(t, sizeof(t), "stb (r%u)", op0 & 15); break;
    case  (0x3c):snprintf(t, sizeof(t), "loop"); break;
    case  (0x3d):snprintf(t, sizeof(t), "alt1"); break;
    case  (0x3e):snprintf(t, sizeof(t), "alt2"); break;
    case  (0x3f):snprintf(t, sizeof(t), "alt3"); break;
    case12(0x40):snprintf(t, sizeof(t), "ldb (r%u)", op0 & 15); break;
    case  (0x4c):snprintf(t, sizeof(t), "rpix"); break;
    case  (0x4d):snprintf(t, sizeof(t), "swap"); break;
    case  (0x4e):snprintf(t, sizeof(t), "cmode"); break;
    case  (0x4f):snprintf(t, sizeof(t), "not"); break;
    case16(0x50):snprintf(t, sizeof(t), "adc #%u", op0 & 15); break;
    case16(0x60):snprintf(t, sizeof(t), "cmp r%u", op0 & 15); break;
    case  (0x70):snprintf(t, sizeof(t), "merge"); break;
    case15(0x71):snprintf(t, sizeof(t), "bic #%u", op0 & 15); break;
    case16(0x80):snprintf(t, sizeof(t), "umult #%u", op0 & 15); break;
    case  (0x90):snprintf(t, sizeof(t), "sbk"); break;
    case4 (0x91):snprintf(t, sizeof(t), "link #%u", op0 & 15); break;
    case  (0x95):snprintf(t, sizeof(t), "sex"); break;
    case  (0x96):snprintf(t, sizeof(t), "div2"); break;
    case  (0x97):snprintf(t, sizeof(t), "ror"); break;
    case6 (0x98):snprintf(t, sizeof(t), "ljmp r%u", op0 & 15); break;
    case  (0x9e):snprintf(t, sizeof(t), "lob"); break;
    case  (0x9f):snprintf(t, sizeof(t), "lmult"); break;
    case16(0xa0):snprintf(t, sizeof(t), "lms r%u", op0 & 15); break;
    case16(0xb0):snprintf(t, sizeof(t), "from r%u", op0 & 15); break;
    case  (0xc0):snprintf(t, sizeof(t), "hib"); break;
    case15(0xc1):snprintf(t, sizeof(t), "xor #%u", op0 & 15); break;
    case15(0xd0):snprintf(t, sizeof(t), "inc r%u", op0 & 15); break;
    case  (0xdf):snprintf(t, sizeof(t), "romb"); break;
    case15(0xe0):snprintf(t, sizeof(t), "dec r%u", op0 & 15); break;
    case  (0xef):snprintf(t, sizeof(t), "getbs"); break;
    case16(0xf0):snprintf(t, sizeof(t), "lm r%u", op0 & 15); break;
  }
  strcat(output, t);
}

#undef case4
#undef case6
#undef case12
#undef case15
#undef case16
#undef op0
#undef op1
#undef op2
