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
  mov(rbx, imm64(&self.ipu.r[0]));
  mov(rbp, imm64(&self));
  mov(r13, imm64(&self.fpu.r[0]));

  bool hasBranched = 0;
  while(true) {
    u32 instruction = bus.read<Word>(address);
    bool branched = emitEXECUTE(instruction);
    if(unlikely(instruction == 0x1000'ffff)) {
      //accelerate idle loops
      mov(rax, mem64(&self.clock));
      add(rax, imm8(64));
      mov(mem64(&self.clock), rax);
    }
    call(&CPU::instructionEpilogue, &self);
    address += 4;
    if(hasBranched || (address & 0xfc) == 0) break;  //block boundary
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

#define OP instruction
#define RD &self.ipu.r[RDn]
#define RT &self.ipu.r[RTn]
#define RS &self.ipu.r[RSn]

#define jp(id, name) \
  case id: \
    return emit##name(instruction); \

#define op(id, name, ...) \
  case id: \
    call(&CPU::instruction##name, &self, ##__VA_ARGS__); \
    return 0; \

#define br(id, name, ...) \
  case id: \
    call(&CPU::instruction##name, &self, ##__VA_ARGS__); \
    return 1; \

#define Sa  (instruction >>  6 & 31)
#define Rdn (instruction >> 11 & 31)
#define Rtn (instruction >> 16 & 31)
#define Rsn (instruction >> 21 & 31)
#define Fdn (instruction >>  6 & 31)
#define Fsn (instruction >> 11 & 31)
#define Ftn (instruction >> 16 & 31)
#define Rd  dis32(rbx, Rdn * 8)
#define Rt  dis32(rbx, Rtn * 8)
#define Rs  dis32(rbx, Rsn * 8)
#define Fd  dis32(r13, Fdn * 8)
#define Fs  dis32(r13, Fsn * 8)
#define Ft  dis32(r13, Ftn * 8)
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
    mov(esi, imm32(n26));
    call(&CPU::instructionJ);
    return 1;
  }

  //JAL n26
  case 0x03: {
    mov(esi, imm32(n26));
    call(&CPU::instructionJAL);
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionBEQ);
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionBNE);
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBLEZ);
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBGTZ);
    return 1;
  }

  //ADDI Rt,Rs,i16
  case 0x08: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionADDI);
    return 0;
  }

  //ADDIU Rt,Rs,i16
  case 0x09: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionADDIU);
    return 0;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSLTI);
    return 0;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSLTIU);
    return 0;
  }

  //ANDI Rt,Rs,u16
  case 0x0c: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(n16));
    call(&CPU::instructionANDI);
    return 0;
  }

  //ORI Rt,Rs,u16
  case 0x0d: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(n16));
    call(&CPU::instructionORI);
    return 0;
  }

  //XORI Rt,Rs,u16
  case 0x0e: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(n16));
    call(&CPU::instructionXORI);
    return 0;
  }

  //LUI Rt,u16
  case 0x0f: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(n16));
    call(&CPU::instructionLUI);
    return 0;
  }

  //SCC
  case 0x10: {
    return emitSCC(instruction);
  }

  //FPU
  case 0x11: {
    return emitFPU(instruction);
  }

  //COP2
  case 0x12: {
    call(&CPU::instructionCOP2);
    return 1;
  }

  //COP3
  case 0x13: {
    call(&CPU::instructionCOP3);
    return 1;
  }

  //BEQL Rs,Rt,i16
  case 0x14: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionBEQL);
    return 1;
  }

  //BNEL Rs,Rt,i16
  case 0x15: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionBNEL);
    return 1;
  }

  //BLEZL Rs,i16
  case 0x16: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBLEZL);
    return 1;
  }

  //BGTZL Rs,i16
  case 0x17: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBGTZL);
    return 1;
  }

  //DADDI Rt,Rs,i16
  case 0x18: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionDADDI);
    return 0;
  }

  //DADDIU Rt,Rs,i16
  case 0x19: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionDADDIU);
    return 0;
  }

  //LDL Rt,Rs,i16
  case 0x1a: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLDL);
    return 0;
  }

  //LDR Rt,Rs,i16
  case 0x1b: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLDR);
    return 0;
  }

  //INVALID
  case 0x1c ... 0x1f: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLB);
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLH);
    return 0;
  }

  //LWL Rt,Rs,i16
  case 0x22: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLWL);
    return 0;
  }

  //LW Rt,Rs,i16
  case 0x23: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLW);
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLBU);
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLHU);
    return 0;
  }

  //LWR Rt,Rs,i16
  case 0x26: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLWR);
    return 0;
  }

  //LWU Rt,Rs,i16
  case 0x27: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLWU);
    return 0;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSB);
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSH);
    return 0;
  }

  //SWL Rt,Rs,i16
  case 0x2a: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSWL);
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSW);
    return 0;
  }

  //SDL Rt,Rs,i16
  case 0x2c: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSDL);
    return 0;
  }

  //SDR Rt,Rs,i16
  case 0x2d: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSDR);
    return 0;
  }

  //SWR Rt,Rs,i16
  case 0x2e: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSWR);
    return 0;
  }

  //CACHE op(offset),base
  case 0x2f: {
    mov(esi, imm32(instruction >> 16 & 31));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionCACHE);
    return 0;
  }

  //LL Rt,Rs,i16
  case 0x30: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLL);
    return 0;
  }

  //LWC1 Ft,Rs,i16
  case 0x31: {
    mov(esi, imm32(Ftn));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLWC1);
    return 0;
  }

  //LWC2
  case 0x32: {
    call(&CPU::instructionCOP2);
    return 1;
  }

  //LWC3
  case 0x33: {
    call(&CPU::instructionCOP3);
    return 1;
  }

  //LLD Rt,Rs,i16
  case 0x34: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLLD);
    return 0;
  }

  //LDC1 Ft,Rs,i16
  case 0x35: {
    mov(esi, imm32(Ftn));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLDC1);
    return 0;
  }

  //LDC2
  case 0x36: {
    call(&CPU::instructionCOP2);
    return 1;
  }

  //LD Rt,Rs,i16
  case 0x37: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionLD);
    return 0;
  }

  //SC Rt,Rs,i16
  case 0x38: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSC);
    return 0;
  }

  //SWC1 Ft,Rs,i16
  case 0x39: {
    mov(esi, imm32(Ftn));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSWC1);
    return 0;
  }

  //SWC2
  case 0x3a: {
    call(&CPU::instructionCOP2);
    return 1;
  }

  //SWC3
  case 0x3b: {
    call(&CPU::instructionCOP3);
    return 1;
  }

  //SCD Rt,Rs,i16
  case 0x3c: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSCD);
    return 0;
  }

  //SDC1 Ft,Rs,i16
  case 0x3d: {
    mov(esi, imm32(Ftn));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSDC1);
    return 0;
  }

  //SDC2
  case 0x3e: {
    call(&CPU::instructionCOP2);
    return 1;
  }

  //SD Rt,Rs,i16
  case 0x3f: {
    lea(rsi, dis32(rbx, Rtn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    mov(ecx, imm32(i16));
    call(&CPU::instructionSD);
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa));
    call(&CPU::instructionSLL);
    return 0;
  }

  //INVALID
  case 0x01: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa));
    call(&CPU::instructionSRL);
    return 0;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa));
    call(&CPU::instructionSRA);
    return 0;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    lea(rcx, dis32(rbx, Rsn*8));
    call(&CPU::instructionSLLV);
    return 0;
  }

  //INVALID
  case 0x05: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //SRLV Rd,Rt,RS
  case 0x06: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    lea(rcx, dis32(rbx, Rsn*8));
    call(&CPU::instructionSRLV);
    return 0;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    lea(rcx, dis32(rbx, Rsn*8));
    call(&CPU::instructionSRAV);
    return 0;
  }

  //JR Rs
  case 0x08: {
    lea(rsi, dis32(rbx, Rsn*8));
    call(&CPU::instructionJR);
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    call(&CPU::instructionJALR);
    return 1;
  }

  //INVALID
  case 0x0a ... 0x0b: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //SYSCALL
  case 0x0c: {
    call(&CPU::instructionSYSCALL);
    return 1;
  }

  //BREAK
  case 0x0d: {
    call(&CPU::instructionBREAK);
    return 1;
  }

  //INVALID
  case 0x0e: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //SYNC
  case 0x0f: {
    call(&CPU::instructionSYNC);
    return 0;
  }

  //MFHI Rd
  case 0x10: {
    lea(rsi, dis32(rbx, Rdn*8));
    call(&CPU::instructionMFHI);
    return 0;
  }

  //MTHI Rs
  case 0x11: {
    lea(rsi, dis32(rbx, Rsn*8));
    call(&CPU::instructionMTHI);
    return 0;
  }

  //MFLO Rd
  case 0x12: {
    lea(rsi, dis32(rbx, Rdn*8));
    call(&CPU::instructionMFLO);
    return 0;
  }

  //MTLO Rs
  case 0x13: {
    lea(rsi, dis32(rbx, Rsn*8));
    call(&CPU::instructionMTLO);
    return 0;
  }

  //DSLLV Rd,Rt,Rs
  case 0x14: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    lea(rcx, dis32(rbx, Rsn*8));
    call(&CPU::instructionDSLLV);
    return 0;
  }

  //INVALID
  case 0x15: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //DSRLV Rd,Rt,Rs
  case 0x16: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    lea(rcx, dis32(rbx, Rsn*8));
    call(&CPU::instructionDSRLV);
    return 0;
  }

  //DSRAV Rd,Rt,Rs
  case 0x17: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    lea(rcx, dis32(rbx, Rsn*8));
    call(&CPU::instructionDSRAV);
    return 0;
  }

  //MULT Rs,Rt
  case 0x18: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionMULT);
    return 0;
  }

  //MULTU Rs,Rt
  case 0x19: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionMULTU);
    return 0;
  }

  //DIV Rs,Rt
  case 0x1a: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDIV);
    return 0;
  }

  //DIVU Rs,Rt
  case 0x1b: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDIVU);
    return 0;
  }

  //DMULT Rs,Rt
  case 0x1c: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDMULT);
    return 0;
  }

  //DMULTU Rs,Rt
  case 0x1d: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDMULTU);
    return 0;
  }

  //DDIV Rs,Rt
  case 0x1e: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDDIV);
    return 0;
  }

  //DDIVU Rs,Rt
  case 0x1f: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDDIVU);
    return 0;
  }

  //ADD Rd,Rs,Rt
  case 0x20: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionADD);
    return 0;
  }

  //ADDU Rd,Rs,Rt
  case 0x21: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionADDU);
    return 0;
  }

  //SUB Rd,Rs,Rt
  case 0x22: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionSUB);
    return 0;
  }

  //SUBU Rd,Rs,Rt
  case 0x23: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionSUBU);
    return 0;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionAND);
    return 0;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionOR);
    return 0;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionXOR);
    return 0;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionNOR);
    return 0;
  }

  //INVALID
  case 0x28 ... 0x29: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionSLT);
    return 0;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionSLTU);
    return 0;
  }

  //DADD Rd,Rs,Rt
  case 0x2c: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDADD);
    return 0;
  }

  //DADDU Rd,Rs,Rt
  case 0x2d: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDADDU);
    return 0;
  }

  //DSUB Rd,Rs,Rt
  case 0x2e: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDSUB);
    return 0;
  }

  //DSUBU Rd,Rs,Rt
  case 0x2f: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rsn*8));
    lea(rcx, dis32(rbx, Rtn*8));
    call(&CPU::instructionDSUBU);
    return 0;
  }

  //TGE Rs,Rt
  case 0x30: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionTGE);
    return 0;
  }

  //TGEU Rs,Rt
  case 0x31: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionTGEU);
    return 0;
  }

  //TLT Rs,Rt
  case 0x32: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionTLT);
    return 0;
  }

  //TLTU Rs,Rt
  case 0x33: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionTLTU);
    return 0;
  }

  //TEQ Rs,Rt
  case 0x34: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionTEQ);
    return 0;
  }

  //INVALID
  case 0x35: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //TNE Rs,Rt
  case 0x36: {
    lea(rsi, dis32(rbx, Rsn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    call(&CPU::instructionTNE);
    return 0;
  }

  //INVALID
  case 0x37: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //DSLL Rd,Rt,Sa
  case 0x38: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa));
    call(&CPU::instructionDSLL);
    return 0;
  }

  //INVALID
  case 0x39: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //DSRL Rd,Rt,Sa
  case 0x3a: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa));
    call(&CPU::instructionDSRL);
    return 0;
  }

  //DSRA Rd,Rt,Sa
  case 0x3b: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa));
    call(&CPU::instructionDSRA);
    return 0;
  }

  //DSLL32 Rd,Rt,Sa
  case 0x3c: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa+32));
    call(&CPU::instructionDSLL);
    return 0;
  }

  //INVALID
  case 0x3d: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //DSRL32 Rd,Rt,Sa
  case 0x3e: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa+32));
    call(&CPU::instructionDSRL);
    return 0;
  }

  //DSRA32 Rd,Rt,Sa
  case 0x3f: {
    lea(rsi, dis32(rbx, Rdn*8));
    lea(rdx, dis32(rbx, Rtn*8));
    mov(ecx, imm32(Sa+32));
    call(&CPU::instructionDSRA);
    return 0;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitREGIMM(u32 instruction) -> bool {
  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBLTZ);
    return 0;
  }

  //BGEZ Rs,i16
  case 0x01: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBGEZ);
    return 0;
  }

  //BLTZL Rs,i16
  case 0x02: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBLTZL);
    return 0;
  }

  //BGEZL Rs,i16
  case 0x03: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBGEZL);
    return 0;
  }

  //INVALID
  case 0x04 ... 0x07: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //TGEI Rs,i16
  case 0x08: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionTGEI);
    return 0;
  }

  //TGEIU Rs,i16
  case 0x09: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionTGEIU);
    return 0;
  }

  //TLTI Rs,i16
  case 0x0a: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionTLTI);
    return 0;
  }

  //TLTIU Rs,i16
  case 0x0b: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionTLTIU);
    return 0;
  }

  //TEQI Rs,i16
  case 0x0c: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionTEQI);
    return 0;
  }

  //INVALID
  case 0x0d: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //TNEI Rs,i16
  case 0x0e: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionTNEI);
    return 0;
  }

  //INVALID
  case 0x0f: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBLTZAL);
    return 0;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBGEZAL);
    return 0;
  }

  //BLTZALL Rs,i16
  case 0x12: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBLTZALL);
    return 0;
  }

  //BGEZALL Rs,i16
  case 0x13: {
    lea(rsi, dis32(rbx, Rsn*8));
    mov(edx, imm32(i16));
    call(&CPU::instructionBGEZALL);
    return 0;
  }

  //INVALID
  case 0x14 ... 0x1f: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitSCC(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC0 Rt,Rd
  case 0x00: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Rdn));
    call(&CPU::instructionMFC0);
    return 0;
  }

  //DMFC0 Rt,Rd
  case 0x01: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Rdn));
    call(&CPU::instructionDMFC0);
    return 0;
  }

  //INVALID
  case 0x02 ... 0x03: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Rdn));
    call(&CPU::instructionMTC0);
    return 0;
  }

  //DMTC0 Rt,Rd
  case 0x05: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Rdn));
    call(&CPU::instructionDMTC0);
    return 0;
  }

  //INVALID
  case 0x06 ... 0x0f: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  }

  switch(instruction & 0x3f) {

  //TLBR
  case 0x01: {
    call(&CPU::instructionTLBR);
    return 0;
  }

  //TLBWI
  case 0x02: {
    call(&CPU::instructionTLBWI);
    return 0;
  }

  //TLBWR
  case 0x06: {
    call(&CPU::instructionTLBWR);
    return 0;
  }

  //TLBP
  case 0x08: {
    call(&CPU::instructionTLBP);
    return 0;
  }

  //ERET
  case 0x18: {
    call(&CPU::instructionERET);
    return 1;
  }

  }

  return 0;
}

