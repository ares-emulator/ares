auto TLCS900H::serialize(serializer& s) -> void {
  for(uint n : range(4)) {
    s.integer(r.xwa[n].l.l0);
    s.integer(r.xbc[n].l.l0);
    s.integer(r.xde[n].l.l0);
    s.integer(r.xhl[n].l.l0);
  }
  s.integer(r.xix.l.l0);
  s.integer(r.xiy.l.l0);
  s.integer(r.xiz.l.l0);
  s.integer(r.xsp.l.l0);
  s.integer(r.pc.l.l0);

  for(uint n : range(4)) {
    s.integer(r.dmas[n].l.l0);
    s.integer(r.dmad[n].l.l0);
    s.integer(r.dmam[n].l.l0);
  }
  s.integer(r.intnest.l.l0);

  s.integer(r.c); s.integer(r.cp);
  s.integer(r.n); s.integer(r.np);
  s.integer(r.v); s.integer(r.vp);
  s.integer(r.h); s.integer(r.hp);
  s.integer(r.z); s.integer(r.zp);
  s.integer(r.s); s.integer(r.sp);
  s.integer(r.rfp);
  s.integer(r.iff);

  s.integer(r.halted);
  s.integer(r.prefix);

  s.integer(p.valid);
  s.integer(p.data);

  s.integer(mar);
  s.integer(mdr);
}
