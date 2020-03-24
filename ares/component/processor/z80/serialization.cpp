auto Z80::serialize(serializer& s) -> void {
  s.integer((uint&)mosfet);
  s.integer((uint&)prefix);
  s.integer(r.af.word);
  s.integer(r.bc.word);
  s.integer(r.de.word);
  s.integer(r.hl.word);
  s.integer(r.ix.word);
  s.integer(r.iy.word);
  s.integer(r.ir.word);
  s.integer(r.wz.word);
  s.integer(r.sp);
  s.integer(r.pc);
  s.integer(r.af_.word);
  s.integer(r.bc_.word);
  s.integer(r.de_.word);
  s.integer(r.hl_.word);
  s.boolean(r.ei);
  s.boolean(r.p);
  s.boolean(r.q);
  s.boolean(r.halt);
  s.boolean(r.iff1);
  s.boolean(r.iff2);
  s.boolean(r.im);
}

auto Z80::Bus::serialize(serializer& s) -> void {
  s.integer(_requested);
  s.integer(_granted);
}
