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

auto Z80::instructionADC_a_irr(uint16& x) -> void { Q = 1;
  A = ADD(A, read(displace(x)), CF);
}

auto Z80::instructionADC_a_n() -> void { Q = 1;
  A = ADD(A, operand(), CF);
}

auto Z80::instructionADC_a_r(uint8& x) -> void { Q = 1;
  A = ADD(A, x, CF);
}

auto Z80::instructionADC_hl_rr(uint16& x) -> void { Q = 1;
  WZ = HL + 1;
  wait(4);
  auto lo = ADD(HL >> 0, x >> 0, CF);
  wait(3);
  auto hi = ADD(HL >> 8, x >> 8, CF);
  HL = hi << 8 | lo << 0;
  ZF = HL == 0;
}

auto Z80::instructionADD_a_irr(uint16& x) -> void { Q = 1;
  A = ADD(A, read(displace(x)));
}

auto Z80::instructionADD_a_n() -> void { Q = 1;
  A = ADD(A, operand());
}

auto Z80::instructionADD_a_r(uint8& x) -> void { Q = 1;
  A = ADD(A, x);
}

auto Z80::instructionADD_hl_rr(uint16& x) -> void { Q = 1;
  WZ = HL + 1;
  bool vf = VF, zf = ZF, sf = SF;
  wait(4);
  auto lo = ADD(HL >> 0, x >> 0);
  wait(3);
  auto hi = ADD(HL >> 8, x >> 8, CF);
  HL = hi << 8 | lo << 0;
  VF = vf, ZF = zf, SF = sf;  //restore unaffected flags
}

auto Z80::instructionAND_a_irr(uint16& x) -> void { Q = 1;
  A = AND(A, read(displace(x)));
}

auto Z80::instructionAND_a_n() -> void { Q = 1;
  A = AND(A, operand());
}

auto Z80::instructionAND_a_r(uint8& x) -> void { Q = 1;
  A = AND(A, x);
}

auto Z80::instructionBIT_o_irr(uint3 bit, uint16& addr) -> void { Q = 1;
  BIT(bit, read(addr));
  XF = WZH.bit(3);
  YF = WZH.bit(5);
}

auto Z80::instructionBIT_o_irr_r(uint3 bit, uint16& addr, uint8& x) -> void { Q = 1;
  x = BIT(bit, read(addr));
  XF = WZH.bit(3);
  YF = WZH.bit(5);
}

auto Z80::instructionBIT_o_r(uint3 bit, uint8& x) -> void { Q = 1;
  BIT(bit, x);
}

auto Z80::instructionCALL_c_nn(bool c) -> void { Q = 0;
  WZ = operands();
  if(!c) return;
  wait(1);
  push(PC);
  PC = WZ;
}

auto Z80::instructionCALL_nn() -> void { Q = 0;
  WZ = operands();
  wait(1);
  push(PC);
  PC = WZ;
}

auto Z80::instructionCCF() -> void {
  if(Q) XF = 0, YF = 0;
  HF = CF;
  CF = !CF;
  NF = 0;
  XF = XF | A.bit(3);
  YF = YF | A.bit(5);
  Q = 1;
}

auto Z80::instructionCP_a_irr(uint16& x) -> void { Q = 1;
  CP(A, read(displace(x)));
}

auto Z80::instructionCP_a_n() -> void { Q = 1;
  CP(A, operand());
}

auto Z80::instructionCP_a_r(uint8& x) -> void { Q = 1;
  CP(A, x);
}

auto Z80::instructionCPD() -> void { Q = 1;
  WZ--;
  uint8 data = read(_HL--);
  wait(5);
  uint8 n = A - data;
  NF = 1;
  VF = --BC != 0;
  HF = uint8(A ^ data ^ n).bit(4);
  XF = uint8(n - HF).bit(3);
  YF = uint8(n - HF).bit(1);
  ZF = n == 0;
  SF = n.bit(7);
}

