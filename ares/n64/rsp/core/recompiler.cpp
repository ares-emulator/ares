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
  auto& block = pool()->blocks[address >> 2 & 0x3ff];
  if(!block) block = emit(address);
  return block;
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

  bool branched = 0;
  uint instructions = 0;
  do {
    u32 instruction = self.imem.readWord(address);
    branched = emitEXECUTE(instruction);
    call(&RSP::instructionEpilogue, &self);
    test(rax, rax);
    jz(imm8{+1});
    ret();
    address += 4;
    if((address & 0xffc) == 0) break;  //IMEM boundary
  } while(++instructions < 64 && !branched);
  ret();

  block->step = 3 * instructions;
  allocator.reserve(size());
//print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}

#define OP instruction
#define RD &self.GPR[RDn]
#define RT &self.GPR[RTn]
#define RS &self.GPR[RSn]
#define VD &self.VPR[VDn]
#define VS &self.VPR[VSn]
#define VT &self.VPR[VTn]

#define jp(id, name) \
  case id: \
    return emit##name(instruction); \

#define op(id, name, ...) \
  case id: \
    call(&RSP::instruction##name, &self, ##__VA_ARGS__); \
    return 0; \

#define br(id, name, ...) \
  case id: \
    call(&RSP::instruction##name, &self, ##__VA_ARGS__); \
    return 1; \

auto RSP::Recompiler::emitEXECUTE(u32 instruction) -> bool {
  #define DECODER_EXECUTE
  #include "decoder.hpp"
  return 0;
}

auto RSP::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  #define DECODER_SPECIAL
  #include "decoder.hpp"
  return 0;
}

auto RSP::Recompiler::emitREGIMM(u32 instruction) -> bool {
  #define DECODER_REGIMM
  #include "decoder.hpp"
  return 0;
}

auto RSP::Recompiler::emitCOP0(u32 instruction) -> bool {
  #define DECODER_COP0
  #include "decoder.hpp"
  return 0;
}

auto RSP::Recompiler::emitCOP2(u32 instruction) -> bool {
  #define DECODER_COP2
  #include "decoder.hpp"
  return 0;
}

auto RSP::Recompiler::emitLWC2(u32 instruction) -> bool {
  #define DECODER_LWC2
  #include "decoder.hpp"
  return 0;
}

auto RSP::Recompiler::emitSWC2(u32 instruction) -> bool {
  #define DECODER_SWC2
  #include "decoder.hpp"
  return 0;
}

#undef jp
#undef op
#undef br

#undef OP
#undef RD
#undef RT
#undef RS
#undef VD
#undef VS
#undef VT
