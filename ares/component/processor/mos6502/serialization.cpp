auto MOS6502::serialize(serializer& s) -> void {
  s(BCD);
  s(A);
  s(X);
  s(Y);
  s(S);
  s(P.c);
  s(P.z);
  s(P.i);
  s(P.d);
  s(P.v);
  s(P.n);
  s(PC);
  s(MDR);
}
