auto ARM7TDMI::bindCDP(Coprocessor& cp) -> void {
  CDP[cp.id] = [&](n4 cm, n3 op2, n4 cd, n4 cn, n4 op1) { return cp.CDP(cm, op2, cd, cn, op1); };
}

auto ARM7TDMI::bindMCR(Coprocessor& cp) -> void {
  MCR[cp.id] = [&](n32 data, n4 cm, n3 op2, n4 cn, n3 op1) { return cp.MCR(data, cm, op2, cn, op1); };
}

auto ARM7TDMI::bindMRC(Coprocessor& cp) -> void {
  MRC[cp.id] = [&](n4 cm, n3 op2, n4 cn, n3 op1) { return cp.MRC(cm, op2, cn, op1); };
}
