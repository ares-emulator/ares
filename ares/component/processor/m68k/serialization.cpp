auto M68K::serialize(serializer& s) -> void {
  s.array(r.d);
  s.array(r.a);
  s.integer(r.sp);
  s.integer(r.pc);

  s.integer(r.c);
  s.integer(r.v);
  s.integer(r.z);
  s.integer(r.n);
  s.integer(r.x);
  s.integer(r.i);
  s.integer(r.s);
  s.integer(r.t);

  s.integer(r.irc);
  s.integer(r.ir);
  s.integer(r.ird);

  s.integer(r.stop);
  s.integer(r.reset);
}
