auto SH2::Recompiler::pool(u32 address) -> Pool* {
  auto& pool = pools[address >> 8 & 0xffffff];
  if(!pool) pool = (Pool*)allocator.acquire(sizeof(Pool));
  return pool;
}

auto SH2::Recompiler::block(u32 address) -> Block* {
  if(auto block = pool(address)->blocks[address >> 1 & 0x7f]) return block;
  auto block = emit(address);
  return pool(address)->blocks[address >> 1 & 0x7f] = block;
}

auto SH2::Recompiler::emit(u32 address) -> Block* {
  if(unlikely(allocator.available() < 1_MiB)) {
    print("SH2 allocator flush\n");
    allocator.release(bump_allocator::zero_fill);
    reset();
  }

  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = allocator.acquire();
  bind({block->code, allocator.available()});
  push(rbx);
  push(rbx);
  mov(rax, imm64{&self.R[0]});
  mov(rbx, rax);

  bool hasBranched = 0;
  while(true) {
    u16 instruction = self.readWord(address);
    bool branched = emitInstruction(instruction);
    mov(rax, mem64(&clock));
    inc(rax);
    mov(mem64(&clock), rax);
    call(&SH2::instructionEpilogue, &self);
    test(rax, rax);
    jz(imm8(+3));
    pop(rbx);
    pop(rbx);
    ret();
    address += 2;
    if(hasBranched || (address & 0xfe) == 0) break;  //block boundary
    hasBranched = branched;
  }
  pop(rbx);
  pop(rbx);
  ret();

  allocator.reserve(size());
  return block;
}

#define DYNAREC

#define call0(function) \
  sub(rsp, imm8(0x28)); \
  mov(cr0, imm64(&self)); \
  mov(rax, imm64(&SH2::function)); \
  call(rax); \
  add(rsp, imm8(0x28));
#define call1(function, argument1) \
  sub(rsp, imm8(0x28)); \
  mov(cr0, imm64(&self)); \
  mov(cr1, argument1); \
  mov(rax, imm64(&SH2::function)); \
  call(rax); \
  add(rsp, imm8(0x28));
#define call2(function, argument1, argument2) \
  sub(rsp, imm8(0x28)); \
  mov(cr0, imm64(&self)); \
  mov(cr1, argument1); \
  mov(cr2, argument2); \
  mov(rax, imm64(&SH2::function)); \
  call(rax); \
  add(rsp, imm8(0x28));

