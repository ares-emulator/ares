//{
  struct Allocator {
    auto construct() -> void {
      destruct();
      memory = (u8*)calloc(512_MiB, 1);
      offset = 0;
      mprotect(memory, 512_MiB, PROT_READ | PROT_WRITE | PROT_EXEC);
    }

    auto destruct() -> void {
      free(memory);
      memory = nullptr;
    }

    u8* memory = nullptr;
    u32 offset = 0;
  } allocator;

  struct Block {
    auto execute() -> void {
      auto function = (void (*)())code;
      function();
    }

    u32 step;
    u32 size;
    u8* code;
  };

  struct Pool {
    static auto allocate() -> Pool*;

    auto operator==(const Pool& source) const -> bool { return hashcode == source.hashcode; }
    auto operator< (const Pool& source) const -> bool { return hashcode <  source.hashcode; }
    auto hash() const -> u32 { return hashcode; }

    u32 hashcode = 0;
    Block* blocks[1024];
  };

  auto recompile(u32 address) -> Block*;
  auto recompileEXECUTE(u32 instruction) -> bool;
  auto recompileSPECIAL(u32 instruction) -> bool;
  auto recompileREGIMM(u32 instruction) -> bool;
  auto recompileCOP0(u32 instruction) -> bool;
  auto recompileCOP2(u32 instruction) -> bool;
  auto recompileLWC2(u32 instruction) -> bool;
  auto recompileSWC2(u32 instruction) -> bool;

  struct Recompiler {
    auto reset() -> void {
      pool = nullptr;
      pools.reset();
    }

    Pool* pool = nullptr;
    set<Pool> pools;
  //hashset<Pool> pools;
  } recompiler;
//};
