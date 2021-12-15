#define Reg(r)  mem(sreg(1), offsetof(Registers, r))
#define RegR(i) mem(sreg(1), offsetof(Registers, R) + (i) * sizeof(u32))
#define R0      Reg(R[0])
#define R15     Reg(R[15])
#define PC      Reg(PC)
#define PR      Reg(PR)
#define GBR     Reg(GBR)
#define VBR     Reg(VBR)
#define MAC     Reg(MAC)
#define MACL    Reg(MACL)
#define MACH    Reg(MACH)
#define CCR     Reg(CCR)
#define T       Reg(SR.T)
#define S       Reg(SR.S)
#define I       Reg(SR.I)
#define Q       Reg(SR.Q)
#define M       Reg(SR.M)
#define PPC     Reg(PPC)
#define PPM     Reg(PPM)
#define ET      Reg(ET)
#define ID      Reg(ID)

auto SH2::Recompiler::invalidate(u32 address) -> void {
  auto pool = pools[address >> 8 & 0xffffff];
  if(!pool) return;
  memory::jitprotect(false);
  pool->blocks[address >> 1 & 0x7f] = nullptr;
  memory::jitprotect(true);
}

auto SH2::Recompiler::pool(u32 address) -> Pool* {
  auto& pool = pools[address >> 8 & 0xffffff];
  if(!pool) pool = (Pool*)allocator.acquire(sizeof(Pool));
  return pool;
}

auto SH2::Recompiler::block(u32 address) -> Block* {
  if(auto block = pool(address)->blocks[address >> 1 & 0x7f]) return block;
  auto block = emit(address);
  pool(address)->blocks[address >> 1 & 0x7f] = block;
  memory::jitprotect(true);
  return block;
}

auto SH2::Recompiler::emit(u32 address) -> Block* {
  if(unlikely(allocator.available() < 1_MiB)) {
    print("SH2 allocator flush\n");
    memory::jitprotect(false);
    allocator.release(bump_allocator::zero_fill);
    memory::jitprotect(true);
    reset();
  }

  auto block = (Block*)allocator.acquire(sizeof(Block));
  beginFunction(2);

  bool hasBranched = 0;
  while(true) {
    u16 instruction = self.readWord(address);
    bool branched = emitInstruction(instruction);
    add64(CCR, CCR, imm(1));
  //add64(CCR, CCR, imm(2));  //underclocking hack
    call(&SH2::instructionEpilogue);
    address += 2;
    if(hasBranched || (address & 0xfe) == 0) break;  //block boundary
    hasBranched = branched;
    testJumpEpilog();
  }
  jumpEpilog();

  memory::jitprotect(false);
  block->code = endFunction();

  return block;
}

#define readByte  &SH2::readByte
#define readWord  &SH2::readWord
#define readLong  &SH2::readLong
#define writeByte &SH2::writeByte
#define writeWord &SH2::writeWord
#define writeLong &SH2::writeLong
#define illegal   &SH2::illegalInstruction

