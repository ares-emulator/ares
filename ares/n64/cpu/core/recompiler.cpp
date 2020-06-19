auto CPU::Recompiler::pool(u32 address) -> Pool* {
  auto& pool = pools[address >> 8 & 0x1fffff];
  if(!pool) pool = (Pool*)allocator.acquire(sizeof(Pool));
  return pool;
}

auto CPU::Recompiler::block(u32 address) -> Block* {
  auto& block = pool(address)->blocks[address >> 2 & 0x3f];
  if(!block) block = emit(address);
  return block;
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

  bool branched = 0;
  uint instructions = 0;
  uint idle = 0;
  do {
    u32 instruction = bus.readWord(address);
    if(instruction == 0x1000'ffff) idle += 64;  //accelerate idle loops
    branched = emitEXECUTE(instruction);
    call(&CPU::instructionEpilogue, &self);
    test(rax, rax);
    jz(imm8{+1});
    ret();
    address += 4;
    if((address & 0xfc) == 0) break;  //block boundary
  } while(++instructions < 64 && !branched);
  ret();

  block->step = 2 * instructions + idle;
  allocator.reserve(size());
//print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}

#define OP instruction
#define RD &self.GPR[RDn]
#define RT &self.GPR[RTn]
#define RS &self.GPR[RSn]

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

auto CPU::Recompiler::emitCOP0(u32 instruction) -> bool {
  #define DECODER_COP0
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitCOP1(u32 instruction) -> bool {
  call(&CPU::Recompiler::emitCheckCOP1, this);
  test(rax, rax);
  jz(imm8{+1});
  ret();

  #define DECODER_COP1
  #include "decoder.hpp"
  return 0;
}

auto CPU::Recompiler::emitCheckCOP1() -> bool {
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
