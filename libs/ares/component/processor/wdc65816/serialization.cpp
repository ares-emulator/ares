auto WDC65816::serialize(serializer& s) -> void {
  s(r.pc.d);

  s(r.a.w);
  s(r.x.w);
  s(r.y.w);
  s(r.z.w);
  s(r.s.w);
  s(r.d.w);
  s(r.b);

  s(r.p.c);
  s(r.p.z);
  s(r.p.i);
  s(r.p.d);
  s(r.p.x);
  s(r.p.m);
  s(r.p.v);
  s(r.p.n);

  s(r.e);
  s(r.irq);
  s(r.wai);
  s(r.stp);

  s(r.vector);
  s(r.mar);
  s(r.mdr);

  s(r.u.d);
  s(r.v.d);
  s(r.w.d);
}
