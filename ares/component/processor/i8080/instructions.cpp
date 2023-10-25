//legend:
//  a   = register A
//  c   = condition
//  e   = relative operand
//  in  = (operand)
//  inn = (operand-word)
//  irr = (register-word)
//  o   = opcode bits
//  n   = operand
//  nn  = operand-word
//  r   = register

auto I8080::instructionADC_a_irr(n16& x) -> void { Q = 1;
  A = ADD(A, read(x), CF);
}

auto I8080::instructionADC_a_n() -> void { Q = 1;
  A = ADD(A, operand(), CF);
}

auto I8080::instructionADC_a_r(n8& x) -> void { Q = 1;
  A = ADD(A, x, CF);
}

auto I8080::instructionADD_a_irr(n16& x) -> void { Q = 1;
  A = ADD(A, read(x));
}

auto I8080::instructionADD_a_n() -> void { Q = 1;
  A = ADD(A, operand());
}

auto I8080::instructionADD_a_r(n8& x) -> void { Q = 1;
  A = ADD(A, x);
}

auto I8080::instructionADD_hl_rr(n16& x) -> void { Q = 1;
  WZ = HL + 1;
  bool pf = PF, zf = ZF, sf = SF, hf = HF;
  wait(3);
  auto lo = ADD(HL >> 0, x >> 0);
  wait(4);
  auto hi = ADD(HL >> 8, x >> 8, CF);
  HL = hi << 8 | lo << 0;
  PF = pf, ZF = zf, SF = sf, HF = hf;  //restore unaffected flags
}

auto I8080::instructionAND_a_irr(n16& x) -> void { Q = 1;
  A = AND(A, read(x));
}

auto I8080::instructionAND_a_n() -> void { Q = 1;
  A = AND(A, operand());
}

auto I8080::instructionAND_a_r(n8& x) -> void { Q = 1;
  A = AND(A, x);
}

auto I8080::instructionCALL_c_nn(bool c) -> void { Q = 0;
  WZ = operands();
  if(!c) return;
  wait(1);
  push(PC);
  PC = WZ;
}

auto I8080::instructionCCF() -> void {
  CF = !CF;
  Q = 1;
}

auto I8080::instructionCP_a_irr(n16& x) -> void { Q = 1;
  CP(A, read(x));
}

auto I8080::instructionCP_a_n() -> void { Q = 1;
  CP(A, operand());
}

auto I8080::instructionCP_a_r(n8& x) -> void { Q = 1;
  CP(A, x);
}

auto I8080::instructionCPL() -> void { Q = 1;
  A = ~A;
}

auto I8080::instructionDAA() -> void { Q = 1;
  auto a = A;
  if(CF || (A.bit(0,7) > 0x99)) { A += 0x60; CF = 1; }
  if(HF || (A.bit(0,3) > 0x09)) { A += 0x06; }

  PF = parity(A);
  HF = n8(A ^ a).bit(4);
  ZF = A == 0;
  SF = A.bit(7);
}

auto I8080::instructionDEC_irr(n16& x) -> void { Q = 1;
  auto addr = x;
  auto data = read(addr);
  wait(1);
  write(addr, DEC(data));
}

auto I8080::instructionDEC_r(n8& x) -> void { Q = 1;
  x = DEC(x);
}

auto I8080::instructionDEC_rr(n16& x) -> void { Q = 0;
  wait(2);
  x--;
}

auto I8080::instructionDI() -> void { Q = 0;
  INTE = 0;
}

auto I8080::instructionEI() -> void { Q = 0;
  INTE = 1;
  EI = 1;  //ignore maskable interrupts until after the next instruction
}

auto I8080::instructionEX_irr_rr(n16& x, n16& y) -> void { Q = 0;
  WZL = read(x + 0);
  WZH = read(x + 1);
  wait(1);
  write(x + 0, y >> 0);
  write(x + 1, y >> 8);
  wait(2);
  y = WZ;
}

auto I8080::instructionEX_rr_rr(n16& x, n16& y) -> void { Q = 0;
  swap(x, y);
}

auto I8080::instructionHALT() -> void { Q = 0;
  HALT = 1;
}

auto I8080::instructionIN_a_in() -> void { Q = 0;
  WZL = operand();
  WZH = A;
  A = in(WZ++);
}

auto I8080::instructionINC_irr(n16& x) -> void { Q = 1;
  auto addr = x;
  auto data = read(addr);
  wait(1);
  write(addr, INC(data));
}

auto I8080::instructionINC_r(n8& x) -> void { Q = 1;
  x = INC(x);
}

auto I8080::instructionINC_rr(n16& x) -> void { Q = 0;
  wait(2);
  x++;
}

auto I8080::instructionJP_c_nn(bool c) -> void { Q = 0;
  WZ = operands();
  if(c) PC = WZ;
}

auto I8080::instructionJP_rr(n16& x) -> void { Q = 0;
  PC = x;
}

auto I8080::instructionLD_a_inn() -> void { Q = 0;
  WZ = operands();
  A = read(WZ++);
}

