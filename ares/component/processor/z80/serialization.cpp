auto Z80::serialize(serializer& s) -> void {
  s((u32&)mosfet);
  s((u32&)prefix);
  s(r.af.word);
  s(r.bc.word);
  s(r.de.word);
  s(r.hl.word);
  s(r.ix.word);
  s(r.iy.word);
  s(r.ir.word);
  s(r.wz.word);
  s(r.sp);
  s(r.pc);
  s(r.af_.word);
  s(r.bc_.word);
  s(r.de_.word);
  s(r.hl_.word);
  s(r.ei);
  s(r.p);
  s(r.q);
  s(r.halt);
  s(r.iff1);
  s(r.iff2);
  s(r.im);
}

auto Z80::Bus::serialize(serializer& s) -> void {
  s(_requested);
  s(_granted);
}
