auto TLCS900H::serialize(serializer& s) -> void {
  for(u32 n : range(4)) {
    s(r.xwa[n].l0);
    s(r.xbc[n].l0);
    s(r.xde[n].l0);
    s(r.xhl[n].l0);
  }
  s(r.xix.l0);
  s(r.xiy.l0);
  s(r.xiz.l0);
  s(r.xsp.l0);
  s(r.pc.l0);

  for(u32 n : range(4)) {
    s(r.dmas[n].l0);
    s(r.dmad[n].l0);
    s(r.dmam[n].l0);
  }
  s(r.intnest.l0);

  s(CF);
  s(NF);
  s(VF);
  s(HF);
  s(ZF);
  s(SF);
  s(CA);
  s(NA);
  s(VA);
  s(HA);
  s(ZA);
  s(SA);
  s(RFP);
  s(IFF);

  s(OP);
  s(HALT);
  s(PIC);
  s(PIQ);
  s(MAR);
  s(MDR);
}
