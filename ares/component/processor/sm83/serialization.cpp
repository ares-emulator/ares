auto SM83::serialize(serializer& s) -> void {
  s(r.af.word);
  s(r.bc.word);
  s(r.de.word);
  s(r.hl.word);
  s(r.sp.word);
  s(r.pc.word);
  s(r.ei);
  s(r.halt);
  s(r.stop);
  s(r.ime);
}
