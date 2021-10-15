#define R0   dis (rbx)
#define R15  dis8(rbx,15*4)
#define PC   dis8(rbx,16*4)
#define PR   dis8(rbx,17*4)
#define GBR  dis8(rbx,18*4)
#define VBR  dis8(rbx,19*4)
#define MAC  dis8(rbx,20*4)
#define MACL dis8(rbx,20*4)
#define MACH dis8(rbx,21*4)
#define CCR  dis8(rbx,22*4)
#define T    dis8(rbx,23*4)
#define S    dis8(rbx,24*4)
#define I    dis8(rbx,25*4)
#define Q    dis8(rbx,26*4)
#define M    dis8(rbx,27*4)
#define PPC  dis8(rbx,28*4)
#define PPM  dis8(rbx,29*4)
#define ET   dis8(rbx,30*4)
#define ID   dis8(rbx,31*4)

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

  bool hasBranched = 0;
  while(true) {
    u16 instruction = self.readWord(address);
    bool branched = emitInstruction(instruction);
    incd(CCR);
  //addd(CCR, imm8(2));  //underclocking hack
    call(&SH2::instructionEpilogue);
    address += 2;
    if(hasBranched || (address & 0xfe) == 0) break;  //block boundary
    hasBranched = branched;
    test(al, al);
    jnz(epilogue);
  }
  jmp(epilogue);

  allocator.reserve(size());
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
  #define Rn  dis8(rbx, n * 4)
  #define Rm  dis8(rbx, m * 4)
  switch(opcode >> 8 & 0x00f0 | opcode & 0x000f) {

  //MOV.B Rm,@(R0,Rn)
  case 0x04: {
    mov(ra1d, Rn);
    add(ra1d, R0);
    mov(ra2d, Rm);
    call(writeByte);
    return 0;
  }

  //MOV.W Rm,@(R0,Rn)
  case 0x05: {
    mov(ra1d, Rn);
    add(ra1d, R0);
    mov(ra2d, Rm);
    call(writeWord);
    return 0;
  }

  //MOV.L Rm,@(R0,Rn)
  case 0x06: {
    mov(ra1d, Rn);
    add(ra1d, R0);
    mov(ra2d, Rm);
    call(writeLong);
    return 0;
  }

  //MUL.L Rm,Rn
  case 0x07: {
    mov(eax, Rn);
    mov(edx, Rm);
    mul(edx);
    mov(MACL, eax);
    return 0;
  }

  //MOV.B @(R0,Rm),Rn
  case 0x0c: {
    mov(ra1d, Rm);
    add(ra1d, R0);
    call(readByte);
    movsx(eax, al);
    mov(Rn, eax);
    return 0;
  }

  //MOV.W @(R0,Rm),Rn
  case 0x0d: {
    mov(ra1d, Rm);
    add(ra1d, R0);
    call(readWord);
    movsx(eax, ax);
    mov(Rn, eax);
    return 0;
  }

  //MOV.L @(R0,Rm),Rn
  case 0x0e: {
    mov(ra1d, Rm);
    add(ra1d, R0);
    call(readLong);
    mov(Rn, eax);
    return 0;
  }

  //MAC.L @Rm+,@Rn+
  case 0x0f: {
    auto skip = declareLabel();
    mov(ra1d, Rn);
    addd(Rn, imm8(4));
    call(readLong);
    movsxd(rax, eax);
    push(rax);
    push(rax);
    mov(ra1d, Rm);
    addd(Rm, imm8(4));
    call(readLong);
    movsxd(rax, eax);
    pop(rdx);
    pop(rdx);
    imul(rdx);
    mov(rdx, rax);
    add(rdx, MAC);
    mov(al, S);
    test(al, al);
    jz8(skip);
    sal(rdx, imm8(16));
    sar(rdx, imm8(16));
    defineLabel(skip);
    mov(MAC, rdx);
    return 0;
  }

  //MOV.L Rm,@(disp,Rn)
  case 0x10 ... 0x1f: {
    mov(ra1d, Rn);
    add(ra1d, imm8(d4*4));
    mov(ra2d, Rm);
    call(writeLong);
    return 0;
  }

  //MOV.B Rm,@Rn
  case 0x20: {
    mov(ra1d, Rn);
    mov(ra2d, Rm);
    call(writeByte);
    return 0;
  }

  //MOV.W Rm,@Rn
  case 0x21: {
    mov(ra1d, Rn);
    mov(ra2d, Rm);
    call(writeWord);
    return 0;
  }

  //MOV.L Rm,@Rn
  case 0x22: {
    mov(ra1d, Rn);
    mov(ra2d, Rm);
    call(writeLong);
    return 0;
  }

  //MOV.B Rm,@-Rn
  case 0x24: {
    mov(ra1d, Rn);
    dec(ra1d);
    mov(Rn, ra1d);
    mov(ra2d, Rm);
    call(writeByte);
    return 0;
  }

  //MOV.W Rm,@-Rn
  case 0x25: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(2));
    mov(Rn, ra1d);
    mov(ra2d, Rm);
    call(writeWord);
    return 0;
  }

  //MOV.L Rm,@-Rn
  case 0x26: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    mov(ra2d, Rm);
    call(writeLong);
    return 0;
  }

  //DIV0S Rm,Rn
  case 0x27: {
    mov(eax, Rn);
    shr(eax, imm8(31));
    mov(Q, al);
    mov(dl, al);
    mov(eax, Rm);
    shr(eax, imm8(31));
    mov(M, al);
    xor(al, dl);
    mov(T, al);
    return 0;
  }

  //TST Rm,Rn
  case 0x28: {
    mov(eax, Rn);
    and(eax, Rm);
    setz(T);
    return 0;
  }

  //AND Rm,Rn
  case 0x29: {
    mov(eax, Rn);
    and(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //XOR Rm,Rn
  case 0x2a: {
    mov(eax, Rn);
    xor(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //OR Rm,Rn
  case 0x2b: {
    mov(eax, Rn);
    or(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //CMP/STR Rm,Rn
  case 0x2c: {
    mov(eax, Rn);
    mov(edx, Rm);
    xor(eax, edx);
    setnz(T);
    return 0;
  }

  //XTRCT Rm,Rn
  case 0x2d: {
    mov(eax, Rn);
    mov(edx, Rm);
    shr(eax, imm8(16));
    shl(edx, imm8(16));
    or(eax, edx);
    mov(Rn, eax);
    return 0;
  }

  //MULU Rm,Rn
  case 0x2e: {
    mov(eax, Rn);
    mov(edx, Rm);
    movzx(eax, ax);
    movzx(edx, dx);
    mul(edx);
    mov(MACL, eax);
    return 0;
  }

  //MULS Rm,Rn
  case 0x2f: {
    mov(eax, Rn);
    mov(edx, Rm);
    movsx(eax, ax);
    movsx(edx, dx);
    imul(edx);
    mov(MACL, eax);
    return 0;
  }

  //CMP/EQ Rm,Rn
  case 0x30: {
    mov(eax, Rn);
    cmp(eax, Rm);
    setz(T);
    return 0;
  }

  //CMP/HS Rm,Rn
  case 0x32: {
    mov(eax, Rn);
    cmp(eax, Rm);
    setnc(T);
    return 0;
  }

  //CMP/GE Rm,Rn
  case 0x33: {
    mov(eax, Rn);
    cmp(eax, Rm);
    setge(T);
    return 0;
  }

  //DIV1 Rm,Rn
  case 0x34: {
    auto skip = declareLabel();
    auto skip2 = declareLabel();
    mov(al, Q);
    mov(cl, al);
    mov(eax, Rn);
    shr(eax, imm8(31));
    mov(Q, al);
    mov(edx, Rn);
    shl(edx, imm8(1));
    mov(al, T);
    or(dl, al);
    mov(al, M);
    cmp(al, cl);
    jnz8(skip);
    sub(edx, Rm);
    jmp8(skip2);
    defineLabel(skip);
    add(edx, Rm);
    defineLabel(skip2);
    mov(Rn, edx);
    setc(dl);
    mov(al, Q);
    mov(cl, al);
    mov(al, M);
    xor(al, cl);
    xor(al, dl);
    mov(Q, al);
    mov(al, Q);
    mov(cl, al);
    mov(al, M);
    xor(cl, al);
    mov(al, imm8(1));
    sub(al, cl);
    mov(T, al);
    return 0;
  }

  //DMULU.L Rm,Rn
  case 0x35: {
    mov(eax, Rn);
    mov(edx, Rm);
    mul(edx);
    mov(MACL, eax);
    mov(MACH, edx);
    return 0;
  }

  //CMP/HI Rm,Rn
  case 0x36: {
    mov(eax, Rn);
    cmp(eax, Rm);
    seta(T);
    return 0;
  }

  //CMP/GT Rm,Rn
  case 0x37: {
    mov(eax, Rn);
    cmp(eax, Rm);
    setg(T);
    return 0;
  }

  //SUB Rm,Rn
  case 0x38: {
    mov(eax, Rn);
    sub(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //SUBC Rm,Rn
  case 0x3a: {
    mov(al, T);
    rcr(al);
    mov(eax, Rn);
    sbb(eax, Rm);
    mov(Rn, eax);
    setc(T);
    return 0;
  }

  //SUBV Rm,Rn
  case 0x3b: {
    mov(eax, Rn);
    sub(eax, Rm);
    mov(Rn, eax);
    seto(al);
    mov(T, al);
    return 0;
  }

  //ADD Rm,Rn
  case 0x3c: {
    mov(eax, Rn);
    add(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //DMULS.L Rm,Rn
  case 0x3d: {
    mov(eax, Rn);
    mov(edx, Rm);
    imul(edx);
    mov(MACL, eax);
    mov(MACH, edx);
    return 0;
  }

  //ADDC Rm,Rn
  case 0x3e: {
    mov(rax, T);
    rcr(al);
    mov(eax, Rn);
    adc(eax, Rm);
    mov(Rn, eax);
    setc(T);
    return 0;
  }

  //ADDV Rm,Rn
  case 0x3f: {
    mov(eax, Rn);
    add(eax, Rm);
    mov(Rn, eax);
    seto(al);
    mov(T, al);
    return 0;
  }

  //MAC.W @Rm+,@Rn+
  case 0x4f: {
    auto skip = declareLabel();
    mov(ra1d, Rn);
    addd(Rn, imm8(2));
    call(readWord);
    movsx(eax, ax);
    push(rax);
    push(rax);
    mov(ra1d, Rm);
    addd(Rm, imm8(2));
    call(readWord);
    movsx(eax, ax);
    pop(rdx);
    pop(rdx);
    imul(edx);
    shl(rdx, imm8(32));
    or(edx, eax);
    add(rdx, MAC);
    mov(al, S);
    test(al, al);
    jz8(skip);
    movsxd(rdx, edx);
    defineLabel(skip);
    mov(MAC, rdx);
    return 0;
  }

  //MOV.L @(disp,Rm),Rn
  case 0x50 ... 0x5f: {
    mov(ra1d, Rm);
    add(ra1d, imm8(d4*4));
    call(readLong);
    mov(Rn, eax);
    return 0;
  }

  //MOV.B @Rm,Rn
  case 0x60: {
    mov(ra1d, Rm);
    call(readByte);
    movsx(eax, al);
    mov(Rn, eax);
    return 0;
  }

  //MOV.W @Rm,Rn
  case 0x61: {
    mov(ra1d, Rm);
    call(readWord);
    movsx(eax, ax);
    mov(Rn, eax);
    return 0;
  }

  //MOV.L @Rm,Rn
  case 0x62: {
    mov(ra1d, Rm);
    call(readLong);
    mov(Rn, eax);
    return 0;
  }

  //MOV Rm,Rn
  case 0x63: {
    mov(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //MOV.B @Rm+,Rn
  case 0x64: {
    mov(ra1d, Rm);
    if(n != m) addd(Rm, imm8(1));
    call(readByte);
    movsx(eax, al);
    mov(Rn, eax);
    return 0;
  }

  //MOV.W @Rm+,Rn
  case 0x65: {
    mov(ra1d, Rm);
    if(n != m) addd(Rm, imm8(2));
    call(readWord);
    movsx(eax, ax);
    mov(Rn, eax);
    return 0;
  }

  //MOV.L @Rm+,Rn
  case 0x66: {
    mov(ra1d, Rm);
    if(n != m) addd(Rm, imm8(4));
    call(readLong);
    mov(Rn, eax);
    return 0;
  }

  //NOT Rm,Rn
  case 0x67: {
    mov(eax, Rm);
    not(eax);
    mov(Rn, eax);
    return 0;
  }

  //SWAP.B Rm,Rn
  case 0x68: {
    mov(eax, Rm);
    mov(edx, eax);
    mov(al, ah);
    mov(ah, dl);
    mov(Rn, eax);
    return 0;
  }

  //SWAP.W Rm,Rn
  case 0x69: {
    mov(eax, Rm);
    mov(edx, eax);
    shr(eax, imm8(16));
    shl(edx, imm8(16));
    or(eax, edx);
    mov(Rn, eax);
    return 0;
  }

  //NEGC Rm,Rn
  case 0x6a: {
    xor(edx, edx);
    sub(edx, Rm);
    mov(al, T);
    and(eax, imm8(1));
    sub(edx, eax);
    mov(Rn, edx);
    setc(al);
    mov(T, al);
    return 0;
  }

  //NEG Rm,Rn
  case 0x6b: {
    xor(eax, eax);
    sub(eax, Rm);
    mov(Rn, eax);
    return 0;
  }

  //EXTU.B Rm,Rn
  case 0x6c: {
    mov(eax, Rm);
    movzx(eax, al);
    mov(Rn, eax);
    return 0;
  }

  //EXTU.W Rm,Rn
  case 0x6d: {
    mov(eax, Rm);
    movzx(eax, ax);
    mov(Rn, eax);
    return 0;
  }

  //EXTS.B Rm,Rn
  case 0x6e: {
    mov(eax, Rm);
    movsx(eax, al);
    mov(Rn, eax);
    return 0;
  }

  //EXTS.W Rm,Rn
  case 0x6f: {
    mov(eax, Rm);
    movsx(eax, ax);
    mov(Rn, eax);
    return 0;
  }

  //ADD #imm,Rn
  case 0x70 ... 0x7f: {
    mov(al, imm8(d8));
    movsx(eax, al);
    add(eax, Rn);
    mov(Rn, eax);
    return 0;
  }

  //MOV.W @(disp,PC),Rn
  case 0x90 ... 0x9f: {
    mov(ra1d, PC);
    add(ra1d, imm32(d8*2));
    call(readWord);
    movsx(eax, ax);
    mov(Rn, eax);
    return 0;
  }

  //BRA disp
  case 0xa0 ... 0xaf: {
    mov(eax, PC);
    add(eax, imm32(4 + (i12)d12 * 2));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //BSR disp
  case 0xb0 ... 0xbf: {
    mov(eax, PC);
    mov(PR, eax);
    add(eax, imm32(4 + (i12)d12 * 2));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //MOV.L @(disp,PC),Rn
  case 0xd0 ... 0xdf: {
    mov(ra1d, PC);
    and(ra1d, imm8(~3));
    add(ra1d, imm32(d8*4));
    call(readLong);
    mov(Rn, eax);
    return 0;
  }

  //MOV #imm,Rn
  case 0xe0 ... 0xef: {
    mov(eax, imm32((s8)i));
    mov(Rn, eax);
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
  #define Rn dis8(rbx, n * 4)
  #define Rm dis8(rbx, m * 4)
  switch(opcode >> 8) {

  //MOV.B R0,@(disp,Rn)
  case 0x80: {
    mov(ra1d, Rm);
    add(ra1d, imm8(d4));
    mov(ra2d, R0);
    call(writeByte);
    return 0;
  }

  //MOV.W R0,@(disp,Rn)
  case 0x81: {
    mov(ra1d, Rm);
    add(ra1d, imm8(d4*2));
    mov(ra2d, R0);
    call(writeWord);
    return 0;
  }

  //MOV.B @(disp,Rn),R0
  case 0x84: {
    mov(ra1d, Rm);
    add(ra1d, imm8(d4));
    call(readByte);
    movsx(eax, al);
    mov(R0, eax);
    return 0;
  }

  //MOV.W @(disp,Rn),R0
  case 0x85: {
    mov(ra1d, Rm);
    add(ra1d, imm8(d4*2));
    call(readWord);
    movsx(eax, ax);
    mov(R0, eax);
    return 0;
  }

  //CMP/EQ #imm,R0
  case 0x88: {
    mov(eax, R0);
    cmp(eax, imm8(i));
    setz(T);
    return 0;
  }

  //BT disp
  case 0x89: {
    auto skip = declareLabel();
    mov(al, T);
    test(al, al);
    jz8(skip);
    mov(eax, PC);
    add(eax, imm32(4 + (s8)d8 * 2));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Take));
    defineLabel(skip);
    return 1;
  }

  //BF disp
  case 0x8b: {
    auto skip = declareLabel();
    mov(al, T);
    test(al, al);
    jnz8(skip);
    mov(eax, PC);
    add(eax, imm32(4 + (s8)d8 * 2));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Take));
    defineLabel(skip);
    return 1;
  }

  //BT/S disp
  case 0x8d: {
    auto skip = declareLabel();
    mov(al, T);
    test(al, al);
    jz8(skip);
    mov(eax, PC);
    add(eax, imm32(4 + (s8)d8 * 2));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    defineLabel(skip);
    return 1;
  }

  //BF/S disp
  case 0x8f: {
    auto skip = declareLabel();
    mov(al, T);
    test(al, al);
    jnz8(skip);
    mov(eax, PC);
    add(eax, imm32(4 + (s8)d8 * 2));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    defineLabel(skip);
    return 1;
  }

  //MOV.B R0,@(disp,GBR)
  case 0xc0: {
    mov(ra1d, GBR);
    add(ra1d, imm32(d8));
    mov(ra2d, R0);
    call(writeByte);
    return 0;
  }

  //MOV.W R0,@(disp,GBR)
  case 0xc1: {
    mov(ra1d, GBR);
    add(ra1d, imm32(d8*2));
    mov(ra2d, R0);
    call(writeWord);
    return 0;
  }

  //MOV.L R0,@(disp,GBR)
  case 0xc2: {
    mov(ra1d, GBR);
    add(ra1d, imm32(d8*4));
    mov(ra2d, R0);
    call(writeLong);
    return 0;
  }

  //TRAPA #imm
  case 0xc3: {
    xor(edx, edx);
    xor(eax, eax);
    mov(al, T);
    or(dl, al);
    mov(al, S);
    shl(eax, imm8(1));
    or(dl, al);
    mov(al, I);
    shl(eax, imm8(4));
    or(dl, al);
    mov(al, Q);
    shl(eax, imm8(8));
    or(edx, eax);
    mov(al, M);
    shl(eax, imm8(9));
    or(edx, eax);
    mov(ra2d, edx);
    mov(ra1d, R15);
    addd(R15, imm8(4));
    call(writeLong);
    mov(edx, PC);
    add(edx, imm8(2));
    mov(ra2d, edx);
    mov(ra1d, R15);
    addd(R15, imm8(4));
    call(writeLong);
    mov(ra1d, VBR);
    add(ra1d, imm32(i * 4));
    call(readLong);
    add(eax, imm8(4));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Take));
    return 1;
  }

  //MOV.B @(disp,GBR),R0
  case 0xc4: {
    mov(ra1d, GBR);
    add(ra1d, imm32(d8));
    call(readByte);
    movsx(eax, al);
    mov(R0, eax);
    return 0;
  }

  //MOV.W @(disp,GBR),R0
  case 0xc5: {
    mov(ra1d, GBR);
    add(ra1d, imm32(d8*2));
    call(readWord);
    movsx(eax, ax);
    mov(R0, eax);
    return 0;
  }

  //MOV.L @(disp,GBR),R0
  case 0xc6: {
    mov(ra1d, GBR);
    add(ra1d, imm32(d8*4));
    call(readLong);
    mov(R0, rax);
    return 0;
  }

  //MOVA @(disp,PC),R0
  case 0xc7: {
    auto skip = declareLabel();
    mov(eax, PC);
    and(al, imm8(~3));
    add(eax, imm32(d8*4));
    mov(R0, eax);
    mov(eax, PPM);
    test(eax, eax);
    jz8(skip);
    mov(eax, R0);
    sub(eax, imm8(2));
    mov(R0, eax);
    defineLabel(skip);
    return 0;
  }

  //TST #imm,R0
  case 0xc8: {
    mov(eax, R0);
    and(eax, imm32(i));
    setz(T);
    return 0;
  }

  //AND #imm,R0
  case 0xc9: {
    mov(eax, R0);
    and(eax, imm32(i));
    mov(R0, eax);
    return 0;
  }

  //XOR #imm,R0
  case 0xca: {
    mov(eax, R0);
    xor(eax, imm32(i));
    mov(R0, eax);
    return 0;
  }

  //OR #imm,R0
  case 0xcb: {
    mov(eax, R0);
    or(eax, imm32(i));
    mov(R0, eax);
    return 0;
  }

  //TST.B #imm,@(R0,GBR)
  case 0xcc: {
    mov(ra1d, GBR);
    add(ra1d, R0);
    call(readByte);
    and(al, imm8(i));
    setz(T);
    return 0;
  }

  //AND.B #imm,@(R0,GBR)
  case 0xcd: {
    mov(ra1d, GBR);
    add(ra1d, R0);
    call(readByte);
    and(al, imm8(i));
    mov(ra2d, eax);
    mov(ra1d, GBR);
    add(ra1d, R0);
    call(writeByte);
    return 0;
  }

  //XOR.B #imm,@(R0,GBR)
  case 0xce: {
    mov(ra1d, GBR);
    add(ra1d, R0);
    call(readByte);
    xor(al, imm8(i));
    mov(ra2d, eax);
    mov(ra1d, GBR);
    add(ra1d, R0);
    call(writeByte);
    return 0;
  }

  //OR.B #imm,@(R0,GBR)
  case 0xcf: {
    mov(ra1d, GBR);
    add(ra1d, R0);
    call(readByte);
    or(al, imm8(i));
    mov(ra2d, eax);
    mov(ra1d, GBR);
    add(ra1d, R0);
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
  #define Rn dis8(rbx, n * 4)
  #define Rm dis8(rbx, m * 4)
  switch(opcode >> 4 & 0x0f00 | opcode & 0x00ff) {

  //STC SR,Rn
  case 0x002: {
    auto skip = declareLabel();
    auto skip2 = declareLabel();
    xor(edx, edx);
    xor(eax, eax);
    mov(al, T);
    or(dl, al);
    mov(al, S);
    shl(eax, imm8(1));
    or(dl, al);
    mov(al, I);
    shl(eax, imm8(4));
    or(dl, al);
    mov(al, Q);
    test(al, al);
    jz8(skip);
    or(edx, imm32(0x100));
    defineLabel(skip);
    mov(al, M);
    test(al, al);
    jz8(skip2);
    or(edx, imm32(0x200));
    defineLabel(skip2);
    mov(Rn, edx);
    return 0;
  }

  //BSRF Rm
  case 0x003: {
    mov(eax, PC);
    mov(PR, eax);
    add(eax, Rm);
    add(eax, imm8(4));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //STS MACH,Rn
  case 0x00a: {
    mov(eax, MACH);
    mov(Rn, eax);
    return 0;
  }

  //STC GBR,Rn
  case 0x012: {
    mov(eax, GBR);
    mov(Rn, eax);
    return 0;
  }

  //STS MACL,Rn
  case 0x01a: {
    mov(eax, MACL);
    mov(Rn, eax);
    return 0;
  }

  //STC VBR,Rn
  case 0x022: {
    mov(eax, VBR);
    mov(Rn, eax);
    return 0;
  }

  //BRAF Rm
  case 0x023: {
    mov(eax, PC);
    add(eax, Rm);
    add(eax, imm8(4));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //MOVT Rn
  case 0x029: {
    mov(al, T);
    movzx(eax, al);
    mov(Rn, eax);
    return 0;
  }

  //STS PR,Rn
  case 0x02a: {
    mov(eax, PR);
    mov(Rn, eax);
    return 0;
  }

  //SHLL Rn
  case 0x400: {
    mov(eax, Rn);
    shr(eax, imm8(31));
    mov(T, al);
    mov(eax, Rn);
    shl(eax, imm8(1));
    mov(Rn, eax);
    return 0;
  }

  //SHLR Rn
  case 0x401: {
    mov(eax, Rn);
    and(al, imm8(1));
    mov(T, al);
    mov(eax, Rn);
    shr(eax, imm8(1));
    mov(Rn, eax);
    return 0;
  }

  //STS.L MACH,@-Rn
  case 0x402: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    mov(ra2d, MACH);
    call(writeLong);
    return 0;
  }

  //STC.L SR,@-Rn
  case 0x403: {
    auto skip = declareLabel();
    auto skip2 = declareLabel();
    xor(edx, edx);
    xor(eax, eax);
    mov(al, T);
    or(dl, al);
    mov(al, S);
    shl(eax, imm8(1));
    or(dl, al);
    mov(al, I);
    shl(eax, imm8(4));
    or(dl, al);
    mov(al, Q);
    test(al, al);
    jz8(skip);
    or(edx, imm32(0x100));
    defineLabel(skip);
    mov(al, M);
    test(al, al);
    jz8(skip2);
    or(edx, imm32(0x200));
    defineLabel(skip2);
    mov(ra2d, edx);
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    call(writeLong);
    return 0;
  }

  //ROTL Rn
  case 0x404: {
    mov(eax, Rn);
    shr(eax, imm8(31));
    mov(T, al);
    mov(eax, Rn);
    rol(eax);
    mov(Rn, eax);
    return 0;
  }

  //ROTR Rn
  case 0x405: {
    mov(eax, Rn);
    and(al, imm8(1));
    mov(T, al);
    mov(eax, Rn);
    ror(eax);
    mov(Rn, eax);
    return 0;
  }

  //LDS.L @Rm+,MACH
  case 0x406: {
    mov(ra1d, Rn);
    addd(Rn, imm8(4));
    call(readLong);
    mov(MACH, rax);
    return 0;
  }

  //LDC.L @Rm+,SR
  case 0x407: {
    mov(ra1d, Rn);
    addd(Rn, imm8(4));
    call(readLong);
    mov(edx, eax);
    and(al, imm8(1));
    mov(T, al);
    mov(eax, edx);
    shr(eax, imm8(1));
    and(al, imm8(1));
    mov(S, al);
    mov(eax, edx);
    shr(eax, imm8(4));
    and(al, imm8(15));
    mov(I, al);
    mov(eax, edx);
    shr(eax, imm8(8));
    and(al, imm8(1));
    mov(Q, al);
    mov(eax, edx);
    shr(eax, imm8(9));
    and(al, imm8(1));
    mov(M, al);
    return 0;
  }

  //SHLL2 Rn
  case 0x408: {
    mov(eax, Rn);
    shl(eax, imm8(2));
    mov(Rn, eax);
    return 0;
  }

  //SHLR2 Rn
  case 0x409: {
    mov(eax, Rn);
    shr(eax, imm8(2));
    mov(Rn, eax);
    return 0;
  }

  //LDS Rm,MACH
  case 0x40a: {
    mov(eax, Rm);
    mov(MACH, eax);
    return 0;
  }

  //JSR @Rm
  case 0x40b: {
    mov(eax, PC);
    mov(PR, eax);
    mov(eax, Rm);
    add(eax, imm8(4));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //LDC Rm,SR
  case 0x40e: {
    mov(edx, Rm);
    mov(eax, edx);
    and(al, imm8(1));
    mov(T, al);
    mov(eax, edx);
    shr(eax, imm8(1));
    and(al, imm8(1));
    mov(S, al);
    mov(eax, edx);
    shr(eax, imm8(4));
    and(al, imm8(15));
    mov(I, al);
    mov(eax, edx);
    shr(eax, imm8(8));
    and(al, imm8(1));
    mov(Q, al);
    mov(eax, edx);
    shr(eax, imm8(9));
    and(al, imm8(1));
    mov(M, al);
    return 0;
  }

  //DT Rn
  case 0x410: {
    mov(eax, Rn);
    dec(eax);
    mov(Rn, eax);
    setz(al);
    mov(T, al);
    return 0;
  }

  //CMP/PZ Rn
  case 0x411: {
    mov(eax, Rn);
    test(eax, eax);
    setge(T);
    return 0;
  }

  //STS.L MACL,@-Rn
  case 0x412: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    mov(ra2d, MACL);
    call(writeLong);
    return 0;
  }

  //STC.L GBR,@-Rn
  case 0x413: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    mov(ra2d, GBR);
    call(writeLong);
    return 0;
  }

  //CMP/PL Rn
  case 0x415: {
    mov(eax, Rn);
    test(eax, eax);
    setg(T);
    return 0;
  }

  //LDS.L @Rm+,MACL
  case 0x416: {
    mov(ra1d, Rm);
    addd(Rm, imm8(4));
    call(readLong);
    mov(MACL, eax);
    return 0;
  }

  //LDS.L @Rm+,GBR
  case 0x417: {
    mov(ra1d, Rm);
    addd(Rm, imm8(4));
    call(readLong);
    mov(GBR, eax);
    return 0;
  }

  //SHLL8 Rn
  case 0x418: {
    mov(eax, Rn);
    shl(eax, imm8(8));
    mov(Rn, eax);
    return 0;
  }

  //SHLR8 Rn
  case 0x419: {
    mov(eax, Rn);
    shr(eax, imm8(8));
    mov(Rn, eax);
    return 0;
  }

  //LDS Rm,MACL
  case 0x41a: {
    mov(eax, Rm);
    mov(MACL, eax);
    return 0;
  }

  //TAS @Rn
  case 0x41b: {
    mov(ra1d, Rn);
    and(ra1d, imm32(0x1fff'ffff));
    or(ra1d, imm32(0x2000'0000));
    call(readByte);
    mov(cl, al);
    test(al, al);
    setz(al);
    mov(T, al);
    mov(al, cl);
    or(al, imm8(0x80));
    mov(ra2d, eax);
    mov(ra1d, Rn);
    and(ra1d, imm32(0x1fff'ffff));
    or(ra1d, imm32(0x2000'0000));
    call(writeByte);
    return 0;
  }

  //LDC Rm,GBR
  case 0x41e: {
    mov(eax, Rn);
    mov(GBR, eax);
    return 0;
  }

  //SHAL Rn
  case 0x420: {
    mov(eax, Rn);
    shr(eax, imm8(31));
    mov(T, al);
    mov(eax, Rn);
    sal(eax, imm8(1));
    mov(Rn, eax);
    return 0;
  }

  //SHAR Rn
  case 0x421: {
    mov(eax, Rn);
    and(eax, imm8(1));
    mov(T, al);
    mov(eax, Rn);
    sar(eax, imm8(1));
    mov(Rn, eax);
    return 0;
  }

  //STS.L PR,@-Rn
  case 0x422: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    mov(ra2d, PR);
    call(writeLong);
    return 0;
  }

  //STC.L VBR,@-Rn
  case 0x423: {
    mov(ra1d, Rn);
    sub(ra1d, imm8(4));
    mov(Rn, ra1d);
    mov(ra2d, VBR);
    call(writeLong);
    return 0;
  }

  //ROTCL Rn
  case 0x424: {
    mov(eax, Rn);
    shr(eax, imm8(31));
    mov(cl, al);
    mov(edx, Rn);
    shl(edx, imm8(1));
    mov(al, T);
    or(dl, al);
    mov(Rn, edx);
    mov(al, cl);
    mov(T, al);
    return 0;
  }

  //ROTCR Rn
  case 0x425: {
    mov(eax, Rn);
    and(al, imm8(1));
    mov(cl, al);
    mov(edx, Rn);
    shr(edx, imm8(1));
    xor(eax, eax);
    mov(al, T);
    shl(eax, imm8(31));
    or(edx, eax);
    mov(Rn, edx);
    mov(al, cl);
    mov(T, al);
    return 0;
  }

  //LDS.L @Rm+,PR
  case 0x426: {
    mov(ra1d, Rm);
    addd(Rm, imm8(4));
    call(readLong);
    mov(PR, eax);
    return 0;
  }

  //LDC.L @Rm+,VBR
  case 0x427: {
    mov(ra1d, Rm);
    addd(Rm, imm8(4));
    call(readLong);
    mov(VBR, eax);
    return 0;
  }

  //SHLL16 Rn
  case 0x428: {
    mov(eax, Rn);
    shl(eax, imm8(16));
    mov(Rn, eax);
    return 0;
  }

  //SHLR16 Rn
  case 0x429: {
    mov(eax, Rn);
    shr(eax, imm8(16));
    mov(Rn, eax);
    return 0;
  }

  //LDS Rm,PR
  case 0x42a: {
    mov(eax, Rm);
    mov(PR, eax);
    return 0;
  }

  //JMP @Rm
  case 0x42b: {
    mov(eax, Rm);
    add(eax, imm8(4));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //LDC Rm,VBR
  case 0x42e: {
    mov(eax, Rm);
    mov(VBR, eax);
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
    xor(al, al);
    mov(T, al);
    return 0;
  }

  //NOP
  case 0x0009: {
    return 0;
  }

  //RTS
  case 0x000b: {
    mov(eax, PR);
    add(eax, imm8(4));
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    return 1;
  }

  //SETT
  case 0x0018: {
    mov(al, imm8(1));
    mov(T, al);
    return 0;
  }

  //DIV0U
  case 0x0019: {
    xor(al, al);
    mov(Q, al);
    mov(M, al);
    mov(T, al);
    return 0;
  }

  //SLEEP
  case 0x001b: {
    auto skip = declareLabel();
    auto skip2 = declareLabel();
    mov(eax, ET);
    test(al, al);
    jnz8(skip);
    subd(PC, imm8(2));
    jmp8(skip2);
    defineLabel(skip);
    movb(ET, imm8(0));
    defineLabel(skip2);
    return 1;
  }

  //CLRMAC
  case 0x0028: {
    xor(rax, rax);
    mov(MAC, rax);
    return 0;
  }

  //RTE
  case 0x002b: {
    mov(ra1d, R15);
    addd(R15, imm8(4));
    call(readLong);
    mov(PPC, eax);
    movb(PPM, imm8(Branch::Slot));
    mov(ra1d, R15);
    addd(R15, imm8(4));
    call(readLong);
    mov(edx, eax);
    and(al, imm8(1));
    mov(T, al);
    mov(eax, edx);
    shr(eax, imm8(1));
    and(al, imm8(1));
    mov(S, al);
    mov(eax, edx);
    shr(eax, imm8(4));
    and(al, imm8(15));
    mov(I, al);
    mov(eax, edx);
    shr(eax, imm8(8));
    and(al, imm8(1));
    mov(Q, al);
    mov(eax, edx);
    shr(eax, imm8(9));
    and(al, imm8(1));
    mov(M, al);
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
