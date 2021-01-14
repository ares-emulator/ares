auto uPD96050::serialize(serializer& s) -> void {
  s(dataRAM);
  s(regs);
  s(flags.a);
  s(flags.b);
}

auto uPD96050::Flag::serialize(serializer& s) -> void {
  s(ov0);
  s(ov1);
  s(z);
  s(c);
  s(s0);
  s(s1);
}

auto uPD96050::Status::serialize(serializer& s) -> void {
  s(p0);
  s(p1);
  s(ei);
  s(sic);
  s(soc);
  s(drc);
  s(dma);
  s(drs);
  s(usf0);
  s(usf1);
  s(rqm);
  s(siack);
  s(soack);
}

auto uPD96050::Registers::serialize(serializer& s) -> void {
  s(stack);
  s(pc);
  s(rp);
  s(dp);
  s(sp);
  s(si);
  s(so);
  s(k);
  s(l);
  s(m);
  s(n);
  s(a);
  s(b);
  s(tr);
  s(trb);
  s(dr);
  s(sr);
}
