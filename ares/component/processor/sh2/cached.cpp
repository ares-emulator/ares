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

#define CCR mem(sreg(1), offsetof(Registers, CCR))

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

#undef CCR
