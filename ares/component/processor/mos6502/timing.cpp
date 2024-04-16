auto Ricoh2A03::lastCycle() -> void {
  io.interruptPending = (io.irqLine & !I) | io.nmiPending;
}

auto Ricoh2A03::nmi(n16& vector) -> void {
  if(io.nmiPending) {
    io.nmiPending = false;
    vector = 0xfffa;
  }
}

auto Ricoh2A03::irqLine(bool line) -> void {
  // level-sensitive
  io.irqLine = line;
}

auto Ricoh2A03::nmiLine(bool line) -> void {
  // edge-sensitive (0->1)
  if(!io.nmiLine && line) io.nmiPending = true;
  io.nmiLine = line;
}
