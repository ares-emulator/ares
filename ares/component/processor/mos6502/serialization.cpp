auto MOS6502::serialize(serializer& s) -> void {
  s(BCD);
  s(r.a);
  s(r.x);
  s(r.y);
  s(r.s);
  s(r.pc);
  s(r.p.c);
  s(r.p.z);
  s(r.p.i);
  s(r.p.d);
  s(r.p.v);
  s(r.p.n);
  s(r.mdr);
}
