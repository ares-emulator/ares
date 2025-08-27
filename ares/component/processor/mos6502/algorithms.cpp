auto MOS6502::algorithmADC() -> void { 
  n8 i = MDR;
  i16 o = A + i + C;
  if(!BCD || !D) {
    C = o.bit(8);
    Z = n8(o) == 0;
    N = o.bit(7);
    V = ~(A ^ i) & (A ^ o) & 0x80;
  } else {
    idle();
    Z = n8(o) == 0;
    o = (A & 0x0f) + (i & 0x0f) + (C << 0);
    if(o > 0x09) o += 0x06;
    C = o > 0x0f;
    o = (A & 0xf0) + (i & 0xf0) + (C << 4) + (o & 0x0f);
    N = o.bit(7);
    V = ~(A ^ i) & (A ^ o) & 0x80;
    if(o > 0x9f) o += 0x60;
    C = o > 0xff;
  }
  A = o;
}

auto MOS6502::algorithmAND() -> void {
  A &= MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmASLA() -> void {
  C = A.bit(7);
  A <<= 1;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmASLM() -> void {
  C = MDR.bit(7);
  MDR <<= 1;
  Z = MDR == 0;
  N = MDR.bit(7);
}

auto MOS6502::algorithmBCC() -> void {
  algorithmBranch(C == 0);
}

auto MOS6502::algorithmBCS() -> void {
  algorithmBranch(C == 1);
}

auto MOS6502::algorithmBEQ() -> void {
  algorithmBranch(Z == 1);
}

auto MOS6502::algorithmBIT() -> void {
  Z = (A & MDR) == 0;
  V = MDR.bit(6);
  N = MDR.bit(7);
}

auto MOS6502::algorithmBMI() -> void {
  algorithmBranch(N == 1);
}

auto MOS6502::algorithmBNE() -> void {
  algorithmBranch(Z == 0);
}

auto MOS6502::algorithmBPL() -> void {
  algorithmBranch(N == 0);
}

auto MOS6502::algorithmBRK() -> void {
  MDR = read(MAR); // dummy implied read
  PC++;
  push(PCH);
  push(PCL);

  n16 vector = 0xfffe;
  nmi(vector);

  push(P | 0x30);
  I = 1;

  PCL = read(vector++);
  cancelNmi();
  PCH = read(vector++);
}

auto MOS6502::algorithmBVC() -> void {
  algorithmBranch(V == 0);
}

auto MOS6502::algorithmBVS() -> void {
  algorithmBranch(V == 1);
}

auto MOS6502::algorithmBranch(bool take) -> void {
  if(!take) {
  L MDR = read(MAR); // dummy relative read
  } else {
    bool irq = irqPending();
    MDR = read(MAR);
    n16 newPC = PC + (i8)MDR;
    bool pageCrossed = newPC >> 8 != PC >> 8;

    // idle page crossed without carry
    if (pageCrossed) {
      read(PC & 0xff00 | newPC & 0x00ff);
      PC = newPC;
    L idle();
    } else {
      // a taken non-page-crossing branch ignores
      // IRQ/NMI during its last clock, so that
      // next instruction executes before the IRQ
      if (!irq)
        delayIrq();
      else
        lastCycle();

      PC = newPC;
      // idle page crossed with carry
      idle();
    }
  }
}

auto MOS6502::algorithmCLC() -> void {
  C = 0;
}

auto MOS6502::algorithmCLD() -> void {
  D = 0;
}

auto MOS6502::algorithmCLI() -> void {
  I = 0;
}

auto MOS6502::algorithmCLV() -> void {
  V = 0;
}

auto MOS6502::algorithmCMP() -> void {
  n9 o = A - MDR;
  C = !o.bit(8);
  Z = n8(o) == 0;
  N = o.bit(7);
}

auto MOS6502::algorithmCPX() -> void {
  n9 o = X - MDR;
  C = !o.bit(8);
  Z = n8(o) == 0;
  N = o.bit(7);
}

auto MOS6502::algorithmCPY() -> void {
  n9 o = Y - MDR;
  C = !o.bit(8);
  Z = n8(o) == 0;
  N = o.bit(7);
}

auto MOS6502::algorithmDEC() -> void {
  MDR--;
  Z = MDR == 0;
  N = MDR.bit(7);
}

auto MOS6502::algorithmDEX() -> void {
  X--;
  Z = X == 0;
  N = X.bit(7);
}

auto MOS6502::algorithmDEY() -> void {
  Y--;
  Z = Y == 0;
  N = Y.bit(7);
}

auto MOS6502::algorithmEOR() -> void {
  A ^= MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmINC() -> void {
  MDR++;
  Z = MDR == 0;
  N = MDR.bit(7);
}

auto MOS6502::algorithmINX() -> void {
  X++;
  Z = X == 0;
  N = X.bit(7);
}

auto MOS6502::algorithmINY() -> void {
  Y++;
  Z = Y == 0;
  N = Y.bit(7);
}

auto MOS6502::algorithmJMP() -> void {
  PC = MAR;
}

auto MOS6502::algorithmJSR() -> void {
  n16 target = operand();
  MDR = read(0x0100 | S); // dummy stack read
  push(PCH);
  push(PCL);
L target |= operand() << 8;
  PC = target;
}

auto MOS6502::algorithmLDA() -> void {
  A = MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmLDX() -> void {
  X = MDR;
  Z = X == 0;
  N = X.bit(7);
}

auto MOS6502::algorithmLDY() -> void {
  Y = MDR;
  Z = Y == 0;
  N = Y.bit(7);
}

auto MOS6502::algorithmLSRA() -> void {
  C = A.bit(0);
  A >>= 1;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmLSRM() -> void {
  C = MDR.bit(0);
  MDR >>= 1;
  Z = MDR == 0;
  N = MDR.bit(7);
}

auto MOS6502::algorithmNOP() -> void {}

auto MOS6502::algorithmORA() -> void {
  A |= MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmPHA() -> void {
  read(MAR); // dummy implied read
L push(A);
}

auto MOS6502::algorithmPHP() -> void {
  MDR = read(MAR); // dummy implied read
L push(P | 0x30);
}

auto MOS6502::algorithmPLA() -> void {
  MDR = read(MAR);        // dummy implied read
  MDR = read(0x0100 | S); // dummy stack read
L A = pull();
  Z = n8(A) == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmPLP() -> void {
  MDR = read(MAR);        // dummy implied read
  MDR = read(0x0100 | S); // dummy stack read
L P = pull();
}

auto MOS6502::algorithmROLA() -> void {
  bool c = C;
  C = A.bit(7);
  A = A << 1 | c;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmROLM() -> void {
  bool c = C;
  C = MDR.bit(7);
  MDR = MDR << 1 | c;
  Z = MDR == 0;
  N = MDR.bit(7);
}

auto MOS6502::algorithmRORA() -> void {
  bool c = C;
  C = A.bit(0);
  A = A >> 1 | c << 7;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmRORM() -> void {
  bool c = C;
  C = MDR.bit(0);
  MDR = MDR >> 1 | c << 7;
  Z = MDR == 0;
  N = MDR.bit(7);
}

auto MOS6502::algorithmRTI() -> void {
  MDR = read(MAR);        // dummy implied read
  MDR = read(0x0100 | S); // dummy stack read
  P = pull();
  PCL = pull();
L PCH = pull();
}

auto MOS6502::algorithmRTS() -> void {
  MDR = read(MAR);        // dummy implied read
  MDR = read(0x0100 | S); // dummy stack read
  PCL = pull();
  PCH = pull();
L idle();
  PC++;
}

auto MOS6502::algorithmSBC() -> void {
  n8 i = ~MDR;
  i16 o = A + i + C;
  if(!BCD || !D) {
    C = o.bit(8);
    Z = n8(o) == 0;
    N = o.bit(7);
    V = ~(A ^ i) & (A ^ o) & 0x80;
  } else {
    idle();
    Z = n8(o) == 0;
    o = (A & 0x0f) + (i & 0x0f) + (C << 0);
    if(o <= 0x0f) o -= 0x06;
    C = o > 0x0f;
    o = (A & 0xf0) + (i & 0xf0) + (C << 4) + (o & 0x0f);
    N = o.bit(7);
    V = ~(A ^ i) & (A ^ o) & 0x80;
    if(o <= 0xff) o -= 0x60;
    C = o > 0xff;
  }
  A = o;
}

auto MOS6502::algorithmSEC() -> void {
  C = 1;
}

auto MOS6502::algorithmSED() -> void {
  D = 1;
}

auto MOS6502::algorithmSEI() -> void {
  I = 1;
}

auto MOS6502::algorithmSTA() -> void {
  MDR = A;
}

auto MOS6502::algorithmSTX() -> void {
  MDR = X;
}

auto MOS6502::algorithmSTY() -> void {
  MDR = Y;
}

auto MOS6502::algorithmTAX() -> void {
  X = A;
  Z = X == 0;
  N = X.bit(7);
}

auto MOS6502::algorithmTAY() -> void {
  Y = A;
  Z = Y == 0;
  N = Y.bit(7);
}

auto MOS6502::algorithmTSX() -> void {
  X = S;
  Z = X == 0;
  N = X.bit(7);
}

auto MOS6502::algorithmTXA() -> void {
  A = X;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmTXS() -> void {
  S = X;
}

auto MOS6502::algorithmTYA() -> void {
  A = Y;
  Z = A == 0;
  N = A.bit(7);
}

// unofficial algorithm
auto MOS6502::algorithmAAC() -> void {
  A &= MDR;
  C = A.bit(7);
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmAAX() -> void {
  MDR = A & X;
}

// Highly unstable, do not use.
// A base value in A is determined based on the
// contets of A and a constant, which may be
// typically $00, $ff, $ee, etc. The value of
// this constant depends on temerature, the chip
// series, and maybe other factors, as well.
// In order to eliminate these uncertaincies
// from the equation, use either 0 as the operand
// or a value of $FF in the accumulator.
auto MOS6502::algorithmANE() -> void {
  A = (A | 0xff) & X & MDR;
  Z = A == 0;
  N = A.bit(7);
}

// This operation involves the adder:
// V-flag is set according to (A AND oper) + oper
// The carry is not set, but bit 7 (sign)
// is exchanged with the carry
auto MOS6502::algorithmARR() -> void {
    A = ((A & MDR) >> 1) | (C << 7);
    C = A.bit(6);
    Z = A == 0;
    N = A.bit(7);
    V = C ^ A.bit(5);
}

auto MOS6502::algorithmASR() -> void {
  A &= MDR;
  C = A.bit(0);
  A >>= 1;
  Z = A == 0;
  N = A.bit(7);
}

// Highly unstable, involves a 'magic' constant, see ANE
auto MOS6502::algorithmATX() -> void {
  X = A = (A | 0xff) & MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmAXS() -> void {
  X &= A;
  n9 o = X - MDR;
  C = !o.bit(8);
  X = o;
  Z = X == 0;
  N = X.bit(7);
}

auto MOS6502::algorithmDCP() -> void {
  MDR--;
  algorithmCMP();
}

auto MOS6502::algorithmISC() -> void {
  MDR++;
  algorithmSBC();
}

auto MOS6502::algorithmLAS() -> void {
  S = A = X = MDR & S;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmLAX() -> void {
  A = X = MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmRLA() -> void {
  bool c = C;
  C = MDR.bit(7);
  MDR = MDR << 1 | c;
  A &= MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmRRA() -> void {
  bool c = C;
  C = MDR.bit(0);
  MDR = MDR >> 1 | c << 7;
  algorithmADC();
}

auto MOS6502::algorithmSLO() -> void {
  C = MDR.bit(7);
  MDR <<= 1;
  A |= MDR;
  Z = A == 0;
  N = A.bit(7);
}

auto MOS6502::algorithmSRE() -> void {
  C = MDR.bit(0);
  MDR >>= 1;
  A ^= MDR;
  Z = A == 0;
  N = A.bit(7);
}

// unstable: sometimes 'AND (H+1)' is dropped,
// page boundary crossings may not work (with
// the high-byte of the value used as the
// high-byte of the address)
auto MOS6502::algorithmSXA() -> void {
  MAR = ((X & ((MAR >> 8) + 1)) << 8) | (MAR & 0xff);
  MDR = MAR >> 8;
}

// unstable: sometimes 'AND (H+1)' is dropped,
// page boundary crossings may not work (with
// the high-byte of the value used as the
// high-byte of the address)
auto MOS6502::algorithmSYA() -> void {
  MAR = ((Y & ((MAR >> 8) + 1)) << 8) | (MAR & 0xff);
  MDR = MAR >> 8;
}

// unstable: sometimes 'AND (H+1)' is dropped,
// page boundary crossings may not work (with
// the high-byte of the value used as the 
// high-byte of the address)
auto MOS6502::algorithmTAS() -> void {
  S = A & X;
  MDR = A & X & (((MAR - Y) >> 8) + 1);
}

// These instructions freeze the CPU.
// The processor will be trapped infinitely
// in T1 phase with $FF on the data bus.
// Reset required.
auto MOS6502::algorithmJAM() -> void {
  PC--;
}
