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
    case  (0x00): sprintf(t, "stop"); break;
    case  (0x01): sprintf(t, "nop"); break;
    case  (0x02): sprintf(t, "cache"); break;
    case  (0x03): sprintf(t, "lsr"); break;
    case  (0x04): sprintf(t, "rol"); break;
    case  (0x05): sprintf(t, "bra %+d", (s8)op1); break;
    case  (0x06): sprintf(t, "blt %+d", (s8)op1); break;
    case  (0x07): sprintf(t, "bge %+d", (s8)op1); break;
    case  (0x08): sprintf(t, "bne %+d", (s8)op1); break;
    case  (0x09): sprintf(t, "beq %+d", (s8)op1); break;
    case  (0x0a): sprintf(t, "bpl %+d", (s8)op1); break;
    case  (0x0b): sprintf(t, "bmi %+d", (s8)op1); break;
    case  (0x0c): sprintf(t, "bcc %+d", (s8)op1); break;
    case  (0x0d): sprintf(t, "bcs %+d", (s8)op1); break;
    case  (0x0e): sprintf(t, "bvc %+d", (s8)op1); break;
    case  (0x0f): sprintf(t, "bvs %+d", (s8)op1); break;
    case16(0x10): sprintf(t, "to r%u", op0 & 15); break;
    case16(0x20): sprintf(t, "with r%u", op0 & 15); break;
    case12(0x30): sprintf(t, "stw (r%u)", op0 & 15); break;
    case  (0x3c): sprintf(t, "loop"); break;
    case  (0x3d): sprintf(t, "alt1"); break;
    case  (0x3e): sprintf(t, "alt2"); break;
    case  (0x3f): sprintf(t, "alt3"); break;
    case12(0x40): sprintf(t, "ldw (r%u)", op0 & 15); break;
    case  (0x4c): sprintf(t, "plot"); break;
    case  (0x4d): sprintf(t, "swap"); break;
    case  (0x4e): sprintf(t, "color"); break;
    case  (0x4f): sprintf(t, "not"); break;
    case16(0x50): sprintf(t, "add r%u", op0 & 15); break;
    case16(0x60): sprintf(t, "sub r%u", op0 & 15); break;
    case  (0x70): sprintf(t, "merge"); break;
    case15(0x71): sprintf(t, "and r%u", op0 & 15); break;
    case16(0x80): sprintf(t, "mult r%u", op0 & 15); break;
    case  (0x90): sprintf(t, "sbk"); break;
    case4 (0x91): sprintf(t, "link #%u", op0 & 15); break;
    case  (0x95): sprintf(t, "sex"); break;
    case  (0x96): sprintf(t, "asr"); break;
    case  (0x97): sprintf(t, "ror"); break;
    case6 (0x98): sprintf(t, "jmp r%u", op0 & 15); break;
    case  (0x9e): sprintf(t, "lob"); break;
    case  (0x9f): sprintf(t, "fmult"); break;
    case16(0xa0): sprintf(t, "ibt r%u,#$%.2x", op0 & 15, op1 & 255); break;
    case16(0xb0): sprintf(t, "from r%u", op0 & 15); break;
    case  (0xc0): sprintf(t, "hib"); break;
    case15(0xc1): sprintf(t, "or r%u", op0 & 15); break;
    case15(0xd0): sprintf(t, "inc r%u", op0 & 15); break;
    case  (0xdf): sprintf(t, "getc"); break;
    case15(0xe0): sprintf(t, "dec r%u", op0 & 15); break;
    case  (0xef): sprintf(t, "getb"); break;
    case16(0xf0): sprintf(t, "iwt r%u,#$%.2x%.2x", op0 & 15, op2 & 255, op1 & 255); break;
  }
  strcat(output, t);
}

