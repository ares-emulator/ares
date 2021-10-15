auto RSP::Recompiler::pool() -> Pool* {
  if(context) return context;

  context = (Pool*)allocator.acquire();
  u32 hashcode = 0;
  for(u32 offset : range(4096)) {
    hashcode = (hashcode << 5) + hashcode + self.imem.read<Byte>(offset);
  }
  context->hashcode = hashcode;

  if(auto result = pools.find(*context)) {
    context->hashcode = 0;  //leave the memory zeroed out
    return context = &result();
  }

  allocator.reserve(sizeof(Pool));
  if(auto result = pools.insert(*context)) {
    return context = &result();
  }

  throw;  //should never occur
}

auto RSP::Recompiler::block(u32 address) -> Block* {
  if(auto block = pool()->blocks[address >> 2 & 0x3ff]) return block;
  auto block = emit(address);
  return pool()->blocks[address >> 2 & 0x3ff] = block;
}

auto RSP::Recompiler::emit(u32 address) -> Block* {
  if(unlikely(allocator.available() < 1_MiB)) {
    print("RSP allocator flush\n");
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
  mov(r13, ra2);

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

  bool hasBranched = 0;
  while(true) {
    u32 instruction = self.imem.read<Word>(address);
    bool branched = emitEXECUTE(instruction);
    call(&RSP::instructionEpilogue);
    address += 4;
    if(hasBranched || (address & 0xffc) == 0) break;  //IMEM boundary
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
#define Vdn (instruction >>  6 & 31)
#define Vsn (instruction >> 11 & 31)
#define Vtn (instruction >> 16 & 31)
#define Rd  dis8(rbx, Rdn * 4)
#define Rt  dis8(rbx, Rtn * 4)
#define Rs  dis8(rbx, Rsn * 4)
#define Vd  dis32(r13, Vdn * 16)
#define Vs  dis32(r13, Vsn * 16)
#define Vt  dis32(r13, Vtn * 16)
#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

auto RSP::Recompiler::emitEXECUTE(u32 instruction) -> bool {
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
    call(&RSP::J);
    return 1;
  }

  //JAL n26
  case 0x03: {
    mov(ra1d, imm32(n26));
    call(&RSP::JAL);
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    mov(ra3d, imm32(i16));
    call(&RSP::BEQ);
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    lea(ra1, Rs);
    lea(ra2, Rt);
    mov(ra3d, imm32(i16));
    call(&RSP::BNE);
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&RSP::BLEZ);
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&RSP::BGTZ);
    return 1;
  }

  //ADDIU Rt,Rs,i16
  case 0x08 ... 0x09: {
    mov(eax, Rs);
    add(eax, imm32(i16));
    mov(Rt, eax);
    return 0;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    mov(eax, Rs);
    cmp(eax, imm32(i16));
    setl(al);
    movzx(eax, al);
    mov(Rt, eax);
    return 0;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    mov(eax, Rs);
    cmp(eax, imm32(i16));
    setb(al);
    movzx(eax, al);
    mov(Rt, eax);
    return 0;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    mov(eax, Rs);
    and(eax, imm32(n16));
    mov(Rt, eax);
    return 0;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    mov(eax, Rs);
    or(eax, imm32(n16));
    mov(Rt, eax);
    return 0;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    mov(eax, Rs);
    xor(eax, imm32(n16));
    mov(Rt, eax);
    return 0;
  }

  //LUI Rt,n16
  case 0x0f: {
    mov(eax, imm32(n16 << 16));
    mov(Rt, eax);
    return 0;
  }

  //SCC
  case 0x10: {
    return emitSCC(instruction);
  }

  //INVALID
  case 0x11: {
    return 0;
  }

  //VPU
  case 0x12: {
    return emitVU(instruction);
  }

  //INVALID
  case 0x13 ... 0x1f: {
    return 0;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::LB);
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::LH);
    return 0;
  }

  //INVALID
  case 0x22: {
    return 0;
  }

  //LW Rt,Rs,i16
  case 0x23: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::LW);
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::LBU);
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::LHU);
    return 0;
  }

  //INVALID
  case 0x26 ... 0x27: {
    return 0;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::SB);
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::SH);
    return 0;
  }

  //INVALID
  case 0x2a: {
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    lea(ra1, Rt);
    lea(ra2, Rs);
    mov(ra3d, imm32(i16));
    call(&RSP::SW);
    return 0;
  }

  //INVALID
  case 0x2c ... 0x31: {
    return 0;
  }

  //LWC2
  case 0x32: {
    return emitLWC2(instruction);
  }

  //INVALID
  case 0x33 ... 0x39: {
    return 0;
  }

  //SWC2
  case 0x3a: {
    return emitSWC2(instruction);
  }

  //INVALID
  case 0x3b ... 0x3f: {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    mov(eax, Rt);
    shl(eax, imm8(Sa));
    mov(Rd, eax);
    return 0;
  }

  //INVALID
  case 0x01: {
    return 0;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    mov(eax, Rt);
    shr(eax, imm8(Sa));
    mov(Rd, eax);
    return 0;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    mov(eax, Rt);
    sar(eax, imm8(Sa));
    mov(Rd, eax);
    return 0;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    mov(eax, Rt);
    mov(ecx, Rs);
    and(cl, imm8(31));
    shl(eax, cl);
    mov(Rd, eax);
    return 0;
  }

  //INVALID
  case 0x05: {
    return 0;
  }

  //SRLV Rd,Rt,Rs
  case 0x06: {
    mov(eax, Rt);
    mov(ecx, Rs);
    and(cl, imm8(31));
    shr(eax, cl);
    mov(Rd, eax);
    return 0;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    mov(eax, Rt);
    mov(ecx, Rs);
    and(cl, imm8(31));
    sar(eax, cl);
    mov(Rd, eax);
    return 0;
  }

  //JR Rs
  case 0x08: {
    lea(ra1, Rs);
    call(&RSP::JR);
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    lea(ra1, Rd);
    lea(ra2, Rs);
    call(&RSP::JALR);
    return 1;
  }

  //INVALID
  case 0x0a ... 0x0c: {
    return 0;
  }

  //BREAK
  case 0x0d: {
    call(&RSP::BREAK);
    return 1;
  }

  //INVALID
  case 0x0e ... 0x1f: {
    return 0;
  }

  //ADDU Rd,Rs,Rt
  case 0x20 ... 0x21: {
    mov(eax, Rs);
    add(eax, Rt);
    mov(Rd, eax);
    return 0;
  }

  //SUBU Rd,Rs,Rt
  case 0x22 ... 0x23: {
    mov(eax, Rs);
    sub(eax, Rt);
    mov(Rd, eax);
    return 0;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    mov(eax, Rs);
    and(eax, Rt);
    mov(Rd, eax);
    return 0;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    mov(eax, Rs);
    or(eax, Rt);
    mov(Rd, eax);
    return 0;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    mov(eax, Rs);
    xor(eax, Rt);
    mov(Rd, eax);
    return 0;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    mov(eax, Rs);
    or(eax, Rt);
    not(eax);
    mov(Rd, eax);
    return 0;
  }

  //INVALID
  case 0x28 ... 0x29: {
    return 0;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    mov(eax, Rs);
    cmp(eax, Rt);
    setl(al);
    movzx(eax, al);
    mov(Rd, eax);
    return 0;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    mov(eax, Rs);
    cmp(eax, Rt);
    setb(al);
    movzx(eax, al);
    mov(Rd, eax);
    return 0;
  }

  //INVALID
  case 0x2c ... 0x3f: {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitREGIMM(u32 instruction) -> bool {
  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&RSP::BLTZ);
    return 1;
  }

  //BGEZ Rs,i16
  case 0x01: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&RSP::BGEZ);
    return 1;
  }

  //INVALID
  case 0x02 ... 0x0f: {
    return 0;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&RSP::BLTZAL);
    return 1;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    lea(ra1, Rs);
    mov(ra2d, imm32(i16));
    call(&RSP::BGEZAL);
    return 1;
  }

  //INVALID
  case 0x12 ... 0x1f: {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitSCC(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC0 Rt,Rd
  case 0x00: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&RSP::MFC0);
    return 0;
  }

  //INVALID
  case 0x01 ... 0x03: {
    return 0;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&RSP::MTC0);
    return 0;
  }

  //INVALID
  case 0x05 ... 0x1f: {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitVU(u32 instruction) -> bool {
  #define E (instruction >> 7 & 15)
  switch(instruction >> 21 & 0x1f) {

  //MFC2 Rt,Vs(e)
  case 0x00: {
    lea(ra1, Rt);
    lea(ra2, Vs);
    mov(ra3d, imm32(E));
    call(&RSP::MFC2);
    return 0;
  }

  //INVALID
  case 0x01: {
    return 0;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&RSP::CFC2);
    return 0;
  }

  //INVALID
  case 0x03: {
    return 0;
  }

  //MTC2 Rt,Vs(e)
  case 0x04: {
    lea(ra1, Rt);
    lea(ra2, Vs);
    mov(ra3d, imm32(E));
    call(&RSP::MTC2);
    return 0;
  }

  //INVALID
  case 0x05: {
    return 0;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    lea(ra1, Rt);
    mov(ra2d, imm32(Rdn));
    call(&RSP::CTC2);
    return 0;
  }

  //INVALID
  case 0x07 ... 0x0f: {
    return 0;
  }

  }
  #undef E

  #define E  (instruction >> 21 & 15)
  #define DE (instruction >> 11 &  7)
  switch(instruction & 0x3f) {

  //VMULF Vd,Vs,Vt(e)
  case 0x00: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMULF<0>);
    return 0;
  }

  //VMULU Vd,Vs,Vt(e)
  case 0x01: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMULF<1>);
    return 0;
  }

  //VRNDP Vd,Vs,Vt(e)
  case 0x02: {
    lea(ra1, Vd);
    mov(ra2d, imm32(Vsn));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRND<1>);
    return 0;
  }

  //VMULQ Vd,Vs,Vt(e)
  case 0x03: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMULQ);
    return 0;
  }

  //VMUDL Vd,Vs,Vt(e)
  case 0x04: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMUDL);
    return 0;
  }

  //VMUDM Vd,Vs,Vt(e)
  case 0x05: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMUDM);
    return 0;
  }

  //VMUDN Vd,Vs,Vt(e)
  case 0x06: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMUDN);
    return 0;
  }

  //VMUDH Vd,Vs,Vt(e)
  case 0x07: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMUDH);
    return 0;
  }

  //VMACF Vd,Vs,Vt(e)
  case 0x08: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMACF<0>);
    return 0;
  }

  //VMACU Vd,Vs,Vt(e)
  case 0x09: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMACF<1>);
    return 0;
  }

  //VRNDN Vd,Vs,Vt(e)
  case 0x0a: {
    lea(ra1, Vd);
    mov(ra2d, imm32(Vsn));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRND<0>);
    return 0;
  }

  //VMACQ Vd
  case 0x0b: {
    lea(ra1, Vd);
    call(&RSP::VMACQ);
    return 0;
  }

  //VMADL Vd,Vs,Vt(e)
  case 0x0c: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMADL);
    return 0;
  }

  //VMADM Vd,Vs,Vt(e)
  case 0x0d: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMADM);
    return 0;
  }

  //VMADN Vd,Vs,Vt(e)
  case 0x0e: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMADN);
    return 0;
  }

  //VMADH Vd,Vs,Vt(e)
  case 0x0f: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMADH);
    return 0;
  }

  //VADD Vd,Vs,Vt(e)
  case 0x10: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VADD);
    return 0;
  }

  //VSUB Vd,Vs,Vt(e)
  case 0x11: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VSUB);
    return 0;
  }

  //INVALID
  case 0x12: {
    return 0;
  }

  //VABS Vd,Vs,Vt(e)
  case 0x13: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VABS);
    return 0;
  }

  //VADDC Vd,Vs,Vt(e)
  case 0x14: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VADDC);
    return 0;
  }

  //VSUBC Vd,Vs,Vt(e)
  case 0x15: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VSUBC);
    return 0;
  }

  //INVALID
  case 0x16 ... 0x1c: {
    return 0;
  }

  //VSAR Vd,Vs,E
  case 0x1d: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    mov(ra3d, imm32(E));
    call(&RSP::VSAR);
    return 0;
  }

  //INVALID
  case 0x1e ... 0x1f: {
    return 0;
  }

  //VLT Vd,Vs,Vt(e)
  case 0x20: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VLT);
    return 0;
  }

  //VEQ Vd,Vs,Vt(e)
  case 0x21: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VEQ);
    return 0;
  }

  //VNE Vd,Vs,Vt(e)
  case 0x22: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VNE);
    return 0;
  }

  //VGE Vd,Vs,Vt(e)
  case 0x23: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VGE);
    return 0;
  }

  //VCL Vd,Vs,Vt(e)
  case 0x24: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VCL);
    return 0;
  }

  //VCH Vd,Vs,Vt(e)
  case 0x25: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VCH);
    return 0;
  }

  //VCR Vd,Vs,Vt(e)
  case 0x26: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VCR);
    return 0;
  }

  //VMRG Vd,Vs,Vt(e)
  case 0x27: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMRG);
    return 0;
  }

  //VAND Vd,Vs,Vt(e)
  case 0x28: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VAND);
    return 0;
  }

  //VNAND Vd,Vs,Vt(e)
  case 0x29: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VNAND);
    return 0;
  }

  //VOR Vd,Vs,Vt(e)
  case 0x2a: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VOR);
    return 0;
  }

  //VNOR Vd,Vs,Vt(e)
  case 0x2b: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VNOR);
    return 0;
  }

  //VXOR Vd,Vs,Vt(e)
  case 0x2c: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VXOR);
    return 0;
  }

  //VNXOR Vd,Vs,Vt(e)
  case 0x2d: {
    lea(ra1, Vd);
    lea(ra2, Vs);
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VNXOR);
    return 0;
  }

  //INVALID
  case 0x2e ... 0x2f: {
    return 0;
  }

  //VCRP Vd(de),Vt(e)
  case 0x30: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRCP<0>);
    return 0;
  }

  //VRCPL Vd(de),Vt(e)
  case 0x31: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRCP<1>);
    return 0;
  }

  //VRCPH Vd(de),Vt(e)
  case 0x32: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRCPH);
    return 0;
  }

  //VMOV Vd(de),Vt(e)
  case 0x33: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VMOV);
    return 0;
  }

  //VRSQ Vd(de),Vt(e)
  case 0x34: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRSQ<0>);
    return 0;
  }

  //VRSQL Vd(de),Vt(e)
  case 0x35: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRSQ<1>);
    return 0;
  }

  //VRSQH Vd(de),Vt(e)
  case 0x36: {
    lea(ra1, Vd);
    mov(ra2d, imm32(DE));
    lea(ra3, Vt);
    mov(ra4d, imm32(E));
    call(&RSP::VRSQH);
    return 0;
  }

  //VNOP
  case 0x37: {
    call(&RSP::VNOP);
  }

  //INVALID
  case 0x38 ... 0x3f: {
    return 0;
  }

  }
  #undef E
  #undef DE

  return 0;
}