auto CPU::Recompiler::emitFPU(u32 instruction) -> bool {
#if 0
  mov(rsi, imm64(&self.scc.status.enable.coprocessor1));
  mov(al, dis(rsi));
  test(al, al);
  jnz(imm8(23));
  #if defined(PLATFORM_WINDOWS)
  mov(rcx, imm64(&self.exception));
  #else
  mov(rdi, imm64(&self.exception));
  #endif
  mov(rax, imm64(&CPU::Exception::coprocessor1));
  call(rax);
  ret();
#endif

  switch(instruction >> 21 & 0x1f) {

  //MFC1 Rt,Fs
  case 0x00: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionMFC1);
    return 0;
  }

  //DMFC1 Rt,Fs
  case 0x01: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionDMFC1);
    return 0;
  }

  //CFC1 Rt,Rd
  case 0x02: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Rdn));
    call(&CPU::instructionCFC1);
    return 0;
  }

  //INVALID
  case 0x03: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //MTC1 Rt,Fs
  case 0x04: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionMTC1);
    return 0;
  }

  //DMTC1 Rt,Fs
  case 0x05: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionDMTC1);
    return 0;
  }

  //CTC1 Rt,Rd
  case 0x06: {
    lea(rsi, dis32(rbx, Rtn*8));
    mov(edx, imm32(Rdn));
    call(&CPU::instructionCTC1);
    return 0;
  }

  //INVALID
  case 0x07: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  //BC1 offset
  case 0x08: {
    mov(esi, imm32(instruction >> 16 & 1));
    mov(edx, imm32(instruction >> 17 & 1));
    mov(ecx, imm32(i16));
    call(&CPU::instructionBC1);
    return 1;
  }

  //INVALID
  case 0x09 ... 0x0f: {
    call(&CPU::instructionINVALID);
    return 1;
  }

  }

  if((instruction >> 21 & 31) == 16)
  switch(instruction & 0x3f) {
    //todo
  }

  if((instruction >> 21 & 31) == 17)
  switch(instruction & 0x3f) {
    //todo
  }

  if((instruction >> 21 & 31) == 20)
  switch(instruction & 0x3f) {

  //FCVT.S.W Fd,Fs
  case 0x20: {
    mov(esi, imm32(Fdn));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionFCVT_S_W);
    return 0;
  }

  //FCVT.D.W Fd,Fs
  case 0x21: {
    mov(esi, imm32(Fdn));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionFCVT_D_W);
    return 0;
  }

  }

  if((instruction >> 21 & 31) == 21)
  switch(instruction & 0x3f) {

  //FCVT.S.L
  case 0x20: {
    mov(esi, imm32(Fdn));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionFCVT_S_L);
    return 0;
  }

  //FCVT.D.L
  case 0x21: {
    mov(esi, imm32(Fdn));
    mov(edx, imm32(Fsn));
    call(&CPU::instructionFCVT_D_L);
    return 0;
  }

  }

  #define DECODER_FPU
  #include "decoder.hpp"
  return 0;
}

#undef Sa
#undef Rdn
#undef Rtn
#undef Rsn
#undef Fdn
#undef Fsn
#undef Ftn
#undef Rd
#undef Rt
#undef Rs
#undef Fd
#undef Fs
#undef Ft
#undef i16
#undef n16
#undef n26

#undef jp
#undef op
#undef br

#undef OP
#undef RD
#undef RT
#undef RS

template<typename V, typename... P>
auto CPU::Recompiler::call(V (CPU::*function)(P...)) -> void {
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
