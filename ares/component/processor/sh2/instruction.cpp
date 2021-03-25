auto SH2::branch(u32 pc) -> void {
  PPC = pc;
  PPM = Branch::Take;
}

auto SH2::delaySlot(u32 pc) -> void {
  PPC = pc;
  PPM = Branch::Slot;
}

auto SH2::interrupt(u8 level, u8 vector) -> void {
  writeLong(SP - 4, SR);
  writeLong(SP - 8, PC);
  SP -= 8;
  PC  = 4 + readLong(VBR + vector * 4);
  PPM = Branch::Step;
  SR.I = level;
}

auto SH2::exception(u8 vector) -> void {
  writeLong(SP - 4, SR);
  writeLong(SP - 8, PC - 2);
  SP -= 8;
  PC  = 4 + readLong(VBR + vector * 4);
  PPM = Branch::Step;
}

auto SH2::illegal(u16 opcode) -> void {
  exception(0x04);
  debug(unusual, "[SH2] illegal instruction: 0x", hex(opcode, 4L));
}

auto SH2::instruction() -> void {
  if constexpr(Accuracy::Interpreter) {
    u16 opcode = readWord(PC - 4);
    execute(opcode);
    instructionEpilogue();
    step(1);
  }

  if constexpr(Accuracy::Recompiler) {
    auto block = recompiler.block(PC - 4);
    block->execute();
    step(recompiler.clock);
    recompiler.clock = 0;
  }
}

auto SH2::instructionEpilogue() -> bool {
  switch(PPM) {
  case Branch::Step: PC = PC + 2; return 0;
  case Branch::Slot: PC = PC + 2; PPM = Branch::Take; return 0;
  case Branch::Take: PC = PPC;    PPM = Branch::Step; return 1;
  case Branch::Exception:         PPM = Branch::Step; return 1;
  }
  unreachable;
}

auto SH2::execute(u16 opcode) -> void {
  #define op(id, name, ...) case id: return name(__VA_ARGS__)
  #define br(id, name, ...) case id: return name(__VA_ARGS__)
  #include "decoder.hpp"
  #undef op
  #undef br
}