auto RSP::Recompiler::emitLWC2(u32 instruction) -> bool {
  #define E  (instruction >> 7 & 15)
  #define i7 (s8(instruction << 1) >> 1)
  switch(instruction >> 11 & 0x1f) {

  //LBV Vt(e),Rs,i7
  case 0x00: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LBV);
    return 0;
  }

  //LSV Vt(e),Rs,i7
  case 0x01: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LSV);
    return 0;
  }

  //LLV Vt(e),Rs,i7
  case 0x02: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LLV);
    return 0;
  }

  //LDV Vt(e),Rs,i7
  case 0x03: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LDV);
    return 0;
  }

  //LQV Vt(e),Rs,i7
  case 0x04: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LQV);
    return 0;
  }

  //LRV Vt(e),Rs,i7
  case 0x05: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LRV);
    return 0;
  }

  //LPV Vt(e),Rs,i7
  case 0x06: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LPV);
    return 0;
  }

  //LUV Vt(e),Rs,i7
  case 0x07: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LUV);
    return 0;
  }

  //LHV Vt(e),Rs,i7
  case 0x08: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LHV);
    return 0;
  }

  //LFV Vt(e),Rs,i7
  case 0x09: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LFV);
    return 0;
  }

  //LWV (not present on N64 RSP)
  case 0x0a: {
    return 0;
  }

  //LTV Vt(e),Rs,i7
  case 0x0b: {
    mov(ra1d, imm32(Vtn));
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::LTV);
    return 0;
  }

  //INVALID
  case 0x0c ... 0x1f: {
    return 0;
  }

  }
  #undef E
  #undef i7

  return 0;
}