auto I8080::instructionLD_a_irr(n16& x) -> void { Q = 0;
  WZ = x;
  A = read(x);
  WZ++;
}

auto I8080::instructionLD_inn_a() -> void { Q = 0;
  WZ = operands();
  write(WZ++, A);
  WZH = A;
}

auto I8080::instructionLD_inn_rr(n16& x) -> void { Q = 0;
  WZ = operands();
  write(WZ + 0, x >> 0);
  write(WZ + 1, x >> 8);
  WZ++;
}

auto I8080::instructionLD_irr_a(n16& x) -> void { Q = 0;
  WZ = x;
  write(x, A);
  WZL++;
  WZH = A;
}

auto I8080::instructionLD_irr_n(n16& x) -> void { Q = 0;
  auto addr = x;
  auto data = operand();
  if(&x == &ix.word || &x == &iy.word) wait(2);
  write(addr, data);
}

auto I8080::instructionLD_irr_r(n16& x, n8& y) -> void { Q = 0;
  write(x, y);
}

auto I8080::instructionLD_r_n(n8& x) -> void { Q = 0;
  x = operand();
}

auto I8080::instructionLD_r_irr(n8& x, n16& y) -> void { Q = 0;
  x = read(y);
}

auto I8080::instructionLD_r_r(n8& x, n8& y) -> void { Q = 0;
  x = y;
}

auto I8080::instructionLD_rr_inn(n16& x) -> void { Q = 0;
  WZ = operands();
  x.byte(0) = read(WZ);
  WZ++;
  x.byte(1) = read(WZ);
}

auto I8080::instructionLD_rr_nn(n16& x) -> void { Q = 0;
  x = operands();
}

auto I8080::instructionLD_sp_rr(n16& x) -> void { Q = 0;
  wait(2);
  SP = x;
}

auto I8080::instructionNOP() -> void { Q = 0;
}

auto I8080::instructionOR_a_irr(n16& x) -> void { Q = 1;
  A = OR(A, read(x));
}

auto I8080::instructionOR_a_n() -> void { Q = 1;
  A = OR(A, operand());
}

auto I8080::instructionOR_a_r(n8& x) -> void { Q = 1;
  A = OR(A, x);
}

auto I8080::instructionOUT_in_a() -> void { Q = 0;
  WZL = operand();
  WZH = A;
  out(WZ, A);
  WZL++;
}

//note: even though "pop af" affects flags, it does not set Q
auto I8080::instructionPOP_rr(n16& x) -> void { Q = 0;
  x = pop();
  if(&x == &af.word) F = (F & 0xd7) | 2;
}

auto I8080::instructionPUSH_rr(n16& x) -> void { Q = 0;
  wait(1);
  if(&x == &af.word) F = (F & 0xd7) | 2;
  push(x);
}

auto I8080::instructionRET() -> void { Q = 0;
  WZ = pop();
  PC = WZ;
}

auto I8080::instructionRET_c(bool c) -> void { Q = 0;
  wait(1);
  if(!c) return;
  WZ = pop();
  PC = WZ;
}

auto I8080::instructionRLA() -> void { Q = 1;
  bool c = A.bit(7);
  A = A << 1 | CF;

  CF = c;
}

auto I8080::instructionRLCA() -> void { Q = 1;
  bool c = A.bit(7);
  A = A << 1 | c;

  CF = c;
}

auto I8080::instructionRRA() -> void { Q = 1;
  bool c = A.bit(0);
  A = CF << 7 | A >> 1;

  CF = c;
}

auto I8080::instructionRRCA() -> void { Q = 1;
  bool c = A.bit(0);
  A = c << 7 | A >> 1;

  CF = c;
}

auto I8080::instructionRST_o(n3 vector) -> void { Q = 0;
  wait(1);
  push(PC);
  WZ = vector << 3;
  PC = WZ;
}

auto I8080::instructionSBC_a_irr(n16& x) -> void { Q = 1;
  A = SUB(A, read(x), CF);
}

auto I8080::instructionSBC_a_n() -> void { Q = 1;
  A = SUB(A, operand(), CF);
}

auto I8080::instructionSBC_a_r(n8& x) -> void { Q = 1;
  A = SUB(A, x, CF);
}

auto I8080::instructionSCF() -> void {
  CF = 1;
  Q = 1;
}

auto I8080::instructionSUB_a_irr(n16& x) -> void { Q = 1;
  A = SUB(A, read(x));
}

auto I8080::instructionSUB_a_n() -> void { Q = 1;
  A = SUB(A, operand());
}

auto I8080::instructionSUB_a_r(n8& x) -> void { Q = 1;
  A = SUB(A, x);
}

auto I8080::instructionXOR_a_irr(n16& x) -> void { Q = 1;
  A = XOR(A, read(x));
}

auto I8080::instructionXOR_a_n() -> void { Q = 1;
  A = XOR(A, operand());
}

auto I8080::instructionXOR_a_r(n8& x) -> void { Q = 1;
  A = XOR(A, x);
}
