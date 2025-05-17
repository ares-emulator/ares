auto CPU::CP0::CDP(n4 cm, n3 op2, n4 cd, n4 cn, n4 op1) -> void {
  static bool warnVC = false;
  if(!warnVC) {
    warnVC = true;
    print("Warning: CP0 instruction executed. This likely signifies a Virtual Console ROM which may not function correctly on real hardware.\n");
  }
  cpu.armInstructionUndefined();
}

auto CPU::CP14::MCR(n32 data, n4 cm, n3 op2, n4 cn, n3 op1) -> void {
  return;
}

auto CPU::CP14::MRC(n4 cm, n3 op2, n4 cn, n3 op1) -> n32 {
  return cpu.openBus.get(Word, 0);
}
