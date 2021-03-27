auto SSP1601::readGR(n4 r) -> n16 {
  switch(r) {
  case  0: return 0xffff;
  case  1: return X;
  case  2: return Y;
  case  3: return AH;
  case  4: return ST;
  case  5: if(s16(--STACK) < 0) STACK = 5; return FRAME[STACK];
  case  6: return PC;
  case  7: return updateP() >> 16;
  case  8: return readEXT(r);
  case  9: return readEXT(r);
  case 10: return readEXT(r);
  case 11: return readEXT(r);
  case 12: return readEXT(r);
  case 13: return readEXT(r);
  case 14: return readEXT(r);
  case 15: return readEXT(r), AL;
  }
  unreachable;
}

auto SSP1601::writeGR(n4 r, u16 v) -> void {
  switch(r) {
  case  0: return;  //read-only
  case  1: X = v; return;
  case  2: Y = v; return;
  case  3: AH = v; return;
  case  4: ST = v; return;
  case  5: if(STACK >= 6) STACK = 0; FRAME[STACK++] = v; return;
  case  6: PC = v; return;
  case  7: return;  //read-only
  case  8: writeEXT(r, v); return;
  case  9: writeEXT(r, v); return;
  case 10: writeEXT(r, v); return;
  case 11: writeEXT(r, v); return;
  case 12: writeEXT(r, v); return;
  case 13: writeEXT(r, v); return;
  case 14: writeEXT(r, v); return;
  case 15: writeEXT(r, v); AL = v; return;
  }
}

auto SSP1601::updateP() -> n32 {
  return P = (s16)X * (s16)Y << !MACS;
}
