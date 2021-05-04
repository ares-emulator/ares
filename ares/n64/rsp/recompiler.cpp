auto RSP::Recompiler::pool() -> Pool* {
  if(context) return context;

  context = (Pool*)allocator.acquire();
  u32 hashcode = 0;
  for(u32 offset : range(4096)) {
    hashcode = (hashcode << 5) + hashcode + self.imem.readByte(offset);
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
  mov(rbx, imm64(&self.ipu.r[0]));
  mov(rbp, imm64(&self));
  mov(r13, imm64(&self.vpu.r[0]));

  bool hasBranched = 0;
  while(true) {
    u32 instruction = self.imem.readWord(address);
    bool branched = emitEXECUTE(instruction);
    mov(rax, mem64(&self.clock));
    add(rax, imm8(2));
    mov(mem64(&self.clock), rax);
    call(&RSP::instructionEpilogue);
    address += 4;
    if(hasBranched || (address & 0xffc) == 0) break;  //IMEM boundary
    hasBranched = branched;
    test(rax, rax);
    jz(imm8(5));
    pop(r13);
    pop(rbp);
    pop(rbx);
    ret();
  }
  pop(r13);
  pop(rbp);
  pop(rbx);
  ret();

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
    mov(esi, imm32(n26));
    call(&RSP::instructionJ);
    return 1;
  }

  //JAL n26
  case 0x03: {
    mov(esi, imm32(n26));
    call(&RSP::instructionJAL);
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    lea(rsi, dis8(rbx, Rsn*4));
    lea(rdx, dis8(rbx, Rtn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionBEQ);
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    lea(rsi, dis8(rbx, Rsn*4));
    lea(rdx, dis8(rbx, Rtn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionBNE);
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    lea(rsi, dis8(rbx, Rsn*4));
    mov(edx, imm32(i16));
    call(&RSP::instructionBLEZ);
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    lea(rsi, dis8(rbx, Rsn*4));
    mov(edx, imm32(i16));
    call(&RSP::instructionBGTZ);
    return 1;
  }

  //ADDIU Rt,Rs,i16
  case 0x08 ... 0x09: {
    mov(esi, Rs);
    add(esi, imm32(i16));
    mov(Rt, esi);
    return 0;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    mov(esi, Rs);
    cmp(esi, imm32(i16));
    setl(al);
    movzx(eax, al);
    mov(Rt, eax);
    return 0;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    mov(esi, Rs);
    cmp(esi, imm32(i16));
    setb(al);
    movzx(eax, al);
    mov(Rt, eax);
    return 0;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    mov(esi, Rs);
    and(esi, imm32(n16));
    mov(Rt, esi);
    return 0;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    mov(esi, Rs);
    or(esi, imm32(n16));
    mov(Rt, esi);
    return 0;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    mov(esi, Rs);
    xor(esi, imm32(n16));
    mov(Rt, esi);
    return 0;
  }

  //LUI Rt,n16
  case 0x0f: {
    mov(esi, imm32(n16));
    mov(Rt, esi);
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
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionLB);
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionLH);
    return 0;
  }

  //INVALID
  case 0x22: {
    return 0;
  }

  //LW Rt,Rs,i16
  case 0x23: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionLW);
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionLBU);
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionLHU);
    return 0;
  }

  //INVALID
  case 0x26 ... 0x27: {
    return 0;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionSB);
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionSH);
    return 0;
  }

  //INVALID
  case 0x2a: {
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    mov(ecx, imm32(i16));
    call(&RSP::instructionSW);
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
    mov(esi, Rt);
    shl(esi, imm8(Sa));
    mov(Rd, esi);
    return 0;
  }

  //INVALID
  case 0x01: {
    return 0;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    mov(esi, Rt);
    shr(esi, imm8(Sa));
    mov(Rd, esi);
    return 0;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    mov(esi, Rt);
    sar(esi, imm8(Sa));
    mov(Rd, esi);
    return 0;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    mov(esi, Rt);
    mov(ecx, Rs);
    and(cl, imm8(31));
    shl(esi, cl);
    mov(Rd, esi);
    return 0;
  }

  //INVALID
  case 0x05: {
    return 0;
  }

  //SRLV Rd,Rt,Rs
  case 0x06: {
    mov(esi, Rt);
    mov(ecx, Rs);
    and(cl, imm8(31));
    shr(esi, cl);
    mov(Rd, esi);
    return 0;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    mov(esi, Rt);
    mov(ecx, Rs);
    and(cl, imm8(31));
    sar(esi, cl);
    mov(Rd, esi);
    return 0;
  }

  //JR Rs
  case 0x08: {
    lea(rsi, dis8(rbx, Rsn*4));
    call(&RSP::instructionJR);
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    lea(rsi, dis8(rbx, Rdn*4));
    lea(rdx, dis8(rbx, Rsn*4));
    call(&RSP::instructionJALR);
    return 1;
  }

  //INVALID
  case 0x0a ... 0x0c: {
    return 0;
  }

  //BREAK
  case 0x0d: {
    call(&RSP::instructionBREAK);
    return 1;
  }

  //INVALID
  case 0x0e ... 0x1f: {
    return 0;
  }

  //ADDU Rd,Rs,Rt
  case 0x20 ... 0x21: {
    mov(esi, Rs);
    add(esi, Rt);
    mov(Rd, esi);
    return 0;
  }

  //SUBU Rd,Rs,Rt
  case 0x22 ... 0x23: {
    mov(esi, Rs);
    sub(esi, Rt);
    mov(Rd, esi);
    return 0;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    mov(esi, Rs);
    and(esi, Rt);
    mov(Rd, esi);
    return 0;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    mov(esi, Rs);
    or(esi, Rt);
    mov(Rd, esi);
    return 0;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    mov(esi, Rs);
    xor(esi, Rt);
    mov(Rd, esi);
    return 0;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    mov(esi, Rs);
    or(esi, Rt);
    not(esi);
    mov(Rd, esi);
    return 0;
  }

  //INVALID
  case 0x28 ... 0x29: {
    return 0;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    mov(esi, Rs);
    cmp(esi, Rt);
    setl(al);
    movzx(eax, al);
    mov(Rd, eax);
    return 0;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    mov(esi, Rs);
    cmp(esi, Rt);
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
    lea(rsi, dis8(rbx, Rsn*4));
    mov(edx, imm32(i16));
    call(&RSP::instructionBLTZ);
    return 1;
  }

  //BGEZ Rs,i16
  case 0x01: {
    lea(rsi, dis8(rbx, Rsn*4));
    mov(edx, imm32(i16));
    call(&RSP::instructionBGEZ);
    return 1;
  }

  //INVALID
  case 0x02 ... 0x0f: {
    return 0;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    lea(rsi, dis8(rbx, Rsn*4));
    mov(edx, imm32(i16));
    call(&RSP::instructionBLTZAL);
    return 1;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    lea(rsi, dis8(rbx, Rsn*4));
    mov(edx, imm32(i16));
    call(&RSP::instructionBGEZAL);
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
    lea(rsi, dis8(rbx, Rtn*4));
    mov(edx, imm32(Rdn));
    call(&RSP::instructionMFC0);
    return 0;
  }

  //INVALID
  case 0x01 ... 0x03: {
    return 0;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    lea(rsi, dis8(rbx, Rtn*4));
    mov(edx, imm32(Rdn));
    call(&RSP::instructionMTC0);
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
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis32(r13, Vsn*16));
    mov(ecx, imm32(E));
    call(&RSP::instructionMFC2);
    return 0;
  }

  //INVALID
  case 0x01: {
    return 0;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    lea(rsi, dis8(rbx, Rtn*4));
    mov(edx, imm32(Rdn));
    call(&RSP::instructionCFC2);
    return 0;
  }

  //INVALID
  case 0x03: {
    return 0;
  }

  //MTC2 Rt,Vs(e)
  case 0x04: {
    lea(rsi, dis8(rbx, Rtn*4));
    lea(rdx, dis32(r13, Vsn*16));
    mov(ecx, imm32(E));
    call(&RSP::instructionMTC2);
    return 0;
  }

  //INVALID
  case 0x05: {
    return 0;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    lea(rsi, dis8(rbx, Rtn*4));
    mov(edx, imm32(Rdn));
    call(&RSP::instructionCTC2);
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
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMULF<0>);
    return 0;
  }

  //VMULU Vd,Vs,Vt(e)
  case 0x01: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMULF<1>);
    return 0;
  }

  //VRNDP Vd,Vs,Vt(e)
  case 0x02: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(Vsn));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRND<1>);
    return 0;
  }

  //VMULQ Vd,Vs,Vt(e)
  case 0x03: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMULQ);
    return 0;
  }

  //VMUDL Vd,Vs,Vt(e)
  case 0x04: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMUDL);
    return 0;
  }

  //VMUDM Vd,Vs,Vt(e)
  case 0x05: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMUDM);
    return 0;
  }

  //VMUDN Vd,Vs,Vt(e)
  case 0x06: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMUDN);
    return 0;
  }

  //VMUDH Vd,Vs,Vt(e)
  case 0x07: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMUDH);
    return 0;
  }

  //VMACF Vd,Vs,Vt(e)
  case 0x08: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMACF<0>);
    return 0;
  }

  //VMACU Vd,Vs,Vt(e)
  case 0x09: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMACF<1>);
    return 0;
  }

  //VRNDN Vd,Vs,Vt(e)
  case 0x0a: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(Vsn));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRND<0>);
    return 0;
  }

  //VMACQ Vd
  case 0x0b: {
    lea(rsi, dis32(r13, Vdn*16));
    call(&RSP::instructionVMACQ);
    return 0;
  }

  //VMADL Vd,Vs,Vt(e)
  case 0x0c: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMADL);
    return 0;
  }

  //VMADM Vd,Vs,Vt(e)
  case 0x0d: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMADM);
    return 0;
  }

  //VMADN Vd,Vs,Vt(e)
  case 0x0e: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMADN);
    return 0;
  }

  //VMADH Vd,Vs,Vt(e)
  case 0x0f: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMADH);
    return 0;
  }

  //VADD Vd,Vs,Vt(e)
  case 0x10: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVADD);
    return 0;
  }

  //VSUB Vd,Vs,Vt(e)
  case 0x11: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVSUB);
    return 0;
  }

  //INVALID
  case 0x12: {
    return 0;
  }

  //VABS Vd,Vs,Vt(e)
  case 0x13: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVABS);
    return 0;
  }

  //VADDC Vd,Vs,Vt(e)
  case 0x14: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVADDC);
    return 0;
  }

  //VSUBC Vd,Vs,Vt(e)
  case 0x15: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVSUBC);
    return 0;
  }

  //INVALID
  case 0x16 ... 0x1c: {
    return 0;
  }

  //VSAR Vd,Vs,E
  case 0x1d: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    mov(ecx, imm32(E));
    call(&RSP::instructionVSAR);
    return 0;
  }

  //INVALID
  case 0x1e ... 0x1f: {
    return 0;
  }

  //VLT Vd,Vs,Vt(e)
  case 0x20: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVLT);
    return 0;
  }

  //VEQ Vd,Vs,Vt(e)
  case 0x21: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVEQ);
    return 0;
  }

  //VNE Vd,Vs,Vt(e)
  case 0x22: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVNE);
    return 0;
  }

  //VGE Vd,Vs,Vt(e)
  case 0x23: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVGE);
    return 0;
  }

  //VCL Vd,Vs,Vt(e)
  case 0x24: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVCL);
    return 0;
  }

  //VCH Vd,Vs,Vt(e)
  case 0x25: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVCH);
    return 0;
  }

  //VCR Vd,Vs,Vt(e)
  case 0x26: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVCR);
    return 0;
  }

  //VMRG Vd,Vs,Vt(e)
  case 0x27: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMRG);
    return 0;
  }

  //VAND Vd,Vs,Vt(e)
  case 0x28: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVAND);
    return 0;
  }

  //VNAND Vd,Vs,Vt(e)
  case 0x29: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVNAND);
    return 0;
  }

  //VOR Vd,Vs,Vt(e)
  case 0x2a: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVOR);
    return 0;
  }

  //VNOR Vd,Vs,Vt(e)
  case 0x2b: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVNOR);
    return 0;
  }

  //VXOR Vd,Vs,Vt(e)
  case 0x2c: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVXOR);
    return 0;
  }

  //VNXOR Vd,Vs,Vt(e)
  case 0x2d: {
    lea(rsi, dis32(r13, Vdn*16));
    lea(rdx, dis32(r13, Vsn*16));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVNXOR);
    return 0;
  }

  //INVALID
  case 0x2e ... 0x2f: {
    return 0;
  }

  //VCRP Vd(de),Vt(e)
  case 0x30: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRCP<0>);
    return 0;
  }

  //VRCPL Vd(de),Vt(e)
  case 0x31: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRCP<1>);
    return 0;
  }

  //VRCPH Vd(de),Vt(e)
  case 0x32: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRCPH);
    return 0;
  }

  //VMOV Vd(de),Vt(e)
  case 0x33: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVMOV);
    return 0;
  }

  //VRSQ Vd(de),Vt(e)
  case 0x34: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRSQ<0>);
    return 0;
  }

  //VRSQL Vd(de),Vt(e)
  case 0x35: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRSQ<1>);
    return 0;
  }

  //VRSQH Vd(de),Vt(e)
  case 0x36: {
    lea(rsi, dis32(r13, Vdn*16));
    mov(edx, imm32(DE));
    lea(rcx, dis32(r13, Vtn*16));
    mov(r8d, imm32(E));
    call(&RSP::instructionVRSQH);
    return 0;
  }

  //VNOP
  case 0x37: {
    call(&RSP::instructionVNOP);
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
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLBV);
    return 0;
  }

  //LSV Vt(e),Rs,i7
  case 0x01: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLSV);
    return 0;
  }

  //LLV Vt(e),Rs,i7
  case 0x02: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLLV);
    return 0;
  }

  //LDV Vt(e),Rs,i7
  case 0x03: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLDV);
    return 0;
  }

  //LQV Vt(e),Rs,i7
  case 0x04: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLQV);
    return 0;
  }

  //LRV Vt(e),Rs,i7
  case 0x05: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLRV);
    return 0;
  }

  //LPV Vt(e),Rs,i7
  case 0x06: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLPV);
    return 0;
  }

  //LUV Vt(e),Rs,i7
  case 0x07: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLUV);
    return 0;
  }

  //LHV Vt(e),Rs,i7
  case 0x08: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLHV);
    return 0;
  }

  //LFV Vt(e),Rs,i7
  case 0x09: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLFV);
    return 0;
  }

  //LWV (not present on N64 RSP)
  case 0x0a: {
    return 0;
  }

  //LTV Vt(e),Rs,u7
  case 0x0b: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionLTV);
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
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSBV);
    return 0;
  }

  //SSV Vt(e),Rs,i7
  case 0x01: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSSV);
    return 0;
  }

  //SLV Vt(e),Rs,i7
  case 0x02: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSLV);
    return 0;
  }

  //SDV Vt(e),Rs,i7
  case 0x03: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSDV);
    return 0;
  }

  //SQV Vt(e),Rs,i7
  case 0x04: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSQV);
    return 0;
  }

  //SRV Vt(e),Rs,i7
  case 0x05: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSRV);
    return 0;
  }

  //SPV Vt(e),Rs,i7
  case 0x06: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSPV);
    return 0;
  }

  //SUV Vt(e),Rs,i7
  case 0x07: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSUV);
    return 0;
  }

  //SHV Vt(e),Rs,i7
  case 0x08: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSHV);
    return 0;
  }

  //SFV Vt(e),Rs,i7
  case 0x09: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSFV);
    return 0;
  }

  //SWV Vt(e),Rs,i7
  case 0x0a: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSWV);
    return 0;
  }

  //STV Vt(e),Rs,i7
  case 0x0b: {
    lea(rsi, dis32(r13, Vtn*16));
    mov(edx, imm32(E));
    lea(rcx, dis8(rbx, Rsn*4));
    mov(r8d, imm32(i7));
    call(&RSP::instructionSTV);
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
  #if defined(PLATFORM_WINDOWS)
  mov(r8, rdx);
  mov(r9, rcx);
  mov(rdx, rsi);
  mov(rcx, rbp);
  mov(rax, imm64(function));
  call(rax);
  #else
  mov(rdi, rbp);
  mov(rax, imm64(function));
  call(rax);
  #endif
}