auto GSU::disassembleALT1(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00): sprintf(t, "stop"); break;
    case  (0x01): sprintf(t, "nop"); break;
    case  (0x02): sprintf(t, "cache"); break;
    case  (0x03): sprintf(t, "lsr"); break;
    case  (0x04): sprintf(t, "rol"); break;
    case  (0x05): sprintf(t, "bra %+d", (s8)op1); break;
    case  (0x06): sprintf(t, "blt %+d", (s8)op1); break;
    case  (0x07): sprintf(t, "bge %+d", (s8)op1); break;
    case  (0x08): sprintf(t, "bne %+d", (s8)op1); break;
    case  (0x09): sprintf(t, "beq %+d", (s8)op1); break;
    case  (0x0a): sprintf(t, "bpl %+d", (s8)op1); break;
    case  (0x0b): sprintf(t, "bmi %+d", (s8)op1); break;
    case  (0x0c): sprintf(t, "bcc %+d", (s8)op1); break;
    case  (0x0d): sprintf(t, "bcs %+d", (s8)op1); break;
    case  (0x0e): sprintf(t, "bvc %+d", (s8)op1); break;
    case  (0x0f): sprintf(t, "bvs %+d", (s8)op1); break;
    case16(0x10): sprintf(t, "to r%u", op0 & 15); break;
    case16(0x20): sprintf(t, "with r%u", op0 & 15); break;
    case12(0x30): sprintf(t, "stb (r%u)", op0 & 15); break;
    case  (0x3c): sprintf(t, "loop"); break;
    case  (0x3d): sprintf(t, "alt1"); break;
    case  (0x3e): sprintf(t, "alt2"); break;
    case  (0x3f): sprintf(t, "alt3"); break;
    case12(0x40): sprintf(t, "ldb (r%u)", op0 & 15); break;
    case  (0x4c): sprintf(t, "rpix"); break;
    case  (0x4d): sprintf(t, "swap"); break;
    case  (0x4e): sprintf(t, "cmode"); break;
    case  (0x4f): sprintf(t, "not"); break;
    case16(0x50): sprintf(t, "adc r%u", op0 & 15); break;
    case16(0x60): sprintf(t, "sbc r%u", op0 & 15); break;
    case  (0x70): sprintf(t, "merge"); break;
    case15(0x71): sprintf(t, "bic r%u", op0 & 15); break;
    case16(0x80): sprintf(t, "umult r%u", op0 & 15); break;
    case  (0x90): sprintf(t, "sbk"); break;
    case4 (0x91): sprintf(t, "link #%u", op0 & 15); break;
    case  (0x95): sprintf(t, "sex"); break;
    case  (0x96): sprintf(t, "div2"); break;
    case  (0x97): sprintf(t, "ror"); break;
    case6 (0x98): sprintf(t, "ljmp r%u", op0 & 15); break;
    case  (0x9e): sprintf(t, "lob"); break;
    case  (0x9f): sprintf(t, "lmult"); break;
    case16(0xa0): sprintf(t, "lms r%u,(#$%.4x)", op0 & 15, op1 << 1); break;
    case16(0xb0): sprintf(t, "from r%u", op0 & 15); break;
    case  (0xc0): sprintf(t, "hib"); break;
    case15(0xc1): sprintf(t, "xor r%u", op0 & 15); break;
    case15(0xd0): sprintf(t, "inc r%u", op0 & 15); break;
    case  (0xdf): sprintf(t, "getc"); break;
    case15(0xe0): sprintf(t, "dec r%u", op0 & 15); break;
    case  (0xef): sprintf(t, "getbh"); break;
    case16(0xf0): sprintf(t, "lm r%u", op0 & 15); break;
  }
  strcat(output, t);
}

auto GSU::disassembleALT2(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00): sprintf(t, "stop"); break;
    case  (0x01): sprintf(t, "nop"); break;
    case  (0x02): sprintf(t, "cache"); break;
    case  (0x03): sprintf(t, "lsr"); break;
    case  (0x04): sprintf(t, "rol"); break;
    case  (0x05): sprintf(t, "bra %+d", (s8)op1); break;
    case  (0x06): sprintf(t, "blt %+d", (s8)op1); break;
    case  (0x07): sprintf(t, "bge %+d", (s8)op1); break;
    case  (0x08): sprintf(t, "bne %+d", (s8)op1); break;
    case  (0x09): sprintf(t, "beq %+d", (s8)op1); break;
    case  (0x0a): sprintf(t, "bpl %+d", (s8)op1); break;
    case  (0x0b): sprintf(t, "bmi %+d", (s8)op1); break;
    case  (0x0c): sprintf(t, "bcc %+d", (s8)op1); break;
    case  (0x0d): sprintf(t, "bcs %+d", (s8)op1); break;
    case  (0x0e): sprintf(t, "bvc %+d", (s8)op1); break;
    case  (0x0f): sprintf(t, "bvs %+d", (s8)op1); break;
    case16(0x10): sprintf(t, "to r%u", op0 & 15); break;
    case16(0x20): sprintf(t, "with r%u", op0 & 15); break;
    case12(0x30): sprintf(t, "stw (r%u)", op0 & 15); break;
    case  (0x3c): sprintf(t, "loop"); break;
    case  (0x3d): sprintf(t, "alt1"); break;
    case  (0x3e): sprintf(t, "alt2"); break;
    case  (0x3f): sprintf(t, "alt3"); break;
    case12(0x40): sprintf(t, "ldw (r%u)", op0 & 15); break;
    case  (0x4c): sprintf(t, "plot"); break;
    case  (0x4d): sprintf(t, "swap"); break;
    case  (0x4e): sprintf(t, "color"); break;
    case  (0x4f): sprintf(t, "not"); break;
    case16(0x50): sprintf(t, "add #%u", op0 & 15); break;
    case16(0x60): sprintf(t, "sub #%u", op0 & 15); break;
    case  (0x70): sprintf(t, "merge"); break;
    case15(0x71): sprintf(t, "and #%u", op0 & 15); break;
    case16(0x80): sprintf(t, "mult #%u", op0 & 15); break;
    case  (0x90): sprintf(t, "sbk"); break;
    case4 (0x91): sprintf(t, "link #%u", op0 & 15); break;
    case  (0x95): sprintf(t, "sex"); break;
    case  (0x96): sprintf(t, "asr"); break;
    case  (0x97): sprintf(t, "ror"); break;
    case6 (0x98): sprintf(t, "jmp r%u", op0 & 15); break;
    case  (0x9e): sprintf(t, "lob"); break;
    case  (0x9f): sprintf(t, "fmult"); break;
    case16(0xa0): sprintf(t, "sms r%u,(#$%.4x)", op0 & 15, op1 << 1); break;
    case16(0xb0): sprintf(t, "from r%u", op0 & 15); break;
    case  (0xc0): sprintf(t, "hib"); break;
    case15(0xc1): sprintf(t, "or #%u", op0 & 15); break;
    case15(0xd0): sprintf(t, "inc r%u", op0 & 15); break;
    case  (0xdf): sprintf(t, "ramb"); break;
    case15(0xe0): sprintf(t, "dec r%u", op0 & 15); break;
    case  (0xef): sprintf(t, "getbl"); break;
    case16(0xf0): sprintf(t, "sm r%u", op0 & 15); break;
  }
  strcat(output, t);
}

