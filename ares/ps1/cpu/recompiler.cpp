auto CPU::Recompiler::pool(u32 address) -> Pool* {
  auto& pool = pools[address >> 8 & 0x1fffff];
  if(!pool) pool = (Pool*)allocator.acquire(sizeof(Pool));
  return pool;
}

auto CPU::Recompiler::block(u32 address) -> Block* {
  if(auto block = pool(address)->blocks[address >> 2 & 0x3f]) return block;
  auto block = emit(address);
  return pool(address)->blocks[address >> 2 & 0x3f] = block;
}

auto CPU::Recompiler::emit(u32 address) -> Block* {
  if(unlikely(allocator.available() < 1_MiB)) {
    print("CPU allocator flush\n");
    allocator.release(bump_allocator::zero_fill);
    reset();
  }

  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = allocator.acquire();
  bind({block->code, allocator.available()});
  push(rbx);
  push(rbp);
  push(r13);
  if constexpr(ABI::Windows) {
    sub(rsp, imm8(0x40));
  }
  mov(rbx, ra0);
  mov(rbp, ra1);

  auto entry = declareLabel();
  jmp8(entry);

  auto epilogue = defineLabel();

  if constexpr(ABI::Windows) {
    add(rsp, imm8(0x40));
  }
  pop(r13);
  pop(rbp);
  pop(rbx);
  ret();

  defineLabel(entry);

  address &= 0x1fff'ffff;
  bool hasBranched = 0;
  while(true) {
    //shortcut: presume CPU is executing out of either CPU RAM or the BIOS area
    u32 instruction = address <= 0x007f'ffff ? cpu.ram.readWord(address) : bios.readWord(address);
    bool branched = emitEXECUTE(instruction);
    call(&CPU::instructionEpilogue);
    address += 4;
    if(hasBranched || (address & 0xfc) == 0) break;  //block boundary
    hasBranched = branched;
    test(al, al);
    jnz(epilogue);
  }
  jmp(epilogue);

  allocator.reserve(size());
//print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}

#define Sa  (instruction >>  6 & 31)
#define Rdn (instruction >> 11 & 31)
#define Rtn (instruction >> 16 & 31)
#define Rsn (instruction >> 21 & 31)
#define Rd  dis8(rbx, Rdn * 4)
#define Rt  dis8(rbx, Rtn * 4)
#define Rs  dis8(rbx, Rsn * 4)
#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

auto CPU::Recompiler::emitEXECUTE(u32 instruction) -> bool {
  switch(instruction >> 26) {

  //SPECIAL
  case 0x00: {
    return emitSPECIAL(instruction);
  }

  //REGIMM
  case 0x01: {
    return emitREGIMM(instruction);
  }

  //J n26
  case 0x02: {
    mov(ra1d, imm32(n26));
    call(&CPU::J);
    return 1;
  }

  //JAL n26
  case 0x03: {
    mov(ra1d, imm32(n26));
    call(&CPU::JAL);
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    mov(ra3d, imm32(i16));
    call(&CPU::BEQ);
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    mov(ra3d, imm32(i16));
    call(&CPU::BNE);
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&CPU::BLEZ);
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&CPU::BGTZ);
    return 1;
  }

  //ADDI Rt,Rs,i16
  case 0x08: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::ADDI);
    return 0;
  }

  //ADDIU Rt,Rs,i16
  case 0x09: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::ADDIU);
    return 0;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SLTI);
    return 0;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SLTIU);
    return 0;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(n16));
    call(&CPU::ANDI);
    return 0;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(n16));
    call(&CPU::ORI);
    return 0;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(n16));
    call(&CPU::XORI);
    return 0;
  }

  //LUI Rt,n16
  case 0x0f: {
    lea(ra1, Rt);
    mov(ra2d, imm32(n16));
    call(&CPU::LUI);
    return 0;
  }

  //SCC
  case 0x10: {
    return emitSCC(instruction);
  }

  //COP1
  case 0x11: {
    call(&CPU::COP1);
    return 1;
  }

  //GTE
  case 0x12: {
    return emitGTE(instruction);
  }

  //COP3
  case 0x13: {
    call(&CPU::COP3);
    return 1;
  }

  //INVALID
  case 0x14 ... 0x1f: {
    call(&CPU::INVALID);
    return 1;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LB);
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LH);
    return 0;
  }

  //LWL Rt,Rs,i16
  case 0x22: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LWL);
    return 0;
  }

  //LW Rt,Rs,i16
  case 0x23: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LW);
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LBU);
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LHU);
    return 0;
  }

  //LWR Rt,Rs,i16
  case 0x26: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LWR);
    return 0;
  }

  //INVALID
  case 0x27: {
    call(&CPU::INVALID);
    return 1;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SB);
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SH);
    return 0;
  }

  //SWL Rt,Rs,i16
  case 0x2a: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SWL);
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SW);
    return 0;
  }

  //INVALID
  case 0x2c ... 0x2d: {
    call(&CPU::INVALID);
    return 1;
  }

  //SWR Rt,Rs,i16
  case 0x2e: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SWR);
    return 0;
  }

  //INVALID
  case 0x2f: {
    call(&CPU::INVALID);
    return 1;
  }

  //LWC0 Rt,Rs,i16
  case 0x30: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LWC0);
    return 0;
  }

  //LWC1 Rt,Rs,i16
  case 0x31: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LWC1);
    return 0;
  }

  //LWC2 Rt,Rs,i16
  case 0x32: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LWC2);
    return 0;
  }

  //LWC3 Rt,Rs,i16
  case 0x33: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::LWC3);
    return 0;
  }

  //INVALID
  case 0x34 ... 0x37: {
    call(&CPU::INVALID);
    return 1;
  }

  //SWC0 Rt,Rs,i16
  case 0x38: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SWC0);
    return 0;
  }

  //SWC1 Rt,Rs,i16
  case 0x39: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SWC1);
    return 0;
  }

  //SWC2 Rt,Rs,i16
  case 0x3a: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SWC2);
    return 0;
  }

  //SWC3 Rt,Rs,i16
  case 0x3b: {
    mov(ra1d, imm32(Rtn));
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&CPU::SWC3);
    return 0;
  }

  //INVALID
  case 0x3c ... 0x3f: {
    call(&CPU::INVALID);
    return 1;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    lea(ra1, Rd);
    lea(ra2, Rt);
    mov(ra3d, imm32(Sa));
    call(&CPU::SLL);
    return 0;
  }

  //INVALID
  case 0x01: {
    call(&CPU::INVALID);
    return 1;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    lea(ra1, Rd);
    lea(ra2, Rt);
    mov(ra3d, imm32(Sa));
    call(&CPU::SRL);
    return 0;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    lea(ra1, Rd);
    lea(ra2, Rt);
    mov(ra3d, imm32(Sa));
    call(&CPU::SRA);
    return 0;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    lea(ra1, Rd);
    lea(ra2, Rt);
    lea(ra3, Rs);
    call(&CPU::SLLV);
    return 0;
  }

  //INVALID
  case 0x05: {
    call(&CPU::INVALID);
    return 1;
  }

  //SRLV Rd,Rt,Rs
  case 0x06: {
    lea(ra1, Rd);
    lea(ra2, Rt);
    lea(ra3, Rs);
    call(&CPU::SRLV);
    return 0;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    lea(ra1, Rd);
    lea(ra2, Rt);
    lea(ra3, Rs);
    call(&CPU::SRAV);
    return 0;
  }

  //JR Rs
  case 0x08: {
    lea(ra1, Rs);
    call(&CPU::JR);
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    call(&CPU::JALR);
    return 1;
  }

  //INVALID
  case 0x0a ... 0x0b: {
    call(&CPU::INVALID);
    return 1;
  }

  //SYSCALL
  case 0x0c: {
    call(&CPU::SYSCALL);
    return 1;
  }

  //BREAK
  case 0x0d: {
    call(&CPU::BREAK);
    return 1;
  }

  //INVALID
  case 0x0e ... 0x0f: {
    call(&CPU::INVALID);
    return 1;
  }

  //MFHI Rd
  case 0x10: {
    lea(ra1, Rd);
    call(&CPU::MFHI);
    return 0;
  }

  //MTHI Rs
  case 0x11: {
    lea(ra1, Rs);
    call(&CPU::MTHI);
    return 0;
  }

  //MFLO Rd
  case 0x12: {
    lea(ra1, Rd);
    call(&CPU::MFLO);
    return 0;
  }

  //MTLO Rs
  case 0x13: {
    lea(ra1, Rs);
    call(&CPU::MTLO);
    return 0;
  }

  //INVALID
  case 0x14 ... 0x17: {
    call(&CPU::INVALID);
    return 1;
  }

  //MULT Rs,Rt
  case 0x18: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    call(&CPU::MULT);
    return 0;
  }

  //MULTU Rs,Rt
  case 0x19: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    call(&CPU::MULTU);
    return 0;
  }

  //DIV Rs,Rt
  case 0x1a: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    call(&CPU::DIV);
    return 0;
  }

  //DIVU Rs,Rt
  case 0x1b: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    call(&CPU::DIVU);
    return 0;
  }

  //ILLEGAL
  case 0x1c ... 0x1f: {
    call(&CPU::INVALID);
    return 1;
  }

  //ADD Rd,Rs,Rt
  case 0x20: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::ADD);
    return 0;
  }

  //ADDU Rd,Rs,Rt
  case 0x21: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::ADDU);
    return 0;
  }

  //SUB Rd,Rs,Rt
  case 0x22: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::SUB);
    return 0;
  }

  //SUBU Rd,Rs,Rt
  case 0x23: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::SUBU);
    return 0;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::AND);
    return 0;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::OR);
    return 0;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::XOR);
    return 0;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::NOR);
    return 0;
  }

  //INVALID
  case 0x28 ... 0x29: {
    call(&CPU::INVALID);
    return 1;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::SLT);
    return 0;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    lea(ra3, Rt);
    call(&CPU::SLTU);
    return 0;
  }

  //INVALID
  case 0x2c ... 0x3f: {
    call(&CPU::INVALID);
    return 1;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitREGIMM(u32 instruction) -> bool {
  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: case 0x02: case 0x04: case 0x06:
  case 0x08: case 0x0a: case 0x0c: case 0x0e:
  case 0x12: case 0x14: case 0x16: case 0x18:
  case 0x1a: case 0x1c: case 0x1e: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&CPU::BLTZ);
    return 1;
  }

  //BGEZ Rs,i16
  case 0x01: case 0x03: case 0x05: case 0x07:
  case 0x09: case 0x0b: case 0x0d: case 0x0f:
  case 0x13: case 0x15: case 0x17: case 0x19:
  case 0x1b: case 0x1d: case 0x1f: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&CPU::BGEZ);
    return 1;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&CPU::BLTZAL);
    return 1;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&CPU::BGEZAL);
    return 1;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitSCC(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC0 Rt,Rd
  case 0x00: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&CPU::MFC0);
    return 0;
  }

  //INVALID
  case 0x01 ... 0x03: {
    call(&CPU::INVALID);
    return 1;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&CPU::MTC0);
    return 0;
  }

  //INVALID
  case 0x05 ... 0x0f: {
    call(&CPU::INVALID);
    return 1;
  }

  }

  switch(instruction & 0x3f) {

  //RFE
  case 0x10: {
    call(&CPU::RFE);
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitGTE(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC2 Rt,Rd
  case 0x00: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&CPU::MFC2);
    return 0;
  }

  //INVALID
  case 0x01: {
    call(&CPU::INVALID);
    return 1;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&CPU::CFC2);
    return 0;
  }

  //INVALID
  case 0x03: {
    call(&CPU::INVALID);
    return 1;
  }

  //MTC2 Rt,Rd
  case 0x04: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&CPU::MTC2);
    return 0;
  }

  //INVALID
  case 0x05: {
    call(&CPU::INVALID);
    return 1;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&CPU::CTC2);
    return 0;
  }

  //INVALID
  case 0x07 ... 0x0f: {
    call(&CPU::INVALID);
    return 1;
  }

  }

  #define Lm instruction >> 10 & 1
  #define Tv instruction >> 13 & 3
  #define Mv instruction >> 15 & 3
  #define Mm instruction >> 17 & 3
  #define Sf instruction >> 19 & 1 ? 12 : 0
  switch(instruction & 0x3f) {

  //RTPS Lm,Sf (mirror)
  case 0x00: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::RTPS);
    return 0;
  }

  //RTPS Lm,Sf
  case 0x01: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::RTPS);
    return 0;
  }

  //NCLIP
  case 0x06: {
    call(&CPU::NCLIP);
    return 0;
  }

  //OP Lm,Sf
  case 0x0c: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::OP);
    return 0;
  }

  //DPCS Lm,Sf
  case 0x10: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::DPCS);
    return 0;
  }

  //INTPL Lm,Sf
  case 0x11: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::INTPL);
    return 0;
  }

  //MVMVA Lm,Tv,Mv,Mm,Sf
  case 0x12: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Tv));
    mov(ra3d, imm32(Mv));
    mov(ra4d, imm32(Mm));
    mov(ra5d, imm32(Sf));
    call(&CPU::MVMVA);
    return 0;
  }

  //NCDS Lm,Sf
  case 0x13: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::NCDS);
    return 0;
  }

  //CDP Lm,Sf
  case 0x14: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::CDP);
    return 0;
  }

  //NCDT Lm,Sf
  case 0x16: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::NCDT);
    return 0;
  }

  //DCPL Lm,Sf (mirror)
  case 0x1a: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::DCPL);
    return 0;
  }

  //NCCS Lm,Sf
  case 0x1b: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::NCCS);
    return 0;
  }

  //CC Lm,Sf
  case 0x1c: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::CC);
    return 0;
  }

  //NCS Lm,Sf
  case 0x1e: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::NCS);
    return 0;
  }

  //NCT Lm,Sf
  case 0x20: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::NCT);
    return 0;
  }

  //SQR Lm,Sf
  case 0x28: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::SQR);
    return 0;
  }

  //DCPL Lm,Sf
  case 0x29: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::DCPL);
    return 0;
  }

  //DPCT Lm,Sf
  case 0x2a: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::DPCT);
    return 0;
  }

  //AVSZ3
  case 0x2d: {
    call(&CPU::AVSZ3);
    return 0;
  }

  //AVSZ4
  case 0x2e: {
    call(&CPU::AVSZ4);
    return 0;
  }

  //RTPT Lm,Sf
  case 0x30: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::RTPT);
    return 0;
  }

  //GPF Lm,Sf
  case 0x3d: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::GPF);
    return 0;
  }

  //GPL Lm,Sf
  case 0x3e: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::GPL);
    return 0;
  }

  //NCCT Lm,Sf
  case 0x3f: {
    mov(ra1d, imm32(Lm));
    mov(ra2d, imm32(Sf));
    call(&CPU::NCCT);
    return 0;
  }

  }
  #undef Lm
  #undef Tv
  #undef Mv
  #undef Mm
  #undef Sf

  return 0;
}

#undef Sa
#undef Rdn
#undef Rtn
#undef Rsn
#undef Rd
#undef Rt
#undef Rs
#undef i16
#undef n16
#undef n26

template<typename V, typename... P>
auto CPU::Recompiler::call(V (CPU::*function)(P...)) -> void {
  static_assert(sizeof...(P) <= 5);
  if constexpr(ABI::Windows) {
    if constexpr(sizeof...(P) >= 5) mov(dis8(rsp, 0x28), ra5);
    if constexpr(sizeof...(P) >= 4) mov(dis8(rsp, 0x20), ra4);
  }
  mov(ra0, rbp);
  call(imm64{function}, rax);
}
