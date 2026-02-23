auto M68000::serialize(serializer& s) -> void {
  s(r.d);
  s(r.a);
  s(r.sp);
  s(r.pc);

  s(r.c);
  s(r.v);
  s(r.z);
  s(r.n);
  s(r.x);
  s(r.i);
  s(r.s);
  s(r.t);

  s(r.irc);
  s(r.ir);
  s(r.ird);

  s(r.stop);
  s(r.reset);
}