auto Z80::instructionCPDR() -> void { Q = 1;
  instructionCPD();
  if(!BC || ZF) return;
  wait(5);
  PC -= 2;
  WZ = PC + 1;
}

auto Z80::instructionCPI() -> void { Q = 1;
  WZ++;
  uint8 data = read(_HL++);
  wait(5);
  uint8 n = A - data;
  NF = 1;
  VF = --BC != 0;
  HF = uint8(A ^ data ^ n).bit(4);
  XF = uint8(n - HF).bit(3);
  YF = uint8(n - HF).bit(1);
  ZF = n == 0;
  SF = n.bit(7);
}

auto Z80::instructionCPIR() -> void { Q = 1;
  instructionCPI();
  if(!BC || ZF) return;
  wait(5);
  PC -= 2;
  WZ = PC + 1;
}

auto Z80::instructionCPL() -> void { Q = 1;
  A = ~A;

  NF = 1;
  XF = A.bit(3);
  HF = 1;
  YF = A.bit(5);
}

auto Z80::instructionDAA() -> void { Q = 1;
  auto a = A;
  if(CF || (A.bit(0,7) > 0x99)) { A += NF ? -0x60 : 0x60; CF = 1; }
  if(HF || (A.bit(0,3) > 0x09)) { A += NF ? -0x06 : 0x06; }

  PF = parity(A);
  XF = A.bit(3);
  HF = uint8(A ^ a).bit(4);
  YF = A.bit(5);
  ZF = A == 0;
  SF = A.bit(7);
}

auto Z80::instructionDEC_irr(uint16& x) -> void { Q = 1;
  auto addr = displace(x);
  auto data = read(addr);
  wait(1);
  write(addr, DEC(data));
}

auto Z80::instructionDEC_r(uint8& x) -> void { Q = 1;
  x = DEC(x);
}

auto Z80::instructionDEC_rr(uint16& x) -> void { Q = 0;
  wait(2);
  x--;
}

auto Z80::instructionDI() -> void { Q = 0;
  IFF1 = 0;
  IFF2 = 0;
}

auto Z80::instructionDJNZ_e() -> void { Q = 0;
  wait(1);
  auto displacement = (int8)operand();
  if(!--B) return;
  wait(5);
  WZ = PC + displacement;
  PC = WZ;
}

auto Z80::instructionEI() -> void { Q = 0;
  EI = 1;  //raise IFF1, IFF2 after the next instruction
}

auto Z80::instructionEX_irr_rr(uint16& x, uint16& y) -> void { Q = 0;
  WZL = read(x + 0);
  WZH = read(x + 1);
  write(x + 0, y >> 0);
  write(x + 1, y >> 8);
  y = WZ;
}

auto Z80::instructionEX_rr_rr(uint16& x, uint16& y) -> void { Q = 0;
  swap(x, y);
}

auto Z80::instructionEXX() -> void { Q = 0;
  swap(BC, BC_);
  swap(DE, DE_);
  swap(_HL, HL_);
}

auto Z80::instructionHALT() -> void { Q = 0;
  HALT = 1;
}

auto Z80::instructionIM_o(uint2 code) -> void { Q = 0;
  wait(4);
  IM = code;
}

auto Z80::instructionIN_a_in() -> void { Q = 0;
  WZL = operand();
  WZH = A;
  A = in(WZ++);
}

auto Z80::instructionIN_r_ic(uint8& x) -> void { Q = 1;
  x = IN(in(BC));
  WZ = BC + 1;
}

auto Z80::instructionIN_ic() -> void { Q = 1;
  IN(in(BC));
  WZ = BC + 1;
}

auto Z80::instructionINC_irr(uint16& x) -> void { Q = 1;
  auto addr = displace(x);
  auto data = read(addr);
  wait(1);
  write(addr, INC(data));
}

auto Z80::instructionINC_r(uint8& x) -> void { Q = 1;
  x = INC(x);
}

