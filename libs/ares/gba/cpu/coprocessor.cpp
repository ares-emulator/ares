auto CPU::Coprocessor::vcCDP() -> void {
  static bool warnVC = false;
  if(!warnVC) {
    warnVC = true;
    print("Warning: CP0 instruction executed. This likely signifies a Virtual Console ROM which may not function correctly on real hardware.\n");
  }
  cpu.armInstructionUndefined();
}

auto CPU::Coprocessor::debugMCR() -> void {
  return;
}

auto CPU::Coprocessor::debugMRC() -> n32 {
  return cpu.openBus.get(Word, 0);
}
