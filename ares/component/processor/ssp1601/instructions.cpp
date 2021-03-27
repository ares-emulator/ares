//0xfff8 = reset
//0xfffa = int0 (unknown)
//0xfffc = int1 (hblank)
//0xfffe = int2 (unknown)
auto SSP1601::interrupt() -> u16 {
  if(!IE || !IRQ) return false;
  IE = 0;
  if(STACK >= 6) STACK = 0;
  FRAME[STACK++] = PC;
  if(IRQ.bit(0)) { PC = read(0xfffc); IRQ.bit(0) = 0; STACK = 0; return 0xfffc; }
  if(IRQ.bit(1)) { PC = read(0xfffd); IRQ.bit(1) = 0; return 0xfffd; }
  if(IRQ.bit(2)) { PC = read(0xfffe); IRQ.bit(2) = 0; return 0xfffe; }
  if(IRQ.bit(3)) { PC = read(0xffff); IRQ.bit(3) = 0; return 0xffff; }
  return false;
}

auto SSP1601::instruction() -> void {
  u16 op = fetch();

  switch(op >> 9) {

  //ld d,s
  case 0x00: {
    if(op == 0) break;  //nop
    auto s = n4(op >> 0);
    auto d = n4(op >> 4);
    if(s == SSP_P && d == SSP_A) {
      A = updateP();
    } else {
      writeGR(d, readGR(s));
    }
    break;
  }

  //ld d,(ri)
  case 0x01: {
    auto d = n4(op >> 4);
    writeGR(d, readPR1(op));
    break;
  }

  //ld (ri),s
  case 0x02: {
    auto s = n4(op >> 4);
    writePR1(op, readGR(s));
    break;
  }

  //ld a,addr
  case 0x03: {
    AH = RAM[n9(op)];
    break;
  }

  //ldi d,imm
  case 0x04: {
    auto d = n4(op >> 4);
    writeGR(d, fetch());
    break;
  }

  //ld d,((ri))
  case 0x05: {
    auto d = n4(op >> 4);
    writeGR(d, readPR2(op));
    break;
  }

  //ldi (ri),imm
  case 0x06: {
    writePR1(op, fetch());
    break;
  }

  //ld addr,a
  case 0x07: {
    RAM[n9(op)] = AH;
    break;
  }

  //ld d,ri
  case 0x09: {
    auto d = n4(op >> 4);
    auto s = n2(op >> 0) | (op >> 6 & 4);
    writeGR(d, R[s]);
    break;
  }

  //ld ri,s
  case 0x0a: {
    auto s = n4(op >> 4);
    auto d = n2(op >> 0) | (op >> 6 & 4);
    R[d] = readGR(s);
    break;
  }

  //ldi ri,simm
  case 0x0c ... 0x0f: {
    R[n3(op >> 8)] = n8(op);
    break;
  }

  //call cond,addr
  case 0x24: {
    auto pc = fetch();
    if(!test(op)) break;
    if(STACK >= 6) STACK = 0;
    FRAME[STACK++] = PC;
    PC = pc;
    break;
  }

  //ld d,(a)
  case 0x25: {
    auto d = n4(op >> 4);
    writeGR(d, read(AH));
    break;
  }

  //bra cond,addr
  case 0x26: {
    auto pc = fetch();
    if(!test(op)) break;
    PC = pc;
    break;
  }

  //mod cond,op
  case 0x48: {
    if(!test(op)) break;
    switch(n3(op)) {
    case 2: shr(); break;
    case 3: shl(); break;
    case 6: neg(); break;
    case 7: abs(); break;
    }
    break;
  }

  //mpys
  case 0x1b: {
    A -= updateP();
    Z = A == 0;
    N = A >> 31;
    X = readPR1(op >> 0, 0, op >> 2);
    Y = readPR1(op >> 4, 1, op >> 6);
    break;
  }

  //mpya (rj),(ri),b
  case 0x4b: {
    A += updateP();
    Z = A == 0;
    N = A >> 31;
    X = readPR1(op >> 0, 0, op >> 2);
    Y = readPR1(op >> 4, 1, op >> 6);
    break;
  }

  //mld (rj),(ri),b
  case 0x5b: {
    A = 0;
    C = 0;
    Z = 1;
    V = 0;
    N = 0;
    X = readPR1(op >> 0, 0, op >> 2);
    Y = readPR1(op >> 4, 1, op >> 6);
    break;
  }

  //alu a,s
  case 0x10: { auto v = n4(op) == 3 ? A : n4(op) == 7 ? updateP() : n32(readGR(n4(op)) << 16); sub(v); break; }
  case 0x30: { auto v = n4(op) == 3 ? A : n4(op) == 7 ? updateP() : n32(readGR(n4(op)) << 16); cmp(v); break; }
  case 0x40: { auto v = n4(op) == 3 ? A : n4(op) == 7 ? updateP() : n32(readGR(n4(op)) << 16); add(v); break; }
  case 0x50: { auto v = n4(op) == 3 ? A : n4(op) == 7 ? updateP() : n32(readGR(n4(op)) << 16); and(v); break; }
  case 0x60: { auto v = n4(op) == 3 ? A : n4(op) == 7 ? updateP() : n32(readGR(n4(op)) << 16); or (v); break; }
  case 0x70: { auto v = n4(op) == 3 ? A : n4(op) == 7 ? updateP() : n32(readGR(n4(op)) << 16); eor(v); break; }

  //alu a,(ri)
  case 0x11: { auto v = readPR1(op) << 16; sub(v); break; }
  case 0x31: { auto v = readPR1(op) << 16; cmp(v); break; }
  case 0x41: { auto v = readPR1(op) << 16; add(v); break; }
  case 0x51: { auto v = readPR1(op) << 16; and(v); break; }
  case 0x61: { auto v = readPR1(op) << 16; or (v); break; }
  case 0x71: { auto v = readPR1(op) << 16; eor(v); break; }

  //alu a,addr
  case 0x13: { auto v = RAM[n9(op)] << 16; sub(v); break; }
  case 0x33: { auto v = RAM[n9(op)] << 16; cmp(v); break; }
  case 0x43: { auto v = RAM[n9(op)] << 16; add(v); break; }
  case 0x53: { auto v = RAM[n9(op)] << 16; and(v); break; }
  case 0x63: { auto v = RAM[n9(op)] << 16; or (v); break; }
  case 0x73: { auto v = RAM[n9(op)] << 16; eor(v); break; }

  //alu a,imm
  case 0x14: { auto v = fetch() << 16; sub(v); break; }
  case 0x34: { auto v = fetch() << 16; cmp(v); break; }
  case 0x44: { auto v = fetch() << 16; add(v); break; }
  case 0x54: { auto v = fetch() << 16; and(v); break; }
  case 0x64: { auto v = fetch() << 16; or (v); break; }
  case 0x74: { auto v = fetch() << 16; eor(v); break; }

  //alu a,((ri))
  case 0x15: { auto v = readPR2(op) << 16; sub(v); break; }
  case 0x35: { auto v = readPR2(op) << 16; cmp(v); break; }
  case 0x45: { auto v = readPR2(op) << 16; add(v); break; }
  case 0x55: { auto v = readPR2(op) << 16; and(v); break; }
  case 0x65: { auto v = readPR2(op) << 16; or (v); break; }
  case 0x75: { auto v = readPR2(op) << 16; eor(v); break; }

  //alu a,ri
  case 0x19: { auto v = R[n2(op) | (op >> 6 & 4)] << 16; sub(v); break; }
  case 0x39: { auto v = R[n2(op) | (op >> 6 & 4)] << 16; cmp(v); break; }
  case 0x49: { auto v = R[n2(op) | (op >> 6 & 4)] << 16; add(v); break; }
  case 0x59: { auto v = R[n2(op) | (op >> 6 & 4)] << 16; and(v); break; }
  case 0x69: { auto v = R[n2(op) | (op >> 6 & 4)] << 16; or (v); break; }
  case 0x79: { auto v = R[n2(op) | (op >> 6 & 4)] << 16; eor(v); break; }

  //alu simm
  case 0x1c: { auto v = u8(op) << 16; sub(v); break; }
  case 0x3c: { auto v = u8(op) << 16; cmp(v); break; }
  case 0x4c: { auto v = u8(op) << 16; add(v); break; }
  case 0x5c: { auto v = u8(op) << 16; and(v); break; }
  case 0x6c: { auto v = u8(op) << 16; or (v); break; }
  case 0x7c: { auto v = u8(op) << 16; eor(v); break; }

  }

  updateP();
}