auto Z80::instructionINC_rr(uint16& x) -> void { Q = 0;
  wait(2);
  x++;
}

auto Z80::instructionIND() -> void { Q = 1;
  WZ = BC - 1;
  B--;
  wait(1);
  auto data = in(BC);
  write(_HL--, data);
  CF = uint9(uint8(C - 1) + data).bit(8);
  NF = data.bit(7);
  PF = parity(uint8(C - 1) + data & 7 ^ B);
  XF = B.bit(3);
  HF = CF;
  YF = B.bit(5);
  ZF = B == 0;
  SF = B.bit(7);
}

auto Z80::instructionINDR() -> void { Q = 1;
  instructionIND();
  if(!B) return;
  wait(5);
  PC -= 2;
}

auto Z80::instructionINI() -> void { Q = 1;
  WZ = BC + 1;
  B--;
  wait(1);
  auto data = in(BC);
  write(_HL++, data);
  CF = uint9(uint8(C + 1) + data).bit(8);
  NF = data.bit(7);
  PF = parity(uint8(C + 1) + data & 7 ^ B);
  XF = B.bit(3);
  HF = CF;
  YF = B.bit(5);
  ZF = B == 0;
  SF = B.bit(7);
}

auto Z80::instructionINIR() -> void { Q = 1;
  instructionINI();
  if(!B) return;
  wait(5);
  PC -= 2;
}

auto Z80::instructionJP_c_nn(bool c) -> void { Q = 0;
  WZ = operands();
  if(c) PC = WZ;
}

auto Z80::instructionJP_rr(uint16& x) -> void { Q = 0;
  PC = x;
}

auto Z80::instructionJR_c_e(bool c) -> void { Q = 0;
  auto displacement = (int8)operand();
  if(!c) return;
  wait(5);
  WZ = PC + displacement;
  PC = WZ;
}

auto Z80::instructionLD_a_inn() -> void { Q = 0;
  WZ = operands();
  A = read(WZ++);
}

auto Z80::instructionLD_a_irr(uint16& x) -> void { Q = 0;
  WZ = x;
  A = read(displace(WZ));
  WZ++;
}

auto Z80::instructionLD_inn_a() -> void { Q = 0;
  WZ = operands();
  write(WZ++, A);
  WZH = A;
}

auto Z80::instructionLD_inn_rr(uint16& x) -> void { Q = 0;
  WZ = operands();
  write(WZ + 0, x >> 0);
  write(WZ + 1, x >> 8);
  WZ++;
}

auto Z80::instructionLD_irr_a(uint16& x) -> void { Q = 0;
  WZ = x;
  write(displace(WZ), A);
  WZL++;
  WZH = A;
}

auto Z80::instructionLD_irr_n(uint16& x) -> void { Q = 0;
  auto addr = displace(x);
  write(addr, operand());
}

auto Z80::instructionLD_irr_r(uint16& x, uint8& y) -> void { Q = 0;
  write(displace(x), y);
}

auto Z80::instructionLD_r_n(uint8& x) -> void { Q = 0;
  x = operand();
}

auto Z80::instructionLD_r_irr(uint8& x, uint16& y) -> void { Q = 0;
  x = read(displace(y));
}

auto Z80::instructionLD_r_r(uint8& x, uint8& y) -> void { Q = 0;
  x = y;
}

//LD to/from I/R requires an extra T-cycle
auto Z80::instructionLD_r_r1(uint8& x, uint8& y) -> void { Q = 0;
  wait(1);
  x = y;
}

//LD from I/R sets status flags
auto Z80::instructionLD_r_r2(uint8& x, uint8& y) -> void { Q = 1;
  wait(1);
  x = y;
  NF = 0;
  PF = IFF2;
  XF = x.bit(3);
  HF = 0;
  YF = x.bit(5);
  ZF = x == 0;
  SF = x.bit(7);
  P = mosfet == MOSFET::NMOS;
}

