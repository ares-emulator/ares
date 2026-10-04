auto TIA::Collision::clock(n1 pf, const ObjectSignals& signals, n1 vblank) -> void {
  //TIA-1A sheet 2 gates every object input to the collision matrix with VB.
  if(vblank) return;

  auto bl = signals.ball;
  auto p0 = signals.player[0];
  auto p1 = signals.player[1];
  auto m0 = signals.missile[0];
  auto m1 = signals.missile[1];

  if(m0 && p1) M0P1 = 1;
  if(m0 && p0) M0P0 = 1;
  if(m1 && p0) M1P0 = 1;
  if(m1 && p1) M1P1 = 1;
  if(p0 && pf) P0PF = 1;
  if(p0 && bl) P0BL = 1;
  if(p1 && pf) P1PF = 1;
  if(p1 && bl) P1BL = 1;
  if(m0 && pf) M0PF = 1;
  if(m0 && bl) M0BL = 1;
  if(m1 && pf) M1PF = 1;
  if(m1 && bl) M1BL = 1;
  if(bl && pf) BLPF = 1;
  if(p0 && p1) P0P1 = 1;
  if(m0 && m1) M0M1 = 1;
}

auto TIA::Collision::read(n8 address, n8 data) const -> n8 {
  switch(address) {
  case 0x00: data.bit(7) = M0P1; data.bit(6) = M0P0; return data;
  case 0x01: data.bit(7) = M1P0; data.bit(6) = M1P1; return data;
  case 0x02: data.bit(7) = P0PF; data.bit(6) = P0BL; return data;
  case 0x03: data.bit(7) = P1PF; data.bit(6) = P1BL; return data;
  case 0x04: data.bit(7) = M0PF; data.bit(6) = M0BL; return data;
  case 0x05: data.bit(7) = M1PF; data.bit(6) = M1BL; return data;
  case 0x06: data.bit(7) = BLPF;                     return data;
  case 0x07: data.bit(7) = P0P1; data.bit(6) = M0M1; return data;
  }
  unreachable;
}

auto TIA::Collision::clear() -> void {
  M0P0 = M0P1 = M1P0 = M1P1 = 0;
  P0PF = P0BL = P1PF = P1BL = 0;
  M0PF = M0BL = M1PF = M1BL = 0;
  BLPF = P0P1 = M0M1 = 0;
}
