auto HG51B::disassembleInstruction(maybe<uint15> _pb, maybe<uint8> _pc) -> string {
  uint15 pb = _pb ? *_pb : (uint15)r.pb;
  uint8 pc = _pc ? *_pc : (uint8)r.pc;

  return {};  //todo
}

auto HG51B::disassembleContext() -> string {
  string output;

  output.append("a:", hex(r.a, 6L), " ");
  output.append("p:", hex(r.p, 4L), " ");
  for(uint n : range(16)) {
    output.append("r", n, ":", hex(r.gpr[n], 6L), " ");
  }

  output.append(r.n ? "N" : "n");
  output.append(r.z ? "Z" : "z");
  output.append(r.c ? "C" : "c");
  output.append(r.v ? "V" : "v");
  output.append(r.i ? "I" : "i");

  return output;
}
