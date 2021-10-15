auto SH2::jump(u32 pc) -> void {
  PC  = pc;
  PPM = Branch::Step;
}

auto SH2::branch(u32 pc) -> void {
  if(inDelaySlot()) return illegalSlotInstruction();
  PPC = pc;
  PPM = Branch::Take;
}

auto SH2::delaySlot(u32 pc) -> void {
  if(inDelaySlot()) return illegalSlotInstruction();
  PPC = pc;
  PPM = Branch::Slot;
}

auto SH2::instruction() -> void {
  if constexpr(Accuracy::Interpreter) {
    step(1);
    exceptionHandler();
    if constexpr(Accuracy::AddressErrors) {
      if(unlikely(PC & 1)) return addressErrorCPU();
      if(unlikely(PC >> 29 == Area::IO)) return addressErrorCPU();
    }
    ID = 0;
    u16 opcode = readWord(PC - 4);
    execute(opcode);
    instructionEpilogue();
  }

  if constexpr(Accuracy::Recompiler) {
    exceptionHandler();

    // Recompiled blocks may be very small, negating the impact
    // minimum cycle counts ensure that the recompiler is a net positive
    do {
      auto block = recompiler.block(PC - 4);
      block->execute(*this);
    } while (CCR <= recompiler.min_cycles);

    step(CCR);
    CCR = 0;
    ID = 0;
  }
}

auto SH2::instructionEpilogue() -> bool {
  switch(PPM) {
  case Branch::Step: PC = PC + 2; return 0;
  case Branch::Slot: PC = PC + 2; PPM = Branch::Take; return 0;
  case Branch::Take: PC = PPC;    PPM = Branch::Step; return 1;
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
