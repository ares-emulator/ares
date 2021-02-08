auto V30MZ::serialize(serializer& s) -> void {
  s(state.halt);
  s(state.poll);
  s(state.prefix);

  s(opcode);
  if(s.reading()) {
    n8 _prefixes[7] = {};
    n8 _prefixCount = 0;
    s(_prefixCount);
    s(_prefixes);
    prefixes.resize(_prefixCount);
    for(u32 n : range(_prefixCount)) prefixes[n] = _prefixes[n];
  }
  if(s.writing()) {
    n8 _prefixes[7] = {};
    n8 _prefixCount = prefixes.size();
    for(u32 n : range(_prefixCount)) _prefixes[n] = prefixes[n];
    s(_prefixCount);
    s(_prefixes);
  }

  s(modrm.mod);
  s(modrm.reg);
  s(modrm.mem);
  s(modrm.segment);
  s(modrm.address);

  s(r.ax);
  s(r.cx);
  s(r.dx);
  s(r.bx);
  s(r.sp);
  s(r.bp);
  s(r.si);
  s(r.di);
  s(r.es);
  s(r.cs);
  s(r.ss);
  s(r.ds);
  s(r.ip);
  s(r.f.data);
}