auto SH2::Recompiler::emitInstruction(u16 opcode) -> bool {
#ifdef DYNAREC
  #define PC  self.PC
  #define SR  self.SR
  #define GBR self.GBR
  #define MAC self.MAC

  #define n   (opcode >> 8 & 0x00f)*4
  #define m   (opcode >> 4 & 0x00f)*4
  #define i   (opcode >> 0 & 0x0ff)
  #define d4  (opcode >> 0 & 0x00f)
  #define d8  (opcode >> 0 & 0x0ff)
  #define d12 (opcode >> 0 & 0xfff)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {

  //MOV.B Rm,@(R0,Rn)
  case 0x04: {
    mov(eax, dis8(rbx,n));
    add(eax, dis(rbx));
    call2(writeByte, rax, dis8(rbx,m));
    return 0;
  }

  //MOV.W Rm,@(R0,Rn)
  case 0x05: {
    mov(eax, dis8(rbx,n));
    add(eax, dis(rbx));
    call2(writeWord, rax, dis8(rbx,m));
    return 0;
  }

  //MOV.L Rm,@(R0,Rn)
  case 0x06: {
    mov(eax, dis8(rbx,n));
    add(eax, dis(rbx));
    call2(writeLong, rax, dis8(rbx,m));
    return 0;
  }

  //MOV.B @(R0,Rm),Rn
  case 0x0c: {
    mov(eax, dis8(rbx,m));
    add(eax, dis(rbx));
    call1(readByte, rax);
    movsx(eax, al);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.W @(R0,Rm),Rn
  case 0x0d: {
    mov(eax, dis8(rbx,m));
    add(eax, dis(rbx));
    call1(readWord, rax);
    movsx(eax, ax);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.L @(R0,Rm),Rn
  case 0x0e: {
    mov(eax, dis8(rbx,m));
    add(eax, dis(rbx));
    call1(readLong, rax);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.L Rm,@(disp,Rn)
  case 0x10 ... 0x1f: {
    mov(eax, dis8(rbx,n));
    add(eax, imm8(d4*4));
    call2(writeLong, rax, dis8(rbx,m));
    return 0;
  }

  //MOV.B Rm,@Rn
  case 0x20: {
    call2(writeByte, dis8(rbx,n), dis8(rbx,m));
    return 0;
  }

  //MOV.W Rm,@Rn
  case 0x21: {
    call2(writeWord, dis8(rbx,n), dis8(rbx,m));
    return 0;
  }

  //MOV.L Rm,@Rn
  case 0x22: {
    call2(writeLong, dis8(rbx,n), dis8(rbx,m));
    return 0;
  }

  //MOV.B Rm,@-Rn
  case 0x24: {
    xor(rax, rax);
    mov(eax, dis8(rbx,n));
    dec(eax);
    mov(dis8(rbx,n), eax);
    call2(writeByte, rax, dis8(rbx,m));
    return 0;
  }

  //MOV.W Rm,@-Rn
  case 0x25: {
    xor(rax, rax);
    mov(eax, dis8(rbx,n));
    sub(eax, imm8(2));
    mov(dis8(rbx,n), eax);
    call2(writeWord, rax, dis8(rbx,m));
    return 0;
  }

  //MOV.L Rm,@-Rn
  case 0x26: {
    xor(rax, rax);
    mov(eax, dis8(rbx,n));
    sub(eax, imm8(4));
    mov(dis8(rbx,n), eax);
    call2(writeLong, rax, dis8(rbx,m));
    return 0;
  }

  //TST Rm,Rn
  case 0x28: {
    mov(eax, dis8(rbx,n));
    and(eax, dis8(rbx,m));
    mov(rax, imm64(&SR.T));
    setz(dis(rax));
    return 0;
  }

  //AND Rm,Rn
  case 0x29: {
    mov(eax, dis8(rbx,n));
    and(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //XOR Rm,Rn
  case 0x2a: {
    mov(eax, dis8(rbx,n));
    xor(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //OR Rm,Rn
  case 0x2b: {
    mov(eax, dis8(rbx,n));
    or(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //CMP/EQ Rm,Rn
  case 0x30: {
    mov(eax, dis8(rbx,n));
    cmp(eax, dis8(rbx,m));
    mov(rax, imm64(&SR.T));
    setz(dis(rax));
    return 0;
  }

  //CMP/HS Rm,Rn
  case 0x32: {
    mov(eax, dis8(rbx,n));
    cmp(eax, dis8(rbx,m));
    mov(rax, imm64(&SR.T));
    setnc(dis(rax));
    return 0;
  }

  //CMP/GE Rm,Rn
  case 0x33: {
    mov(eax, dis8(rbx,n));
    cmp(eax, dis8(rbx,m));
    mov(rax, imm64(&SR.T));
    setge(dis(rax));
    return 0;
  }

  //CMP/HI Rm,Rn
  case 0x36: {
    mov(eax, dis8(rbx,n));
    cmp(eax, dis8(rbx,m));
    mov(rax, imm64(&SR.T));
    seta(dis(rax));
    return 0;
  }

  //CMP/GT Rm,Rn
  case 0x37: {
    mov(eax, dis8(rbx,n));
    cmp(eax, dis8(rbx,m));
    mov(rax, imm64(&SR.T));
    setg(dis(rax));
    return 0;
  }

  //SUB Rm,Rn
  case 0x38: {
    mov(eax, dis8(rbx,n));
    sub(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //SUBC Rm,Rn
  case 0x3a: {
    mov(rax, mem64(&SR.T));
    rcr(al);
    mov(eax, dis8(rbx,n));
    sbb(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    mov(rax, imm64(&SR.T));
    setc(dis(rax));
    return 0;
  }

  //ADD Rm,Rn
  case 0x3c: {
    mov(eax, dis8(rbx,n));
    add(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //ADDC Rm,Rn
  case 0x3e: {
    mov(rax, mem64(&SR.T));
    rcr(al);
    mov(eax, dis8(rbx,n));
    adc(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    mov(rax, imm64(&SR.T));
    setc(dis(rax));
    return 0;
  }

  //MOV.L @(disp,Rm),Rn
  case 0x50 ... 0x5f: {
    mov(eax, dis8(rbx,m));
    add(eax, imm8(d4*4));
    call1(readLong, rax);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.B @Rm,Rn
  case 0x60: {
    call1(readByte, dis8(rbx,m));
    movsx(eax, al);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.W @Rm,Rn
  case 0x61: {
    call1(readWord, dis8(rbx,m));
    movsx(eax, ax);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.L @Rm,Rn
  case 0x62: {
    call1(readLong, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV Rm,Rn
  case 0x63: {
    mov(eax, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.B @Rm+,Rn
  case 0x64: {
    call1(readByte, dis8(rbx,m));
    movsx(eax, al);
    mov(dis8(rbx,n), eax);
    mov(eax, dis8(rbx,m));
    inc(eax);
    mov(dis8(rbx,m), eax);
    return 0;
  }

  //MOV.W @Rm+,Rn
  case 0x65: {
    call1(readWord, dis8(rbx,m));
    movsx(eax, ax);
    mov(dis8(rbx,n), eax);
    mov(eax, dis8(rbx,m));
    add(eax, imm8(2));
    mov(dis8(rbx,m), eax);
    return 0;
  }

  //MOV.L @Rm+,Rn
  case 0x66: {
    call1(readLong, dis8(rbx,m));
    mov(dis8(rbx,n), eax);
    mov(eax, dis8(rbx,m));
    add(eax, imm8(4));
    mov(dis8(rbx,m), eax);
    return 0;
  }

  //ADD #imm,Rn
  case 0x70 ... 0x7f: {
    mov(al, imm8(d8));
    movsx(eax, al);
    add(eax, dis8(rbx,n));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.W @(disp,PC),Rn
  case 0x90 ... 0x9f: {
    mov(rax, mem64(&PC));
    add(eax, imm32(d8*2));
    call1(readWord, rax);
    movsx(eax, ax);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV.L @(disp,PC),Rn
  case 0xd0 ... 0xdf: {
    mov(rax, mem64(&PC));
    and(al, imm8(~3));
    add(eax, imm32(d8*4));
    call1(readLong, rax);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //MOV #imm,Rn
  case 0xe0 ... 0xef: {
    mov(eax, imm32((s8)i));
    mov(dis8(rbx,n), eax);
    return 0;
  }

  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8
  #undef d12

  #define n  (opcode >> 8 & 0x0f)*4
  #define m  (opcode >> 4 & 0x0f)*4  //n for MOVBS4, MOVWS4
  #define i  (opcode >> 0 & 0xff)
  #define d4 (opcode >> 0 & 0x0f)
  #define d8 (opcode >> 0 & 0xff)
  switch(opcode >> 8) {

  //MOV.B R0,@(disp,Rn)
  case 0x80: {
    mov(eax, dis8(rbx,m));
    add(eax, imm8(d4));
    call2(writeByte, rax, dis(rbx));
    return 0;
  }

  //MOV.W R0,@(disp,Rn)
  case 0x81: {
    mov(eax, dis8(rbx,m));
    add(eax, imm8(d4*2));
    call2(writeWord, rax, dis(rbx));
    return 0;
  }

  //MOV.B @(disp,Rn),R0
  case 0x84: {
    mov(eax, dis8(rbx,m));
    add(eax, imm8(d4));
    call1(readByte, rax);
    movsx(eax, al);
    mov(dis(rbx), eax);
    return 0;
  }

  //MOV.W @(disp,Rn),R0
  case 0x85: {
    mov(eax, dis8(rbx,m));
    add(eax, imm8(d4*2));
    call1(readWord, rax);
    movsx(eax, ax);
    mov(dis(rbx), eax);
    return 0;
  }

  //CMP/EQ #imm,R0
  case 0x88: {
    mov(eax, dis(rbx));
    cmp(eax, imm32((s8)i));
    mov(rax, imm64(&SR.T));
    setz(dis(rax));
    return 0;
  }

  //MOV.B R0,@(disp,GBR)
  case 0xc0: {
    mov(eax, mem64(&GBR));
    add(eax, imm32(d8));
    call2(writeByte, rax, dis(rbx));
    return 0;
  }

  //MOV.W R0,@(disp,GBR)
  case 0xc1: {
    mov(eax, mem64(&GBR));
    add(eax, imm32(d8*2));
    call2(writeWord, rax, dis(rbx));
    return 0;
  }

  //MOV.L R0,@(disp,GBR)
  case 0xc2: {
    mov(eax, mem64(&GBR));
    add(eax, imm32(d8*4));
    call2(writeLong, rax, dis(rbx));
    return 0;
  }

  //MOV.B @(disp,GBR),R0
  case 0xc4: {
    mov(eax, mem64(&GBR));
    add(eax, imm32(d8));
    call1(readByte, rax);
    movsx(eax, al);
    mov(dis(rbx), eax);
    return 0;
  }

  //MOV.W @(disp,GBR),R0
  case 0xc5: {
    mov(eax, mem64(&GBR));
    add(eax, imm32(d8*2));
    call1(readWord, rax);
    movsx(eax, ax);
    mov(dis(rbx), eax);
    return 0;
  }

  //MOV.L @(disp,GBR),R0
  case 0xc6: {
    mov(eax, mem64(&GBR));
    add(eax, imm32(d8*4));
    call1(readLong, rax);
    mov(dis(rbx), rax);
    return 0;
  }

  //MOVA @(disp,PC),R0
  case 0xc7: {
    mov(eax, mem64(&PC));
    and(al, imm8(~3));
    add(eax, imm32(d8*4));
    mov(dis(rbx), eax);
    call0(inDelaySlot);
    test(rax, rax);
    jnz(imm8(+7));
    mov(eax, dis(rbx));
    sub(eax, imm8(2));
    mov(dis(rbx), eax);
    return 0;
  }

  //TST #imm,R0
  case 0xc8: {
    mov(eax, dis(rbx));
    and(eax, imm32(i));
    mov(rax, imm64(&SR.T));
    setz(dis(rax));
    return 0;
  }

  //AND #imm,R0
  case 0xc9: {
    mov(eax, dis(rbx));
    and(eax, imm32(i));
    mov(dis(rbx), eax);
    return 0;
  }

  //XOR #imm,R0
  case 0xca: {
    mov(eax, dis(rbx));
    xor(eax, imm32(i));
    mov(dis(rbx), eax);
    return 0;
  }

  //OR #imm,R0
  case 0xcb: {
    mov(eax, dis(rbx));
    or(eax, imm32(i));
    mov(dis(rbx), eax);
    return 0;
  }

  //TST.B #imm,@(R0,GBR)
  case 0xcc: {
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call1(readByte, rax);
    cmp(al, imm8(i));
    mov(rax, imm64(&SR.T));
    setz(dis{rax});
    return 0;
  }

  //AND.B #imm,@(R0,GBR)
  case 0xcd: {
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call1(readByte, rax);
    and(al, imm8(i));
    mov(edx, eax);
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call2(writeByte, rax, rdx);
    return 0;
  }

  //XOR.B #imm,@(R0,GBR)
  case 0xce: {
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call1(readByte, rax);
    xor(al, imm8(i));
    mov(edx, eax);
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call2(writeByte, rax, rdx);
    return 0;
  }

  //OR.B #imm,@(R0,GBR)
  case 0xcf: {
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call1(readByte, rax);
    or(al, imm8(i));
    mov(edx, eax);
    mov(eax, mem64(&GBR));
    add(eax, dis(rbx));
    call2(writeByte, rax, rdx);
    return 0;
  }

  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8

  #define n (opcode >> 8 & 0xf)*4
  #define m (opcode >> 8 & 0xf)*4
  switch(opcode >> 4 & 0x0f00 | opcode & 0x00ff) {

  //MOVT Rn
  case 0x029: {
    mov(al, mem64(&SR.T));
    movzx(eax, al);
    mov(dis8(rbx,n), eax);
    return 0;
  }

  //CMP/PZ Rn
  case 0x411: {
    mov(eax, dis8(rbx,n));
    cmp(eax, imm8(0));
    mov(rax, imm64(&SR.T));
    setge(dis(rax));
    return 0;
  }

  //CMP/PL Rn
  case 0x415: {
    mov(eax, dis8(rbx,n));
    cmp(eax, imm8(0));
    mov(rax, imm64(&SR.T));
    setg(dis(rax));
    return 0;
  }

  }
  #undef n
  #undef m

  switch(opcode) {

  //CLRT
  case 0x0008: {
    mov(al, imm8(0));
    mov(mem64(&SR.T), al);
    return 0;
  }

  //NOP
  case 0x0009: {
    return 0;
  }

  //CLRMAC
  case 0x0028: {
    xor(rax, rax);
    mov(mem64(&MAC), rax);
    return 0;
  }

  }

  #undef PC
  #undef SR
  #undef GBR
  #undef MAC
#endif

  #define op(id, name, ...) \
    case id: \
      call(&SH2::name, &self, ##__VA_ARGS__); \
      return 0;
  #define br(id, name, ...) \
    case id: \
      call(&SH2::name, &self, ##__VA_ARGS__); \
      return 1;
  #include "decoder.hpp"
  #undef op
  #undef br
  return 0;
}

#undef call0
#undef call1
#undef call2
