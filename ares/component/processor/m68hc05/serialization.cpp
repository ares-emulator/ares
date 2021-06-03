auto M68HC05::serialize(serializer& s) -> void {
  s(A);
  s(X);
  s(PC);
  s(SP);
  s(CCR.C);
  s(CCR.Z);
  s(CCR.N);
  s(CCR.I);
  s(CCR.H);
  s(IRQ);
}