auto GSU::disassembleALT3(char* output) -> void {
  char t[256] = "";
  switch(op0) {
    case  (0x00): sprintf(t, "stop"); break;
    case  (0x01): sprintf(t, "nop"); break;
    case  (0x02): sprintf(t, "cache"); break;
    case  (0x03): sprintf(t, "lsr"); break;
    case  (0x04): sprintf(t, "rol"); break;
    case  (0x05): sprintf(t, "bra %+d", (s8)op1); break;
    case  (0x06): sprintf(t, "blt %+d", (s8)op1); break;
    case  (0x07): sprintf(t, "bge %+d", (s8)op1); break;
    case  (0x08): sprintf(t, "bne %+d", (s8)op1); break;
    case  (0x09): sprintf(t, "beq %+d", (s8)op1); break;
    case  (0x0a): sprintf(t, "bpl %+d", (s8)op1); break;
    case  (0x0b): sprintf(t, "bmi %+d", (s8)op1); break;
    case  (0x0c): sprintf(t, "bcc %+d", (s8)op1); break;
    case  (0x0d): sprintf(t, "bcs %+d", (s8)op1); break;
    case  (0x0e): sprintf(t, "bvc %+d", (s8)op1); break;
    case  (0x0f): sprintf(t, "bvs %+d", (s8)op1); break;
    case16(0x10): sprintf(t, "to r%u", op0 & 15); break;
    case16(0x20): sprintf(t, "with r%u", op0 & 15); break;
    case12(0x30): sprintf(t, "stb (r%u)", op0 & 15); break;
    case  (0x3c): sprintf(t, "loop"); break;
    case  (0x3d): sprintf(t, "alt1"); break;
    case  (0x3e): sprintf(t, "alt2"); break;
    case  (0x3f): sprintf(t, "alt3"); break;
    case12(0x40): sprintf(t, "ldb (r%u)", op0 & 15); break;
    case  (0x4c): sprintf(t, "rpix"); break;
    case  (0x4d): sprintf(t, "swap"); break;
    case  (0x4e): sprintf(t, "cmode"); break;
    case  (0x4f): sprintf(t, "not"); break;
    case16(0x50): sprintf(t, "adc #%u", op0 & 15); break;
    case16(0x60): sprintf(t, "cmp r%u", op0 & 15); break;
    case  (0x70): sprintf(t, "merge"); break;
    case15(0x71): sprintf(t, "bic #%u", op0 & 15); break;
    case16(0x80): sprintf(t, "umult #%u", op0 & 15); break;
    case  (0x90): sprintf(t, "sbk"); break;
    case4 (0x91): sprintf(t, "link #%u", op0 & 15); break;
    case  (0x95): sprintf(t, "sex"); break;
    case  (0x96): sprintf(t, "div2"); break;
    case  (0x97): sprintf(t, "ror"); break;
    case6 (0x98): sprintf(t, "ljmp r%u", op0 & 15); break;
    case  (0x9e): sprintf(t, "lob"); break;
    case  (0x9f): sprintf(t, "lmult"); break;
    case16(0xa0): sprintf(t, "lms r%u", op0 & 15); break;
    case16(0xb0): sprintf(t, "from r%u", op0 & 15); break;
    case  (0xc0): sprintf(t, "hib"); break;
    case15(0xc1): sprintf(t, "xor #%u", op0 & 15); break;
    case15(0xd0): sprintf(t, "inc r%u", op0 & 15); break;
    case  (0xdf): sprintf(t, "romb"); break;
    case15(0xe0): sprintf(t, "dec r%u", op0 & 15); break;
    case  (0xef): sprintf(t, "getbs"); break;
    case16(0xf0): sprintf(t, "lm r%u", op0 & 15); break;
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
