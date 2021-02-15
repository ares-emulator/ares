auto CPU::Recompiler::pool(u32 address) -> Pool* {
  auto& pool = pools[address >> 8 & 0x1ffff];
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

  address &= 0x1fff'ffff;
  bool hasBranched = 0;
  while(true) {
    //shortcut: presume CPU is executing out of either CPU RAM or the BIOS area
    u32 instruction = address <= 0x007f'ffff ? cpu.ram.readWord(address) : bios.readWord(address);
    bool branched = emitEXECUTE(instruction);
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

#define OPCODE instruction
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

#define brIPU(id, name, ...) \
  case id: \
    call(&CPU::IPU::name, &self.ipu, ##__VA_ARGS__); \
    return 1; \

#define opIPU(id, name, ...) \
  case id: \
    call(&CPU::IPU::name, &self.ipu, ##__VA_ARGS__); \
    return 0; \

#define opSCC(id, name, ...) \
  case id: \
    call(&CPU::SCC::name, &self.scc, ##__VA_ARGS__); \
    return 0; \

#define opGTE(id, name, ...) \
  case id: \
    call(&CPU::GTE::name, &self.gte, ##__VA_ARGS__); \
    return 0; \

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

auto CPU::Recompiler::emitGTE(u32 instruction) -> bool {
  #define DECODER_GTE
  #include "decoder.hpp"
  return 0;
}

#undef jp
#undef op
#undef brIPU
#undef opIPU
#undef opSCC
#undef opGTE

#undef OPCODE
#undef RD
#undef RT
#undef RS