auto Z80::instructionLD_rr_inn(uint16& x) -> void { Q = 0;
  auto addr = operands();
  x.byte(0) = read(addr + 0);
  x.byte(1) = read(addr + 1);
}

auto Z80::instructionLD_rr_nn(uint16& x) -> void { Q = 0;
  x = operands();
}

auto Z80::instructionLD_sp_rr(uint16& x) -> void { Q = 0;
  wait(2);
  SP = x;
}

auto Z80::instructionLDD() -> void { Q = 1;
  auto data = read(_HL--);
  write(DE--, data);
  wait(2);
  NF = 0;
  VF = --BC != 0;
  XF = uint8(A + data).bit(3);
  HF = 0;
  YF = uint8(A + data).bit(1);
}

auto Z80::instructionLDDR() -> void { Q = 1;
  instructionLDD();
  if(!BC) return;
  wait(5);
  PC -= 2;
  WZ = PC + 1;
}

auto Z80::instructionLDI() -> void { Q = 1;
  auto data = read(_HL++);
  write(DE++, data);
  wait(2);
  NF = 0;
  VF = --BC != 0;
  XF = uint8(A + data).bit(3);
  HF = 0;
  YF = uint8(A + data).bit(1);
}

auto Z80::instructionLDIR() -> void { Q = 1;
  instructionLDI();
  if(!BC) return;
  wait(5);
  PC -= 2;
  WZ = PC + 1;
}

auto Z80::instructionNEG() -> void { Q = 1;
  A = SUB(0, A);
}

auto Z80::instructionNOP() -> void { Q = 0;
}

auto Z80::instructionOR_a_irr(uint16& x) -> void { Q = 1;
  A = OR(A, read(displace(x)));
}

auto Z80::instructionOR_a_n() -> void { Q = 1;
  A = OR(A, operand());
}

auto Z80::instructionOR_a_r(uint8& x) -> void { Q = 1;
  A = OR(A, x);
}

auto Z80::instructionOTDR() -> void { Q = 1;
  instructionOUTD();
  if(!B) return;
  wait(5);
  PC -= 2;
}

auto Z80::instructionOTIR() -> void { Q = 1;
  instructionOUTI();
  if(!B) return;
  wait(5);
  PC -= 2;
}

auto Z80::instructionOUT_ic_r(uint8& x) -> void { Q = 0;
  out(BC, x);
  WZ = BC + 1;
}

auto Z80::instructionOUT_ic() -> void { Q = 0;
  if(mosfet == MOSFET::NMOS) out(BC, 0x00);
  if(mosfet == MOSFET::CMOS) out(BC, 0xff);
}

auto Z80::instructionOUT_in_a() -> void { Q = 0;
  WZL = operand();
  WZH = A;
  out(WZ, A);
  WZL++;
}

auto Z80::instructionOUTD() -> void { Q = 1;
  wait(1);
  auto data = read(_HL--);
  out(BC, data);
  B--;
  WZ = BC - 1;
  CF = uint9(L + data).bit(8);
  NF = data.bit(7);
  PF = parity(L + data & 7 ^ B);
  XF = B.bit(3);
  HF = CF;
  YF = B.bit(5);
  ZF = B == 0;
  SF = B.bit(7);
}

auto Z80::instructionOUTI() -> void { Q = 1;
  wait(1);
  auto data = read(_HL++);
  out(BC, data);
  B--;
  WZ = BC + 1;
  CF = uint9(L + data).bit(8);
  NF = data.bit(7);
  PF = parity(L + data & 7 ^ B);
  XF = B.bit(3);
  HF = CF;
  YF = B.bit(5);
  ZF = B == 0;
  SF = B.bit(7);
}

//note: even though "pop af" affects flags, it does not set Q
auto Z80::instructionPOP_rr(uint16& x) -> void { Q = 0;
  x = pop();
}

auto Z80::instructionPUSH_rr(uint16& x) -> void { Q = 0;
  wait(1);
  push(x);
}

