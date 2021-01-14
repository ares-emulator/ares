auto SPC700::serialize(serializer& s) -> void {
  s(r.pc.w);
  s(r.ya.w);
  s(r.x);
  s(r.s);

  s(r.p.c);
  s(r.p.z);
  s(r.p.i);
  s(r.p.h);
  s(r.p.b);
  s(r.p.p);
  s(r.p.v);
  s(r.p.n);

  s(r.wait);
  s(r.stop);
}