auto RSP::Recompiler::emitSWC2(u32 instruction) -> bool {
  #define E  (instruction >> 7 & 15)
  #define i7 (s8(instruction << 1) >> 1)
  switch(instruction >> 11 & 0x1f) {

  //SBV Vt(e),Rs,i7
  case 0x00: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SBV);
    return 0;
  }

  //SSV Vt(e),Rs,i7
  case 0x01: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SSV);
    return 0;
  }

  //SLV Vt(e),Rs,i7
  case 0x02: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SLV);
    return 0;
  }

  //SDV Vt(e),Rs,i7
  case 0x03: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SDV);
    return 0;
  }

  //SQV Vt(e),Rs,i7
  case 0x04: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SQV);
    return 0;
  }

  //SRV Vt(e),Rs,i7
  case 0x05: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SRV);
    return 0;
  }

  //SPV Vt(e),Rs,i7
  case 0x06: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SPV);
    return 0;
  }

  //SUV Vt(e),Rs,i7
  case 0x07: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SUV);
    return 0;
  }

  //SHV Vt(e),Rs,i7
  case 0x08: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SHV);
    return 0;
  }

  //SFV Vt(e),Rs,i7
  case 0x09: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SFV);
    return 0;
  }

  //SWV Vt(e),Rs,i7
  case 0x0a: {
    lea(ra1, Vt);
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::SWV);
    return 0;
  }

  //STV Vt(e),Rs,i7
  case 0x0b: {
    mov(ra1d, imm32(Vtn));
    mov(ra2d, imm32(E));
    lea(ra3, Rs);
    mov(ra4d, imm32(i7));
    call(&RSP::STV);
    return 0;
  }

  //INVALID
  case 0x0c ... 0x1f: {
    return 0;
  }

  }
  #undef E
  #undef i7

  return 0;
}

#undef Sa
#undef Rdn
#undef Rtn
#undef Rsn
#undef Vdn
#undef Vsn
#undef Vtn
#undef Rd
#undef Rt
#undef Rs
#undef Vd
#undef Vs
#undef Vt
#undef i16
#undef n16
#undef n26

template<typename V, typename... P>
auto RSP::Recompiler::call(V (RSP::*function)(P...)) -> void {
  static_assert(sizeof...(P) <= 5);
  if constexpr(ABI::Windows) {
    if constexpr(sizeof...(P) >= 5) mov(dis8(rsp, 0x28), ra5);
    if constexpr(sizeof...(P) >= 4) mov(dis8(rsp, 0x20), ra4);
  }
  mov(ra0, rbp);
  call(imm64{function}, rax);
}
