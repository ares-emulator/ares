auto SSP1601::disassembleInstruction() -> string {
  return disassembleInstruction(PC, read(PC + 0), read(PC + 1));
}

auto SSP1601::disassembleInstruction(u16 pc, u16 op, u16 imm) -> string {
  string o = "???";
  string p;

  auto nameGR = [&](n4 r) -> string {
    switch(r) {
    case  0: return "-";
    case  1: return "x";
    case  2: return "y";
    case  3: return "a";
    case  4: return "st";
    case  5: return "stack";
    case  6: return "pc";
    case  7: return "p";
    case  8: return "pm0";
    case  9: return "pm1";
    case 10: return "pm2";
    case 11: return "xst";
    case 12: return "pm4";
    case 13: return "pm5";
    case 14: return "pmc";
    case 15: return "al";
    }
    unreachable;
  };

  auto namePR1 = [&](s32 ri, s32 isj2, s32 modi3) -> string {
    u32 t = ri | isj2 | modi3;
    switch(t) {
    case 0x00:
    case 0x01:
    case 0x02: return {"(r", 0 + ri, ")"};
    case 0x03: return {"(0)"};
    case 0x04:
    case 0x05:
    case 0x06: return {"(r", 4 + ri, ")"};
    case 0x07: return {"(0)"};
    case 0x08:
    case 0x09:
    case 0x0a: return {"(r", 0 + ri, "+!)"};
    case 0x0b: return {"(1)"};
    case 0x0c:
    case 0x0d:
    case 0x0e: return {"(r", 4 + ri, "+!)"};
    case 0x0f: return {"(1)"};
    case 0x10:
    case 0x11:
    case 0x12: return {"(r", 0 + ri, "-)"};
    case 0x13: return {"(2)"};
    case 0x14:
    case 0x15:
    case 0x16: return {"(r", 4 + ri, "-)"};
    case 0x17: return {"(2)"};
    case 0x18:
    case 0x19:
    case 0x1a: return {"(r", 0 + ri, "+)"};
    case 0x1b: return {"(3)"};
    case 0x1c:
    case 0x1d:
    case 0x1e: return {"(r", 4 + ri, "+)"};
    case 0x1f: return {"(3)"};
    }
    unreachable;
  };

  auto namePRo = [&](u16 op) -> string {
    return namePR1(op & 0x03, op >> 6 & 0x04, op << 1 & 0x18);
  };

  auto namePR2 = [&](u16 op) -> string {
    n5 t = (op & 0x03) | (op >> 6 & 0x04) | (op << 1 & 0x18);
    switch(t) {
    case 0x00:
    case 0x01:
    case 0x02: return {"(r", 0 + (op & 3), ")+!"};
    case 0x03: return {"(0)+!"};
    case 0x04:
    case 0x05:
    case 0x06: return {"(r", 4 + (op & 3), ")+!"};
    case 0x07: return {"(0)+!"};
    case 0x08: break;
    case 0x09: break;
    case 0x0a: break;
    case 0x0b: return {"(1)+"};
    case 0x0c: break;
    case 0x0d: break;
    case 0x0e: break;
    case 0x0f: return {"(1)+"};
    case 0x10: break;
    case 0x11: break;
    case 0x12: break;
    case 0x13: return {"(2)+"};
    case 0x14: break;
    case 0x15: break;
    case 0x16: break;
    case 0x17: return {"(2)+"};
    case 0x18: break;
    case 0x19: break;
    case 0x1a: break;
    case 0x1b: return {"(3)+"};
    case 0x1c: break;
    case 0x1d: break;
    case 0x1e: break;
    case 0x1f: return {"(3)+"};
    }
    return "???";
  };

  auto nameRAM = [&]() -> string {
    return {"0x", hex(op & 0x1ff, 3L)};
  };

  auto condition = [&]() -> string {
    switch(op >> 4 & 15) {
    case 0:  return {};
    case 5:  return {"z=", op >> 8 & 1, ","};
    case 7:  return {"n=", op >> 8 & 1, ","};
    };
    return "?,";
  };

  auto im = [&]() -> string {
    return {"0x", hex(imm, 4L)};
  };

  switch(op >> 9) {

  //ld d,s
  case 0x00: {
    if(op == 0) {
      o = "nop";
      break;
    }
    auto s = n4(op >> 0);
    auto d = n4(op >> 4);
    if(s == SSP_STACK && d == SSP_PC) {
      o = "ret";
      break;
    }
    o = "ld";
    p = {nameGR(op >> 4), ",", nameGR(op >> 0)};
    break;
  }

  //ld d,(ri)
  case 0x01: {
    o = "ld";
    p = {nameGR(op >> 4), ",", namePRo(op)};
    break;
  }

  //ld (ri),s
  case 0x02: {
    o = "ld";
    p = {namePRo(op), ",", nameGR(op >> 4)};
    break;
  }

  //ld a,addr
  case 0x03: {
    o = "ld";
    p = {"a,[0x", hex(op & 0x1ff, 3L)};
    break;
  }

  //ldi d,imm
  case 0x04: {
    o = "ldi";
    p = {nameGR(op >> 4), ",", im()};
    break;
  }

  //ld d,((ri))
  case 0x05: {
    o = "ld";
    p = {nameGR(op >> 4), ",", namePR2(op)};
    break;
  }

  //ldi (ri),imm
  case 0x06: {
    o = "ldi";
    p = {namePRo(op), ",", im()};
    break;
  }

  //ld addr,a
  case 0x07: {
    o = "ld";
    p = {nameRAM(), ",AH"};
    break;
  }

  //ld d,ri
  case 0x09: {
    o = "ld";
    p = {nameGR(op >> 4), ",r", (op & 0x03) | (op >> 6 & 0x04)};
    break;
  }

  //ld ri,s
  case 0x0a: {
    o = "ld";
    p = {"r", (op & 0x03) | (op >> 6 & 0x04), ",", nameGR(op >> 4)};
    break;
  }

  //ldi ri,simm
  case 0x0c ... 0x0f: {
    o = "ldi";
    p = {"r", (op >> 8 & 7), ",0x", hex(op & 0xff, 2L)};
    break;
  }

  //call cond,addr
  case 0x24: {
    o = "call";
    p = {condition(), im()};
    break;
  }

  //ld d,(a)
  case 0x25: {
    o = "ld";
    p = {nameGR(op >> 4), ",(a)"};
    break;
  }

  //bra cond,addr
  case 0x26: {
    o = "bra";
    p = {condition(), im()};
    break;
  }

  //mod cond,op
  case 0x48: {
    switch(op & 7) {
    case 2:  o = "shr"; break;
    case 3:  o = "shl"; break;
    case 6:  o = "neg"; break;
    case 7:  o = "abs"; break;
    default: o = "???"; break;
    }
    p = {condition(), o};
    break;
  }

  //mpys
  case 0x1b: {
    o = "mpys";
    break;
  }

  //mpya (rj),(ri),b
  case 0x4b: {
    o = "mpya";
    p = {namePR1(op & 0x03, 0, op << 1 & 0x10), ",", namePR1(op >> 4 & 0x03, 4, op >> 3 & 0x18)};
    break;
  }

  //mld (rj),(ri),b
  case 0x5b: {
    o = "mld";
    p = {namePR1(op & 0x03, 0, op << 1 & 0x10), ",", namePR1(op >> 4 & 0x03, 4, op >> 3 & 0x18)};
    break;
  }

  //alu a,s
  case 0x10: o = "sub"; p = nameGR(op >> 4); break;
  case 0x30: o = "cmp"; p = nameGR(op >> 4); break;
  case 0x40: o = "add"; p = nameGR(op >> 4); break;
  case 0x50: o = "and"; p = nameGR(op >> 4); break;
  case 0x60: o = "or";  p = nameGR(op >> 4); break;
  case 0x70: o = "eor"; p = nameGR(op >> 4); break;

  //alu a,(ri)
  case 0x11: o = "sub"; p = namePRo(op); break;
  case 0x31: o = "cmp"; p = namePRo(op); break;
  case 0x41: o = "add"; p = namePRo(op); break;
  case 0x51: o = "and"; p = namePRo(op); break;
  case 0x61: o = "or";  p = namePRo(op); break;
  case 0x71: o = "eor"; p = namePRo(op); break;

  //alu a,addr
  case 0x13: o = "sub"; p = {"[0x", hex(op & 0x1ff, 3L)}; break;
  case 0x33: o = "cmp"; p = {"[0x", hex(op & 0x1ff, 3L)}; break;
  case 0x43: o = "add"; p = {"[0x", hex(op & 0x1ff, 3L)}; break;
  case 0x53: o = "and"; p = {"[0x", hex(op & 0x1ff, 3L)}; break;
  case 0x63: o = "or" ; p = {"[0x", hex(op & 0x1ff, 3L)}; break;
  case 0x73: o = "eor"; p = {"[0x", hex(op & 0x1ff, 3L)}; break;

  //alu a,imm
  case 0x14: o = "sub"; p = im(); break;
  case 0x34: o = "cmp"; p = im(); break;
  case 0x44: o = "add"; p = im(); break;
  case 0x54: o = "and"; p = im(); break;
  case 0x64: o = "or";  p = im(); break;
  case 0x74: o = "eor"; p = im(); break;

  //alu a,((ri))
  case 0x15: o = "sub"; p = namePR2(op); break;
  case 0x35: o = "cmp"; p = namePR2(op); break;
  case 0x45: o = "add"; p = namePR2(op); break;
  case 0x55: o = "and"; p = namePR2(op); break;
  case 0x65: o = "or";  p = namePR2(op); break;
  case 0x75: o = "eor"; p = namePR2(op); break;

  //alu a,ri
  case 0x19: o = "sub"; p = {"r", (op & 0x03) | (op >> 6 & 4)}; break;
  case 0x39: o = "cmp"; p = {"r", (op & 0x03) | (op >> 6 & 4)}; break;
  case 0x49: o = "add"; p = {"r", (op & 0x03) | (op >> 6 & 4)}; break;
  case 0x59: o = "and"; p = {"r", (op & 0x03) | (op >> 6 & 4)}; break;
  case 0x69: o = "or" ; p = {"r", (op & 0x03) | (op >> 6 & 4)}; break;
  case 0x79: o = "eor"; p = {"r", (op & 0x03) | (op >> 6 & 4)}; break;

  //alu simm
  case 0x1c: o = "sub"; p = {"0x", hex(op & 0xff, 2L)}; break;
  case 0x3c: o = "cmp"; p = {"0x", hex(op & 0xff, 2L)}; break;
  case 0x4c: o = "add"; p = {"0x", hex(op & 0xff, 2L)}; break;
  case 0x5c: o = "and"; p = {"0x", hex(op & 0xff, 2L)}; break;
  case 0x6c: o = "or";  p = {"0x", hex(op & 0xff, 2L)}; break;
  case 0x7c: o = "eor"; p = {"0x", hex(op & 0xff, 2L)}; break;

  }

  string s;

  while(o.size() < 5) o.append(" ");
  s.append(o, " ");

  while(p.size() < 20) p.append(" ");
  s.append(p);

  return s;
}

auto SSP1601::disassembleContext() -> string {
  string s;

  s.append("P:", hex(P >> 16, 4L), "'", hex(P, 4L), " ");
  s.append("A:", hex(A >> 16, 4L), "'", hex(A, 4L), " ");
  s.append("X:", hex(X, 4L), " ");
  s.append("Y:", hex(Y, 4L), " ");
  s.append("ST:",hex(ST,4L), " ");

  s.append("R0:", hex(R[0], 2L), " ");
  s.append("R1:", hex(R[1], 2L), " ");
  s.append("R2:", hex(R[2], 2L), " ");
  s.append("R3:", hex(R[3], 2L), " ");
  s.append("R4:", hex(R[4], 2L), " ");
  s.append("R5:", hex(R[5], 2L), " ");
  s.append("R6:", hex(R[6], 2L), " ");
  s.append("R7:", hex(R[7], 2L), " ");

  s.append(C ? "C" : "c");
  s.append(Z ? "Z" : "z");
  s.append(V ? "V" : "v");
  s.append(N ? "N" : "n");
  s.append(" ", STACK);

  return s;
}
