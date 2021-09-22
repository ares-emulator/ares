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

alwaysinline auto SH2::Recompiler::emitInstruction(u16 opcode) -> bool {
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

auto SH2::Recompiler::emit(u32 address) -> Block* {
  if(unlikely(allocator.available() < 1_MiB)) {
    print("SH2 allocator flush\n");
    allocator.release(bump_allocator::zero_fill);
    reset();
  }

  auto block = (Block*)allocator.acquire(sizeof(Block));
  block->code = allocator.acquire();
  bind({block->code, allocator.available()});

  bool hasBranched = 0;
  while(true) {
    u16 instruction = self.readWord(address);
    bool branched = emitInstruction(instruction);
    mov(rax, mem64(&self.CCR));
    inc(rax);
    mov(mem64(&self.CCR), rax);
    call(&SH2::instructionEpilogue, &self);
    address += 2;
    if(hasBranched || (address & 0xfe) == 0) break;  //block boundary
    hasBranched = branched;
    test(al, al);
    jz(imm8(1));
    ret();
  }
  ret();

  allocator.reserve(size());
  return block;
}
