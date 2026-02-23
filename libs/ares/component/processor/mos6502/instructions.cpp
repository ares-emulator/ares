auto MOS6502::instructionNone(addr mode, algorithm alg) -> void {
  MAR = ADDR();
  ALG();
}

auto MOS6502::instructionLoad(addr mode, algorithm alg) -> void {
  MAR = ADDR();
L MDR = read(MAR);
  ALG();
}

auto MOS6502::instructionStore(addr mode, algorithm alg) -> void {
  MAR = ADDR();
  ALG();
L write(MAR, MDR);
}

auto MOS6502::instructionModify(addr mode, algorithm alg) -> void {
  MAR = ADDR();
  MDR = read(MAR);
  write(MAR, MDR); // dummy write
  ALG();
L write(MAR, MDR);
}
