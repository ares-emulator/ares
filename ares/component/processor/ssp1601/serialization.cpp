auto SSP1601::serialize(serializer& s) -> void {
  s(RAM);
  s(FRAME);
  s(X);
  s(Y);
  s(A);
  s(ST);
  s(STACK);
  s(PC);
  s(P);
  s(R);
}
