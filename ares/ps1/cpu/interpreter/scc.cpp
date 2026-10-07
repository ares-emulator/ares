auto CPU::MFC0(u32& rt, u8 rd) -> void {
  load(rt, getControlRegisterSCC(rd));
}

auto CPU::MTC0(cu32& rt, u8 rd) -> void {
  setControlRegisterSCC(rd, rt);
}

auto CPU::RFE() -> void {
  u32 previous = effectiveStatusRegisterSCC();
  scc.status.frame[0] = scc.status.frame[1];
  scc.status.frame[1] = scc.status.frame[2];
//scc.status.frame[2] remains unchanged
  u64 retirement = max(execution.retiredInstructions + 1, latestStatusVisibilityRetirement());
  scheduleStatusVisibility(previous, statusRegisterSCC(), retirement);
}
