//{
  struct Block {
    auto operator==(const Block& source) const -> bool { return pc == source.pc; }
    auto operator< (const Block& source) const -> bool { return pc <  source.pc; }

    auto allocate() -> void {
      code = (u8*)malloc(4096);
      mprotect(code, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
    //code = (u8*)mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    }

    auto reset() -> void {
      free(code);
    //munmap(code, 4096);
      code = nullptr;
    }

    auto execute() -> void {
      auto function = (void (*)())code;
      function();
    }

    u32 pc;
    u32 step;
    u8* code;
  };

  auto recompile(u32 address) -> maybe<Block&>;
  auto recompileEXECUTE(u32 instruction) -> bool;
  auto recompileSPECIAL(u32 instruction) -> bool;
  auto recompileREGIMM(u32 instruction) -> bool;
  auto recompileCOP0(u32 instruction) -> bool;
  auto recompileCOP1(u32 instruction) -> bool;

  set<Block> blocks;
//};
