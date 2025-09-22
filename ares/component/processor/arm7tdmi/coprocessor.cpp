auto ARM7TDMI::bindCDP(n4 id, std::function<void (n4 cm, n3 op2, n4 cd, n4 cn, n4 op1)> handler) -> void {
  CDP[id] = handler;
}

auto ARM7TDMI::bindMCR(n4 id, std::function<void (n32 data, n4 cm, n3 op2, n4 cn, n3 op1)> handler) -> void {
  MCR[id] = handler;
}

auto ARM7TDMI::bindMRC(n4 id, std::function<n32 (n4 cm, n3 op2, n4 cn, n3 op1)> handler) -> void {
  MRC[id] = handler;
}
