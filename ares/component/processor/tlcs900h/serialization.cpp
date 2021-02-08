auto TLCS900H::serialize(serializer& s) -> void {
  for(u32 n : range(4)) {
    s(r.xwa[n].l.l0);
    s(r.xbc[n].l.l0);
    s(r.xde[n].l.l0);
    s(r.xhl[n].l.l0);
  }
  s(r.xix.l.l0);
  s(r.xiy.l.l0);
  s(r.xiz.l.l0);
  s(r.xsp.l.l0);
  s(r.pc.l.l0);

  for(u32 n : range(4)) {
    s(r.dmas[n].l.l0);
    s(r.dmad[n].l.l0);
    s(r.dmam[n].l.l0);
  }
  s(r.intnest.l.l0);

  s(r.c);
  s(r.n);
  s(r.v);
  s(r.h);
  s(r.z);
  s(r.s);
  s(r.cp);
  s(r.np);
  s(r.vp);
  s(r.hp);
  s(r.zp);
  s(r.sp);
  s(r.rfp);
  s(r.iff);

  s(r.halted);
  s(r.prefix);

  s(p.valid);
  s(p.data);

  s(mar);
  s(mdr);
}