auto Z80::instructionRES_o_irr(uint3 bit, uint16& addr) -> void { Q = 1;
  write(addr, RES(bit, read(addr)));
}

auto Z80::instructionRES_o_irr_r(uint3 bit, uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = RES(bit, read(addr)));
}

auto Z80::instructionRES_o_r(uint3 bit, uint8& x) -> void { Q = 1;
  x = RES(bit, x);
}

auto Z80::instructionRET() -> void { Q = 0;
  wait(1);
  WZ = pop();
  PC = WZ;
}

auto Z80::instructionRET_c(bool c) -> void { Q = 0;
  wait(1);
  if(!c) return;
  WZ = pop();
  PC = WZ;
}

auto Z80::instructionRETI() -> void { Q = 0;
  WZ = pop();
  PC = WZ;
  IFF1 = IFF2;
}

auto Z80::instructionRETN() -> void { Q = 0;
  WZ = pop();
  PC = WZ;
  IFF1 = IFF2;
}

auto Z80::instructionRL_irr(uint16& addr) -> void { Q = 1;
  write(addr, RL(read(addr)));
}

auto Z80::instructionRL_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = RL(read(addr)));
}

auto Z80::instructionRL_r(uint8& x) -> void { Q = 1;
  x = RL(x);
}

auto Z80::instructionRLA() -> void { Q = 1;
  bool c = A.bit(7);
  A = A << 1 | CF;

  CF = c;
  NF = 0;
  XF = A.bit(3);
  HF = 0;
  YF = A.bit(5);
}

auto Z80::instructionRLC_irr(uint16& addr) -> void { Q = 1;
  write(addr, RLC(read(addr)));
}

auto Z80::instructionRLC_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = RLC(read(addr)));
}

auto Z80::instructionRLC_r(uint8& x) -> void { Q = 1;
  x = RLC(x);
}

auto Z80::instructionRLCA() -> void { Q = 1;
  bool c = A.bit(7);
  A = A << 1 | c;

  CF = c;
  NF = 0;
  XF = A.bit(3);
  HF = 0;
  YF = A.bit(5);
}

auto Z80::instructionRLD() -> void { Q = 1;
  WZ = HL + 1;
  auto data = read(HL);
  wait(1);
  write(HL, (data << 4) | (A & 0x0f));
  wait(3);
  A = (A & 0xf0) | (data >> 4);

  NF = 0;
  PF = parity(A);
  XF = A.bit(3);
  HF = 0;
  YF = A.bit(5);
  ZF = A == 0;
  SF = A.bit(7);
}

auto Z80::instructionRR_irr(uint16& addr) -> void { Q = 1;
  write(addr, RR(read(addr)));
}

auto Z80::instructionRR_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = RR(read(addr)));
}

auto Z80::instructionRR_r(uint8& x) -> void { Q = 1;
  x = RR(x);
}

auto Z80::instructionRRA() -> void { Q = 1;
  bool c = A.bit(0);
  A = CF << 7 | A >> 1;

  CF = c;
  NF = 0;
  XF = A.bit(3);
  HF = 0;
  YF = A.bit(5);
}

auto Z80::instructionRRC_irr(uint16& addr) -> void { Q = 1;
  write(addr, RRC(read(addr)));
}

auto Z80::instructionRRC_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = RRC(read(addr)));
}

auto Z80::instructionRRC_r(uint8& x) -> void { Q = 1;
  x = RRC(x);
}

auto Z80::instructionRRCA() -> void { Q = 1;
  bool c = A.bit(0);
  A = c << 7 | A >> 1;

  CF = c;
  NF = 0;
  XF = A.bit(3);
  HF = 0;
  YF = A.bit(5);
}

