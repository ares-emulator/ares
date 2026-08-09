auto GTIA::Priority::signals(u8 players, u8 playfields) const -> Signals {
  const auto& priorityRule = Rules[rule()];
  bool p0 = players & 0x01;
  bool p1 = players & 0x02;
  bool p2 = players & 0x04;
  bool p3 = players & 0x08;
  bool pf0 = playfields & 0x01;
  bool pf1 = playfields & 0x02;
  bool pf2 = playfields & 0x04;
  bool pf3 = playfields & 0x08;
  bool p01 = p0 || p1;
  bool p23 = p2 || p3;
  bool pf01 = pf0 || pf1;
  bool pf23 = pf2 || pf3;

  // These are the output-gate equations from the GTIA hardware manual. More
  // than one output can be active; the corresponding color registers are
  // electrically ORed, which is observable for PRIOR=0 and combined PRI bits.
  bool sp0 = p0 && !(pf01 && priorityRule.pri23) && !(priorityRule.pri2 && pf23);
  bool sp1 = p1 && !(pf01 && priorityRule.pri23) && !(priorityRule.pri2 && pf23)
    && (!p0 || multicolor());
  bool sp2 = p2 && !p01 && !(pf23 && priorityRule.pri12) && !(pf01 && !priorityRule.pri0);
  bool sp3 = p3 && !p01 && !(pf23 && priorityRule.pri12) && !(pf01 && !priorityRule.pri0)
    && (!p2 || multicolor());

  bool sf3 = pf3 && !(p23 && priorityRule.pri03) && !(p01 && !priorityRule.pri2);
  bool sf0 = pf0 && !(p23 && priorityRule.pri0) && !(p01 && priorityRule.pri01) && !sf3;
  bool sf1 = pf1 && !(p23 && priorityRule.pri0) && !(p01 && priorityRule.pri01) && !sf3;
  bool sf2 = pf2 && !(p23 && priorityRule.pri03) && !(p01 && !priorityRule.pri2) && !sf3;
  bool background = !p01 && !p23 && !pf01 && !pf23;

  return {
    (u8)(sp0 << 0 | sp1 << 1 | sp2 << 2 | sp3 << 3),
    (u8)(sf0 << 0 | sf1 << 1 | sf2 << 2 | sf3 << 3),
    background,
  };
}

auto GTIA::Priority::clock() -> void {
  if(lowDelay && !--lowDelay) control = ((u8)control & 0xc0) | pendingLow;
  if(modeDelay && !--modeDelay) control = ((u8)control & 0x3f) | ((u8)pendingMode << 6);
}

auto GTIA::Priority::write(n8 data) -> void {
  pendingLow = (u8)data & 0x3f;
  pendingMode = (u8)data >> 6;
  lowDelay = 2;
  modeDelay = 4;
}

auto GTIA::Priority::mode() const -> u8 {
  return (u8)control >> 6;
}

auto GTIA::Priority::rule() const -> u8 {
  return (u8)control & 15;
}

auto GTIA::Priority::fifthPlayer() const -> bool {
  return control.bit(4);
}

auto GTIA::Priority::multicolor() const -> bool {
  return control.bit(5);
}
