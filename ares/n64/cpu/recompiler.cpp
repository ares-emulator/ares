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

  bool hasBranched = 0;
  while(true) {
    u32 instruction = bus.readWord(address);
    bool branched = emitEXECUTE(instruction);
    mov(rax, mem64{&self.clock});
    if(unlikely(instruction == 0x1000'ffff)) {
      //accelerate idle loops
      add(rax, imm8{64});
    } else {
      add(rax, imm8{2});
    }
    mov(mem64{&self.clock}, rax);
    call(&CPU::instructionEpilogue, &self);
    test(rax, rax);
    jz(imm8{+1});
    ret();
    address += 4;
    if(hasBranched || (address & 0xfc) == 0) break;  //block boundary
    hasBranched = branched;
  }
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

auto CPU::Recompiler::emitEXECUTE(u32 instruction) -> bool {
  #define DECODER_EXECUTE
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  #define DECODER_SPECIAL
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitREGIMM(u32 instruction) -> bool {
  #define DECODER_REGIMM
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitSCC(u32 instruction) -> bool {
  #define DECODER_SCC
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitFPU(u32 instruction) -> bool {
  call(&CPU::Recompiler::emitCheckFPU, this);
  test(rax, rax);
  jz(imm8{+1});
  ret();

  #define DECODER_FPU
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitCheckFPU() -> bool {
  if(!self.scc.status.enable.coprocessor1) {
    return self.exception.coprocessor1(), true;
  }
  return false;
}

#undef jp
#undef op
#undef br

#undef OP
#undef RD
#undef RT
#undef RS