auto Z80::instructionRRD() -> void { Q = 1;
  WZ = HL + 1;
  auto data = read(HL);
  wait(1);
  write(HL, (data >> 4) | (A << 4));
  wait(3);
  A = (A & 0xf0) | (data & 0x0f);

  NF = 0;
  PF = parity(A);
  XF = A.bit(3);
  HF = 0;
  YF = A.bit(5);
  ZF = A == 0;
  SF = A.bit(7);
}

auto Z80::instructionRST_o(uint3 vector) -> void { Q = 0;
  wait(1);
  push(PC);
  WZ = vector << 3;
  PC = WZ;
}

auto Z80::instructionSBC_a_irr(uint16& x) -> void { Q = 1;
  A = SUB(A, read(displace(x)), CF);
}

auto Z80::instructionSBC_a_n() -> void { Q = 1;
  A = SUB(A, operand(), CF);
}

auto Z80::instructionSBC_a_r(uint8& x) -> void { Q = 1;
  A = SUB(A, x, CF);
}

auto Z80::instructionSBC_hl_rr(uint16& x) -> void { Q = 1;
  WZ = HL + 1;
  wait(4);
  auto lo = SUB(HL >> 0, x >> 0, CF);
  wait(3);
  auto hi = SUB(HL >> 8, x >> 8, CF);
  HL = hi << 8 | lo << 0;
  ZF = HL == 0;
}

auto Z80::instructionSCF() -> void {
  if(Q) XF = 0, YF = 0;
  CF = 1;
  NF = 0;
  XF = XF | A.bit(3);
  HF = 0;
  YF = YF | A.bit(5);
  Q = 1;
}

auto Z80::instructionSET_o_irr(uint3 bit, uint16& addr) -> void { Q = 1;
  write(addr, SET(bit, read(addr)));
}

auto Z80::instructionSET_o_irr_r(uint3 bit, uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = SET(bit, read(addr)));
}

auto Z80::instructionSET_o_r(uint3 bit, uint8& x) -> void { Q = 1;
  x = SET(bit, x);
}

auto Z80::instructionSLA_irr(uint16& addr) -> void { Q = 1;
  write(addr, SLA(read(addr)));
}

auto Z80::instructionSLA_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = SLA(read(addr)));
}

auto Z80::instructionSLA_r(uint8& x) -> void { Q = 1;
  x = SLA(x);
}

auto Z80::instructionSLL_irr(uint16& addr) -> void { Q = 1;
  write(addr, SLL(read(addr)));
}

auto Z80::instructionSLL_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = SLL(read(addr)));
}

auto Z80::instructionSLL_r(uint8& x) -> void { Q = 1;
  x = SLL(x);
}

auto Z80::instructionSRA_irr(uint16& addr) -> void { Q = 1;
  write(addr, SRA(read(addr)));
}

auto Z80::instructionSRA_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = SRA(read(addr)));
}

auto Z80::instructionSRA_r(uint8& x) -> void { Q = 1;
  x = SRA(x);
}

auto Z80::instructionSRL_irr(uint16& addr) -> void { Q = 1;
  write(addr, SRL(read(addr)));
}

auto Z80::instructionSRL_irr_r(uint16& addr, uint8& x) -> void { Q = 1;
  write(addr, x = SRL(read(addr)));
}

auto Z80::instructionSRL_r(uint8& x) -> void { Q = 1;
  x = SRL(x);
}

auto Z80::instructionSUB_a_irr(uint16& x) -> void { Q = 1;
  A = SUB(A, read(displace(x)));
}

auto Z80::instructionSUB_a_n() -> void { Q = 1;
  A = SUB(A, operand());
}

auto Z80::instructionSUB_a_r(uint8& x) -> void { Q = 1;
  A = SUB(A, x);
}

auto Z80::instructionXOR_a_irr(uint16& x) -> void { Q = 1;
  A = XOR(A, read(displace(x)));
}

auto Z80::instructionXOR_a_n() -> void { Q = 1;
  A = XOR(A, operand());
}

auto Z80::instructionXOR_a_r(uint8& x) -> void { Q = 1;
  A = XOR(A, x);
}
