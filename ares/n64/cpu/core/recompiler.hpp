//{
  struct Recompiler : recompiler::amd64 {
    CPU& self;
    Recompiler(CPU& self) : self(self) {}

    struct Block {
      auto execute() -> void {
        ((void (*)())code)();
      }

      u32 step;
      u8* code;
    };

    struct Pool {
      Block* blocks[1 << 6];
    };

    auto reset() -> void {
      for(u32 index : range(1 << 21)) pools[index] = nullptr;
    }

    auto invalidate(u32 address) -> void {
      pools[address >> 8 & 0x1fffff] = nullptr;
    }

    auto pool(u32 address) -> Pool*;
    auto block(u32 address) -> Block*;

    auto emit(u32 address) -> Block*;
    auto emitEXECUTE(u32 instruction) -> bool;
    auto emitSPECIAL(u32 instruction) -> bool;
    auto emitREGIMM(u32 instruction) -> bool;
    auto emitCOP0(u32 instruction) -> bool;
    auto emitCOP1(u32 instruction) -> bool;
    auto emitCheckCOP1() -> bool;

    bump_allocator allocator;
    Pool* pools[1 << 21];  //2_MiB * sizeof(void*) == 16_MiB
  } recompiler{*this};
//};