auto SH2::Recompiler::emitInstruction(u16 opcode) -> bool {
  #define n   (opcode >> 8 & 0x00f)
  #define m   (opcode >> 4 & 0x00f)
  #define i   (opcode >> 0 & 0x0ff)
  #define d4  (opcode >> 0 & 0x00f)
  #define d8  (opcode >> 0 & 0x0ff)
  #define d12 (opcode >> 0 & 0xfff)
  #define Rn  RegR(n)
  #define Rm  RegR(m)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {

  //MOV.B Rm,@(R0,Rn)
  case 0x04: {
    add32(reg(1), Rn, R0);
    mov32(reg(2), Rm);
    call(writeByte);
    return 0;
  }

  //MOV.W Rm,@(R0,Rn)
  case 0x05: {
    add32(reg(1), Rn, R0);
    mov32(reg(2), Rm);
    call(writeWord);
    return 0;
  }

  //MOV.L Rm,@(R0,Rn)
  case 0x06: {
    add32(reg(1), Rn, R0);
    mov32(reg(2), Rm);
    call(writeLong);
    return 0;
  }

  //MUL.L Rm,Rn
  case 0x07: {
    mul32(MACL, Rn, Rm);
    return 0;
  }

  //MOV.B @(R0,Rm),Rn
  case 0x0c: {
    add32(reg(1), Rm, R0);
    call(readByte);
    mov32_s8(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.W @(R0,Rm),Rn
  case 0x0d: {
    add32(reg(1), Rm, R0);
    call(readWord);
    mov32_s16(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.L @(R0,Rm),Rn
  case 0x0e: {
    add32(reg(1), Rm, R0);
    call(readLong);
    mov32(Rn, reg(0));
    return 0;
  }

  //MAC.L @Rm+,@Rn+
  case 0x0f: {
    mov32(reg(1), Rn);
    add32(Rn, reg(1), imm(4));
    call(readLong);
    mov32(sreg(2), reg(0));
    mov32(reg(1), Rm);
    add32(Rm, reg(1), imm(4));
    call(readLong);
    mov64_s32(reg(0), reg(0));
    mov64_s32(reg(1), sreg(2));
    mul64(reg(0), reg(0), reg(1));
    add64(reg(0), reg(0), MAC);
    auto skip = cmp32_jump(S, imm(0), flag_eq);
    shl64(reg(0), reg(0), imm(16));
    ashr64(reg(0), reg(0), imm(16));
    setLabel(skip);
    mov64(MAC, reg(0));
    return 0;
  }

  //MOV.L Rm,@(disp,Rn)
  case 0x10 ... 0x1f: {
    add32(reg(1), Rn, imm(d4*4));
    mov32(reg(2), Rm);
    call(writeLong);
    return 0;
  }

  //MOV.B Rm,@Rn
  case 0x20: {
    mov32(reg(1), Rn);
    mov32(reg(2), Rm);
    call(writeByte);
    return 0;
  }

  //MOV.W Rm,@Rn
  case 0x21: {
    mov32(reg(1), Rn);
    mov32(reg(2), Rm);
    call(writeWord);
    return 0;
  }

  //MOV.L Rm,@Rn
  case 0x22: {
    mov32(reg(1), Rn);
    mov32(reg(2), Rm);
    call(writeLong);
    return 0;
  }

  //MOV.B Rm,@-Rn
  case 0x24: {
    mov32(reg(2), Rm);
    sub32(reg(1), Rn, imm(1));
    mov32(Rn, reg(1));
    call(writeByte);
    return 0;
  }

  //MOV.W Rm,@-Rn
  case 0x25: {
    mov32(reg(2), Rm);
    sub32(reg(1), Rn, imm(2));
    mov32(Rn, reg(1));
    call(writeWord);
    return 0;
  }

  //MOV.L Rm,@-Rn
  case 0x26: {
    mov32(reg(2), Rm);
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    call(writeLong);
    return 0;
  }

  //DIV0S Rm,Rn
  case 0x27: {
    lshr32(reg(0), Rn, imm(31));
    mov32(Q, reg(0));
    lshr32(reg(1), Rm, imm(31));
    mov32(M, reg(1));
    xor32(reg(0), reg(0), reg(1));
    mov32(T, reg(0));
    return 0;
  }

  //TST Rm,Rn
  case 0x28: {
    test32(Rn, Rm, set_z);
    mov32_f(T, flag_z);
    return 0;
  }

  //AND Rm,Rn
  case 0x29: {
    and32(Rn, Rn, Rm);
    return 0;
  }

  //XOR Rm,Rn
  case 0x2a: {
    xor32(Rn, Rn, Rm);
    return 0;
  }

  //OR Rm,Rn
  case 0x2b: {
    or32(Rn, Rn, Rm);
    return 0;
  }

  //CMP/STR Rm,Rn
  case 0x2c: {
    xor32(reg(0), Rn, Rm);
    xor32(reg(1), reg(1), reg(1));
    for(auto b : range(4)) {
      test32(reg(0), imm(0xff << b * 8), set_z);
      or32_f(reg(1), flag_z);
    }
    mov32(T, reg(1));
    return 0;
  }

  //XTRCT Rm,Rn
  case 0x2d: {
    lshr32(reg(0), Rn, imm(16));
    shl32(reg(1), Rm, imm(16));
    or32(reg(0), reg(0), reg(1));
    mov32(Rn, reg(0));
    return 0;
  }

  //MULU Rm,Rn
  case 0x2e: {
    mov32(reg(0), Rn);
    mov32(reg(1), Rm);
    mov32_u16(reg(0), reg(0));
    mov32_u16(reg(1), reg(1));
    mul32(reg(0), reg(0), reg(1));
    mov32(MACL, reg(0));
    return 0;
  }

  //MULS Rm,Rn
  case 0x2f: {
    mov32(reg(0), Rn);
    mov32(reg(1), Rm);
    mov32_s16(reg(0), reg(0));
    mov32_s16(reg(1), reg(1));
    mul32(reg(0), reg(0), reg(1));
    mov32(MACL, reg(0));
    return 0;
  }

  //CMP/EQ Rm,Rn
  case 0x30: {
    cmp32(Rn, Rm, set_z);
    mov32_f(T, flag_z);
    return 0;
  }

  //CMP/HS Rm,Rn
  case 0x32: {
    cmp32(Rn, Rm, set_uge);
    mov32_f(T, flag_uge);
    return 0;
  }

  //CMP/GE Rm,Rn
  case 0x33: {
    cmp32(Rn, Rm, set_sge);
    mov32_f(T, flag_sge);
    return 0;
  }

  //DIV1 Rm,Rn
  case 0x34: {
    mov32(reg(0), Q);
    lshr32(Q, Rn, imm(31));
    shl32(reg(1), Rn, imm(1));
    or32(reg(1), reg(1), T);
    auto skip = cmp32_jump(M, reg(0), flag_ne);
    sub32(reg(1), reg(1), Rm, set_c);
    mov32(Rn, reg(1));
    mov32_from_c(reg(1), -1);
    auto skip2 = jump();
    setLabel(skip);
    add32(reg(1), reg(1), Rm, set_c);
    mov32(Rn, reg(1));
    mov32_from_c(reg(1), +1);
    setLabel(skip2);
    xor32(reg(0), M, Q);
    xor32(reg(0), reg(0), reg(1));
    mov32(Q, reg(0));
    xor32(reg(1), Q, M);
    mov32(reg(0), imm(1));
    sub32(reg(0), reg(0), reg(1));
    mov32(T, reg(0));
    return 0;
  }

  //DMULU.L Rm,Rn
  case 0x35: {
    mov64_u32(reg(0), Rn);
    mov64_u32(reg(1), Rm);
    mul64(reg(0), reg(0), reg(1));
    mov64(MAC, reg(0));
    return 0;
  }

  //CMP/HI Rm,Rn
  case 0x36: {
    cmp32(Rn, Rm, set_ugt);
    mov32_f(T, flag_ugt);
    return 0;
  }

  //CMP/GT Rm,Rn
  case 0x37: {
    cmp32(Rn, Rm, set_sgt);
    mov32_f(T, flag_sgt);
    return 0;
  }

  //SUB Rm,Rn
  case 0x38: {
    sub32(Rn, Rn, Rm);
    return 0;
  }

  //SUBC Rm,Rn
  case 0x3a: {
    mov32_to_c(T, -1);
    subc32(Rn, Rn, Rm, set_c);
    mov32_from_c(reg(0), -1);
    mov32(T, reg(0));
    return 0;
  }

  //SUBV Rm,Rn
  case 0x3b: {
    sub32(Rn, Rn, Rm, set_o);
    mov32_f(T, flag_o);
    return 0;
  }

  //ADD Rm,Rn
  case 0x3c: {
    add32(Rn, Rn, Rm);
    return 0;
  }

  //DMULS.L Rm,Rn
  case 0x3d: {
    mov64_s32(reg(0), Rn);
    mov64_s32(reg(1), Rm);
    mul64(reg(0), reg(0), reg(1));
    mov64(MAC, reg(0));
    return 0;
  }

  //ADDC Rm,Rn
  case 0x3e: {
    mov32_to_c(T, +1);
    addc32(Rn, Rn, Rm, set_c);
    mov32_from_c(reg(0), +1);
    mov32(T, reg(0));
    return 0;
  }

  //ADDV Rm,Rn
  case 0x3f: {
    add32(Rn, Rn, Rm, set_o);
    mov32_f(T, flag_o);
    return 0;
  }

  //MAC.W @Rm+,@Rn+
  case 0x4f: {
    mov32(reg(1), Rn);
    add32(Rn, reg(1), imm(2));
    call(readWord);
    mov32(sreg(2), reg(0));
    mov32(reg(1), Rm);
    add32(Rm, reg(1), imm(2));
    call(readWord);
    mov64_s16(reg(0), reg(0));
    mov64_s16(reg(1), sreg(2));
    mul64(reg(0), reg(0), reg(1));
    add64(reg(0), reg(0), MAC);
    auto skip = cmp32_jump(S, imm(0), flag_eq);
    mov64_s32(reg(0), reg(0));
    setLabel(skip);
    mov64(MAC, reg(0));
    return 0;
  }

  //MOV.L @(disp,Rm),Rn
  case 0x50 ... 0x5f: {
    add32(reg(1), Rm, imm(d4*4));
    call(readLong);
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.B @Rm,Rn
  case 0x60: {
    mov32(reg(1), Rm);
    call(readByte);
    mov32_s8(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.W @Rm,Rn
  case 0x61: {
    mov32(reg(1), Rm);
    call(readWord);
    mov32_s16(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.L @Rm,Rn
  case 0x62: {
    mov32(reg(1), Rm);
    call(readLong);
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV Rm,Rn
  case 0x63: {
    mov32(Rn, Rm);
    return 0;
  }

  //MOV.B @Rm+,Rn
  case 0x64: {
    mov32(reg(1), Rm);
    if(n != m) {
      add32(Rm, reg(1), imm(1));
    }
    call(readByte);
    mov32_s8(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.W @Rm+,Rn
  case 0x65: {
    mov32(reg(1), Rm);
    if(n != m) {
      add32(Rm, reg(1), imm(2));
    }
    call(readWord);
    mov32_s16(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV.L @Rm+,Rn
  case 0x66: {
    mov32(reg(1), Rm);
    if (n != m) {
      add32(Rm, reg(1), imm(4));
    }
    call(readLong);
    mov32(Rn, reg(0));
    return 0;
  }

  //NOT Rm,Rn
  case 0x67: {
    not32(Rn, Rm);
    return 0;
  }

  //SWAP.B Rm,Rn
  case 0x68: {
    mov32(reg(0), Rm);
    lshr32(reg(1), reg(0), imm(8));
    mov32_u8(reg(1), reg(1));
    mov32_u8(reg(2), reg(0));
    shl32(reg(2), reg(2), imm(8));
    and32(reg(0), reg(0), imm(~0xffff));
    or32(reg(0), reg(0), reg(1));
    or32(reg(0), reg(0), reg(2));
    mov32(Rn, reg(0));
    return 0;
  }

  //SWAP.W Rm,Rn
  case 0x69: {
    mov32(reg(0), Rm);
    lshr32(reg(1), reg(0), imm(16));
    shl32(reg(0), reg(0), imm(16));
    or32(reg(0), reg(0), reg(1));
    mov32(Rn, reg(0));
    return 0;
  }

  //NEGC Rm,Rn
  case 0x6a: {
    mov32_to_c(T, -1);
    subc32(Rn, imm(0), Rm, set_c);
    mov32_from_c(reg(0), -1);
    mov32(T, reg(0));
    return 0;
  }

  //NEG Rm,Rn
  case 0x6b: {
    neg32(Rn, Rm);
    return 0;
  }

  //EXTU.B Rm,Rn
  case 0x6c: {
    mov32(reg(0), Rm);
    mov32_u8(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //EXTU.W Rm,Rn
  case 0x6d: {
    mov32(reg(0), Rm);
    mov32_u16(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //EXTS.B Rm,Rn
  case 0x6e: {
    mov32(reg(0), Rm);
    mov32_s8(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //EXTS.W Rm,Rn
  case 0x6f: {
    mov32(reg(0), Rm);
    mov32_s16(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //ADD #imm,Rn
  case 0x70 ... 0x7f: {
    add32(Rn, Rn, imm((s8)i));
    return 0;
  }

  //MOV.W @(disp,PC),Rn
  case 0x90 ... 0x9f: {
    add32(reg(1), PC, imm(d8*2));
    call(readWord);
    mov32_s16(reg(0), reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //BRA disp
  case 0xa0 ... 0xaf: {
    add32(PPC, PC, imm(4 + (i12)d12 * 2));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //BSR disp
  case 0xb0 ... 0xbf: {
    mov32(reg(0), PC);
    mov32(PR, reg(0));
    add32(reg(0), reg(0), imm(4 + (i12)d12 * 2));
    mov32(PPC, reg(0));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //MOV.L @(disp,PC),Rn
  case 0xd0 ... 0xdf: {
    and32(reg(1), PC, imm(~3));
    add32(reg(1), reg(1), imm(d8*4));
    call(readLong);
    mov32(Rn, reg(0));
    return 0;
  }

  //MOV #imm,Rn
  case 0xe0 ... 0xef: {
    mov32(Rn, imm((s8)i));
    return 0;
  }

  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8
  #undef d12
  #undef Rn
  #undef Rm

  #define n  (opcode >> 8 & 0x0f)
  #define m  (opcode >> 4 & 0x0f)  //n for 0x80,0x81,0x84,0x85
  #define i  (opcode >> 0 & 0xff)
  #define d4 (opcode >> 0 & 0x0f)
  #define d8 (opcode >> 0 & 0xff)
  #define Rn RegR(n)
  #define Rm RegR(m)
  switch(opcode >> 8) {

  //MOV.B R0,@(disp,Rn)
  case 0x80: {
    add32(reg(1), Rm, imm(d4));
    mov32(reg(2), R0);
    call(writeByte);
    return 0;
  }

  //MOV.W R0,@(disp,Rn)
  case 0x81: {
    add32(reg(1), Rm, imm(d4*2));
    mov32(reg(2), R0);
    call(writeWord);
    return 0;
  }

  //MOV.B @(disp,Rn),R0
  case 0x84: {
    add32(reg(1), Rm, imm(d4));
    call(readByte);
    mov32_s8(reg(0), reg(0));
    mov32(R0, reg(0));
    return 0;
  }

  //MOV.W @(disp,Rn),R0
  case 0x85: {
    add32(reg(1), Rm, imm(d4*2));
    call(readWord);
    mov32_s16(reg(0), reg(0));
    mov32(R0, reg(0));
    return 0;
  }

  //CMP/EQ #imm,R0
  case 0x88: {
    cmp32(R0, imm((s8)i), set_z);
    mov32_f(T, flag_z);
    return 0;
  }

  //BT disp
  case 0x89: {
    auto skip = cmp32_jump(T, imm(0), flag_eq);
    add32(PPC, PC, imm(4 + (s8)d8 * 2));
    mov32(PPM, imm(Branch::Take));
    setLabel(skip);
    return 1;
  }

  //BF disp
  case 0x8b: {
    auto skip = cmp32_jump(T, imm(0), flag_ne);
    add32(PPC, PC, imm(4 + (s8)d8 * 2));
    mov32(PPM, imm(Branch::Take));
    setLabel(skip);
    return 1;
  }

  //BT/S disp
  case 0x8d: {
    auto skip = cmp32_jump(T, imm(0), flag_eq);
    add32(PPC, PC, imm(4 + (s8)d8 * 2));
    mov32(PPM, imm(Branch::Slot));
    setLabel(skip);
    return 1;
  }

  //BF/S disp
  case 0x8f: {
    auto skip = cmp32_jump(T, imm(0), flag_ne);
    add32(PPC, PC, imm(4 + (s8)d8 * 2));
    mov32(PPM, imm(Branch::Slot));
    setLabel(skip);
    return 1;
  }

  //MOV.B R0,@(disp,GBR)
  case 0xc0: {
    add32(reg(1), GBR, imm(d8));
    mov32(reg(2), R0);
    call(writeByte);
    return 0;
  }

  //MOV.W R0,@(disp,GBR)
  case 0xc1: {
    add32(reg(1), GBR, imm(d8*2));
    mov32(reg(2), R0);
    call(writeWord);
    return 0;
  }

  //MOV.L R0,@(disp,GBR)
  case 0xc2: {
    add32(reg(1), GBR, imm(d8*4));
    mov32(reg(2), R0);
    call(writeLong);
    return 0;
  }

  //TRAPA #imm
  case 0xc3: {
    getSR(reg(2));
    sub32(reg(1), R15, imm(4));
    mov32(R15, reg(1));
    call(writeLong);
    add32(reg(2), PC, imm(2));
    sub32(reg(1), R15, imm(4));
    mov32(R15, reg(1));
    call(writeLong);
    add32(reg(1), VBR, imm(i * 4));
    call(readLong);
    add32(reg(0), reg(0), imm(4));
    mov32(PPC, reg(0));
    mov32(PPM, imm(Branch::Take));
    return 1;
  }

  //MOV.B @(disp,GBR),R0
  case 0xc4: {
    add32(reg(1), GBR, imm(d8));
    call(readByte);
    mov32_s8(reg(0), reg(0));
    mov32(R0, reg(0));
    return 0;
  }

  //MOV.W @(disp,GBR),R0
  case 0xc5: {
    add32(reg(1), GBR, imm(d8*2));
    call(readWord);
    mov32_s16(reg(0), reg(0));
    mov32(R0, reg(0));
    return 0;
  }

  //MOV.L @(disp,GBR),R0
  case 0xc6: {
    add32(reg(1), GBR, imm(d8*4));
    call(readLong);
    mov32(R0, reg(0));
    return 0;
  }

  //MOVA @(disp,PC),R0
  case 0xc7: {
    and32(reg(0), PC, imm(~3));
    add32(reg(0), reg(0), imm(d8*4));
    mov32(R0, reg(0));
    auto skip = cmp32_jump(PPM, imm(0), flag_eq);
    sub32(R0, R0, imm(2));
    setLabel(skip);
    return 0;
  }

  //TST #imm,R0
  case 0xc8: {
    test32(R0, imm(i), set_z);
    mov32_f(T, flag_z);
    return 0;
  }

  //AND #imm,R0
  case 0xc9: {
    and32(R0, R0, imm(i));
    return 0;
  }

  //XOR #imm,R0
  case 0xca: {
    xor32(R0, R0, imm(i));
    return 0;
  }

  //OR #imm,R0
  case 0xcb: {
    or32(R0, R0, imm(i));
    return 0;
  }

  //TST.B #imm,@(R0,GBR)
  case 0xcc: {
    mov32(reg(1), GBR);
    add32(reg(1), reg(1), R0);
    call(readByte);
    test32(reg(0), imm(i), set_z);
    mov32_f(T, flag_z);
    return 0;
  }

  //AND.B #imm,@(R0,GBR)
  case 0xcd: {
    add32(reg(1), GBR, R0);
    call(readByte);
    and32(reg(2), reg(0), imm(i));
    add32(reg(1), GBR, R0);
    call(writeByte);
    return 0;
  }

  //XOR.B #imm,@(R0,GBR)
  case 0xce: {
    add32(reg(1), GBR, R0);
    call(readByte);
    xor32(reg(2), reg(0), imm(i));
    add32(reg(1), GBR, R0);
    call(writeByte);
    return 0;
  }

  //OR.B #imm,@(R0,GBR)
  case 0xcf: {
    add32(reg(1), GBR, R0);
    call(readByte);
    or32(reg(2), reg(0), imm(i));
    add32(reg(1), GBR, R0);
    call(writeByte);
    return 0;
  }

  }
  #undef n
  #undef m
  #undef i
  #undef d4
  #undef d8
  #undef Rn
  #undef Rm

  #define n  (opcode >> 8 & 0xf)
  #define m  (opcode >> 8 & 0xf)
  #define Rn RegR(n)
  #define Rm RegR(m)
  switch(opcode >> 4 & 0x0f00 | opcode & 0x00ff) {

  //STC SR,Rn
  case 0x002: {
    getSR(reg(0));
    mov32(Rn, reg(0));
    return 0;
  }

  //BSRF Rm
  case 0x003: {
    mov32(reg(0), PC);
    mov32(PR, reg(0));
    add32(reg(0), reg(0), Rm);
    add32(reg(0), reg(0), imm(4));
    mov32(PPC, reg(0));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //STS MACH,Rn
  case 0x00a: {
    mov32(Rn, MACH);
    return 0;
  }

  //STC GBR,Rn
  case 0x012: {
    mov32(Rn, GBR);
    return 0;
  }

  //STS MACL,Rn
  case 0x01a: {
    mov32(Rn, MACL);
    return 0;
  }

  //STC VBR,Rn
  case 0x022: {
    mov32(Rn, VBR);
    return 0;
  }

  //BRAF Rm
  case 0x023: {
    add32(reg(0), PC, Rm);
    add32(reg(0), reg(0), imm(4));
    mov32(PPC, reg(0));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //MOVT Rn
  case 0x029: {
    mov32(Rn, T);
    return 0;
  }

  //STS PR,Rn
  case 0x02a: {
    mov32(Rn, PR);
    return 0;
  }

  //SHLL Rn
  case 0x400: {
    lshr32(T, Rn, imm(31));
    shl32(Rn, Rn, imm(1));
    return 0;
  }

  //SHLR Rn
  case 0x401: {
    and32(T, Rn, imm(1));
    lshr32(Rn, Rn, imm(1));
    return 0;
  }

  //STS.L MACH,@-Rn
  case 0x402: {
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    mov32(reg(2), MACH);
    call(writeLong);
    return 0;
  }

  //STC.L SR,@-Rn
  case 0x403: {
    getSR(reg(2));
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    call(writeLong);
    return 0;
  }

  //ROTL Rn
  case 0x404: {
    lshr32(T, Rn, imm(31));
    mov32(reg(0), Rn);
    lshr32(reg(1), reg(0), imm(31));
    shl32(reg(0), reg(0), imm(1));
    or32(reg(0), reg(0), reg(1));
    mov32(Rn, reg(0));
    return 0;
  }

  //ROTR Rn
  case 0x405: {
    and32(T, Rn, imm(1));
    mov32(reg(0), Rn);
    lshr32(reg(1), reg(0), imm(1));
    shl32(reg(0), reg(0), imm(31));
    or32(reg(0), reg(0), reg(1));
    mov32(Rn, reg(0));
    return 0;
  }

  //LDS.L @Rm+,MACH
  case 0x406: {
    mov32(reg(1), Rn);
    add32(Rn, reg(1), imm(4));
    call(readLong);
    mov32(MACH, reg(0));
    return 0;
  }

  //LDC.L @Rm+,SR
  case 0x407: {
    mov32(reg(1), Rn);
    add32(Rn, reg(1), imm(4));
    call(readLong);
    setSR(reg(0));
    return 0;
  }

  //SHLL2 Rn
  case 0x408: {
    shl32(Rn, Rn, imm(2));
    return 0;
  }

  //SHLR2 Rn
  case 0x409: {
    lshr32(Rn, Rn, imm(2));
    return 0;
  }

  //LDS Rm,MACH
  case 0x40a: {
    mov32(MACH, Rm);
    return 0;
  }

  //JSR @Rm
  case 0x40b: {
    mov32(PR, PC);
    add32(PPC, Rm, imm(4));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //LDC Rm,SR
  case 0x40e: {
    mov32(reg(0), Rm);
    setSR(reg(0));
    return 0;
  }

  //DT Rn
  case 0x410: {
    sub32(Rn, Rn, imm(1), set_z);
    mov32_f(T, flag_z);
    return 0;
  }

  //CMP/PZ Rn
  case 0x411: {
    cmp32(Rn, imm(0), set_sge);
    mov32_f(T, flag_sge);
    return 0;
  }

  //STS.L MACL,@-Rn
  case 0x412: {
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    mov32(reg(2), MACL);
    call(writeLong);
    return 0;
  }

  //STC.L GBR,@-Rn
  case 0x413: {
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    mov32(reg(2), GBR);
    call(writeLong);
    return 0;
  }

  //CMP/PL Rn
  case 0x415: {
    cmp32(Rn, imm(0), set_sgt);
    mov32_f(T, flag_sgt);
    return 0;
  }

  //LDS.L @Rm+,MACL
  case 0x416: {
    mov32(reg(1), Rm);
    add32(Rm, reg(1), imm(4));
    call(readLong);
    mov32(MACL, reg(0));
    return 0;
  }

  //LDS.L @Rm+,GBR
  case 0x417: {
    mov32(reg(1), Rm);
    add32(Rm, reg(1), imm(4));
    call(readLong);
    mov32(GBR, reg(0));
    return 0;
  }

  //SHLL8 Rn
  case 0x418: {
    shl32(Rn, Rn, imm(8));
    return 0;
  }

  //SHLR8 Rn
  case 0x419: {
    lshr32(Rn, Rn, imm(8));
    return 0;
  }

  //LDS Rm,MACL
  case 0x41a: {
    mov32(MACL, Rm);
    return 0;
  }

  //TAS @Rn
  case 0x41b: {
    and32(reg(1), Rn, imm(0x1fff'ffff));
    or32(reg(1), reg(1), imm(0x2000'0000));
    call(readByte);
    test32(reg(0), reg(0), set_z);
    mov32_f(T, flag_z);
    or32(reg(0), reg(0), imm(0x80));
    mov32(reg(2), reg(0));
    and32(reg(1), Rn, imm(0x1fff'ffff));
    or32(reg(1), reg(1), imm(0x2000'0000));
    call(writeByte);
    return 0;
  }

  //LDC Rm,GBR
  case 0x41e: {
    mov32(GBR, Rn);
    return 0;
  }

  //SHAL Rn
  case 0x420: {
    lshr32(T, Rn, imm(31));
    shl32(Rn, Rn, imm(1));
    return 0;
  }

  //SHAR Rn
  case 0x421: {
    and32(T, Rn, imm(1));
    ashr32(Rn, Rn, imm(1));
    return 0;
  }

  //STS.L PR,@-Rn
  case 0x422: {
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    mov32(reg(2), PR);
    call(writeLong);
    return 0;
  }

  //STC.L VBR,@-Rn
  case 0x423: {
    sub32(reg(1), Rn, imm(4));
    mov32(Rn, reg(1));
    mov32(reg(2), VBR);
    call(writeLong);
    return 0;
  }

  //ROTCL Rn
  case 0x424: {
    mov32(reg(0), Rn);
    lshr32(reg(1), reg(0), imm(31));
    shl32(reg(0), reg(0), imm(1));
    or32(reg(0), reg(0), T);
    mov32(Rn, reg(0));
    mov32(T, reg(1));
    return 0;
  }

  //ROTCR Rn
  case 0x425: {
    mov32(reg(0), Rn);
    and32(reg(1), reg(0), imm(1));
    lshr32(reg(0), reg(0), imm(1));
    shl32(reg(2), T, imm(31));
    or32(reg(0), reg(0), reg(2));
    mov32(Rn, reg(0));
    mov32(T, reg(1));
    return 0;
  }

  //LDS.L @Rm+,PR
  case 0x426: {
    mov32(reg(1), Rm);
    add32(Rm, reg(1), imm(4));
    call(readLong);
    mov32(PR, reg(0));
    return 0;
  }

  //LDC.L @Rm+,VBR
  case 0x427: {
    mov32(reg(1), Rm);
    add32(Rm, reg(1), imm(4));
    call(readLong);
    mov32(VBR, reg(0));
    return 0;
  }

  //SHLL16 Rn
  case 0x428: {
    shl32(Rn, Rn, imm(16));
    return 0;
  }

  //SHLR16 Rn
  case 0x429: {
    lshr32(Rn, Rn, imm(16));
    return 0;
  }

  //LDS Rm,PR
  case 0x42a: {
    mov32(PR, Rm);
    return 0;
  }

  //JMP @Rm
  case 0x42b: {
    add32(PPC, Rm, imm(4));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //LDC Rm,VBR
  case 0x42e: {
    mov32(VBR, Rm);
    return 0;
  }

  }
  #undef n
  #undef m
  #undef Rn
  #undef Rm

  switch(opcode) {

  //CLRT
  case 0x0008: {
    xor32(reg(0), reg(0), reg(0));
    mov32(T, reg(0));
    return 0;
  }

  //NOP
  case 0x0009: {
    return 0;
  }

  //RTS
  case 0x000b: {
    add32(PPC, PR, imm(4));
    mov32(PPM, imm(Branch::Slot));
    return 1;
  }

  //SETT
  case 0x0018: {
    mov32(T, imm(1));
    return 0;
  }

  //DIV0U
  case 0x0019: {
    xor32(reg(0), reg(0), reg(0));
    mov32(Q, reg(0));
    mov32(M, reg(0));
    mov32(T, reg(0));
    return 0;
  }

  //SLEEP
  case 0x001b: {
    auto skip = cmp32_jump(ET, imm(0), flag_ne);
    sub32(PC, PC, imm(2));
    auto skip2 = jump();
    setLabel(skip);
    mov32(ET, imm(0));
    setLabel(skip2);
    return 1;
  }

  //CLRMAC
  case 0x0028: {
    xor64(reg(0), reg(0), reg(0));
    mov64(MAC, reg(0));
    return 0;
  }

  //RTE
  case 0x002b: {
    mov32(reg(1), R15);
    add32(R15, reg(1), imm(4));
    call(readLong);
    mov32(PPC, reg(0));
    mov32(PPM, imm(Branch::Slot));
    mov32(reg(1), R15);
    add32(R15, reg(1), imm(4));
    call(readLong);
    setSR(reg(0));
    return 1;
  }

  }

  switch(0) {

  //ILLEGAL
  case 0: {
    call(illegal);
    return 1;
  }

  }

#if 0
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
#endif

  return 0;
}

auto SH2::Recompiler::getSR(reg dst) -> void {
  mov32(dst, M);
  shl32(dst, dst, imm(1));
  or32(dst, dst, Q);
  shl32(dst, dst, imm(4));
  or32(dst, dst, I);
  shl32(dst, dst, imm(3));
  or32(dst, dst, S);
  shl32(dst, dst, imm(1));
  or32(dst, dst, T);
}

auto SH2::Recompiler::setSR(reg src) -> void {
  and32(T, src, imm(1));
  lshr32(src, src, imm(1));
  and32(S, src, imm(1));
  lshr32(src, src, imm(3));
  and32(I, src, imm(15));
  lshr32(src, src, imm(4));
  and32(Q, src, imm(1));
  lshr32(src, src, imm(1));
  and32(M, src, imm(1));
}

#undef Reg
#undef RegR
#undef R0
#undef R15
#undef PC
#undef PR
#undef GBR
#undef VBR
#undef MAC
#undef MACL
#undef MACH
#undef CCR
#undef T
#undef S
#undef I
#undef Q
#undef M
#undef PPC
#undef PPM
#undef ET
#undef ID

#undef readByte
#undef readWord
#undef readLong
#undef writeByte
#undef writeWord
#undef writeLong
#undef illegal
