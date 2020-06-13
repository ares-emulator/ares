//{
  struct Recompiler : recompiler::amd64 {
    RSP& self;
    Recompiler(RSP& self) : self(self) {}

    struct Block {
      auto execute() -> void {
        ((void (*)())code)();
      }

      u32 step;
      u8* code;
    };

    struct Pool {
      auto operator==(const Pool& source) const -> bool { return hashcode == source.hashcode; }
      auto operator< (const Pool& source) const -> bool { return hashcode <  source.hashcode; }
      auto hash() const -> u32 { return hashcode; }

      u32 hashcode;
      Block* blocks[1024];
    };

    auto reset() -> void {
      context = nullptr;
      pools.reset();
    }

    auto invalidate() -> void {
      context = nullptr;
    }

    auto pool() -> Pool*;
    auto block(u32 address) -> Block*;

    auto emit(u32 address) -> Block*;
    auto emitEXECUTE(u32 instruction) -> bool;
    auto emitSPECIAL(u32 instruction) -> bool;
    auto emitREGIMM(u32 instruction) -> bool;
    auto emitCOP0(u32 instruction) -> bool;
    auto emitCOP2(u32 instruction) -> bool;
    auto emitLWC2(u32 instruction) -> bool;
    auto emitSWC2(u32 instruction) -> bool;

    bump_allocator allocator;
    Pool* context = nullptr;
    set<Pool> pools;
  //hashset<Pool> pools;
  } recompiler{*this};
//};
