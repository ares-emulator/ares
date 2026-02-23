#define ALU(...) (this->*fp)(__VA_ARGS__)

auto M68HC05::instructionBCLR(n3 bit) -> void {
  n8 d = fetch();
  n8 r = load(d);
  step(1);
  r &= ~(1 << bit);
  store(d, r);
}

auto M68HC05::instructionBRA(b1 c) -> void {
  i8 r = fetch();
  step(1);
  if(c) PC = PC + r;
}

auto M68HC05::instructionBRCLR(n3 bit) -> void {
  n8 d = fetch();
  i8 r = fetch();
  n8 b = load(d);
  step(1);
  if((b & 1 << bit) == 0) PC = PC + r;
}

auto M68HC05::instructionBRSET(n3 bit) -> void {
  n8 d = fetch();
  i8 r = fetch();
  n8 b = load(d);
  step(1);
  if((b & 1 << bit) != 0) PC = PC + r;
}

auto M68HC05::instructionBSET(n3 bit) -> void {
  n8 d = fetch();
  n8 r = load(d);
  step(1);
  r |= 1 << bit;
  store(d, r);
}

auto M68HC05::instructionBSR() -> void {
  i8 r = fetch();
  push<n16>(PC - 1);
  step(2);
  PC = PC + r;
}

auto M68HC05::instructionCLR(b1& f) -> void {
  step(1);
  f = 0;
}

auto M68HC05::instructionILL() -> void {
  step(1);
}

auto M68HC05::instructionJMPa() -> void {
  n16 a = fetch<n16>();
  PC = a;
}

auto M68HC05::instructionJMPd() -> void {
  n8 d = fetch();
  PC = d;
}

auto M68HC05::instructionJMPi() -> void {
  step(1);
  PC = X;
}

auto M68HC05::instructionJMPo() -> void {
  n8 d = fetch();
  step(1);
  PC = X + d;
}

auto M68HC05::instructionJMPw() -> void {
  n16 a = fetch<n16>();
  step(1);
  PC = X + a;
}

auto M68HC05::instructionJSRa() -> void {
  n16 a = fetch<n16>();
  push<n16>(PC - 1);
  step(1);
  PC = a;
}

auto M68HC05::instructionJSRd() -> void {
  n8 d = fetch();
  push<n16>(PC - 1);
  step(1);
  PC = d;
}

auto M68HC05::instructionJSRi() -> void {
  step(1);
  push<n16>(PC - 1);
  step(1);
  PC = X;
}

auto M68HC05::instructionJSRo() -> void {
  step(1);
  n8 d = fetch();
  push<n16>(PC - 1);
  step(1);
  PC = X + d;
}

auto M68HC05::instructionJSRw() -> void {
  step(1);
  n16 a = fetch<n16>();
  push<n16>(PC - 1);
  step(1);
  PC = X + a;
}

auto M68HC05::instructionLDa(f2 fp, n8& r) -> void {
  n16 a = fetch<n16>();
  n8 b = load(a);
  r = ALU(r, b);
}

auto M68HC05::instructionLDd(f2 fp, n8& r) -> void {
  n8 d = fetch();
  n8 b = load(d);
  r = ALU(r, b);
}

auto M68HC05::instructionLDi(f2 fp, n8& r) -> void {
  n8 b = load(X);
  step(1);
  r = ALU(r, b);
}

auto M68HC05::instructionLDo(f2 fp, n8& r) -> void {
  n8 d = fetch();
  n8 b = load(X + d);
  step(1);
  r = ALU(r, b);
}

auto M68HC05::instructionLDr(f2 fp, n8& r) -> void {
  n8 b = fetch();
  r = ALU(r, b);
}

auto M68HC05::instructionLDw(f2 fp, n8& r) -> void {
  n16 a = fetch<n16>();
  n8 b = load(X + a);
  step(1);
  r = ALU(r, b);
}

auto M68HC05::instructionMUL() -> void {
  step(10);
  n16 m = A * X;
  A = m >> 0;
  X = m >> 8;
  CCR.C = 0;
  CCR.H = 0;
}

auto M68HC05::instructionNOP() -> void {
  step(1);
}

auto M68HC05::instructionRMWd(f1 fp) -> void {
  n8 d = fetch();
  n8 r = load(d);
  step(1);
  store(d, ALU(r));
}

auto M68HC05::instructionRMWi(f1 fp) -> void {
  step(1);
  n8 r = load(X);
  step(1);
  store(X, ALU(r));
}

auto M68HC05::instructionRMWo(f1 fp) -> void {
  step(1);
  n8 d = fetch();
  n8 r = load(X + d);
  step(1);
  store(d, ALU(r));
}

auto M68HC05::instructionRMWr(f1 fp, n8& r) -> void {
  step(2);
  r = ALU(r);
}

auto M68HC05::instructionRSP() -> void {
  step(1);
  SP = 0xff;
}

auto M68HC05::instructionRTI() -> void {
  CCR = pop();
  PC = pop<n16>();
  step(5);
}

auto M68HC05::instructionRTS() -> void {
  PC = pop<n16>() + 1;
  step(3);
}

auto M68HC05::instructionSET(b1& f) -> void {
  step(1);
  f = 1;
}

auto M68HC05::instructionSTa(n8& r) -> void {
  n16 a = fetch<n16>();
  step(1);
  store(a, r);
}

auto M68HC05::instructionSTd(n8& r) -> void {
  n8 d = fetch();
  step(1);
  store(d, r);
}

auto M68HC05::instructionSTi(n8& r) -> void {
  step(2);
  store(X, r);
}

auto M68HC05::instructionSTo(n8& r) -> void {
  n8 d = fetch();
  step(2);
  store(X + d, r);
}

auto M68HC05::instructionSTw(n8& r) -> void {
  n16 a = fetch<n16>();
  step(2);
  store(X + a, r);
}

auto M68HC05::instructionSTOP() -> void {
  step(1);
  PC = PC - 1;
}

auto M68HC05::instructionSWI() -> void {
  push<n16>(PC);
  push<n8>(CCR);
  step(6);
  CCR.I = 1;
  PC = 0;
}

auto M68HC05::instructionTAX() -> void {
  step(1);
  X = A;
}

auto M68HC05::instructionTSTd(f1 fp) -> void {
  n8 d = fetch();
  n8 r = load(d);
  step(1);
  ALU(r);
}

auto M68HC05::instructionTSTi(f1 fp) -> void {
  step(1);
  n8 r = load(X);
  step(1);
  ALU(r);
}

auto M68HC05::instructionTSTo(f1 fp) -> void {
  step(1);
  n8 d = fetch();
  n8 r = load(X + d);
  step(1);
  ALU(r);
}

auto M68HC05::instructionTSTr(f1 fp, n8& r) -> void {
  step(2);
  ALU(r);
}

auto M68HC05::instructionTXA() -> void {
  step(1);
  A = X;
}

auto M68HC05::instructionWAIT() -> void {
  step(1);
  if(!IRQ) PC = PC - 1;
}

#undef call
