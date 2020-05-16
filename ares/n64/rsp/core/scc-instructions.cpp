auto RSP::instructionMFC0() -> void {
  if((RDn & 8) == 0) RT.u32 = rsp.readSCC((RDn & 7) << 2);
  if((RDn & 8) != 0) RT.u32 = rdp.readSCC((RDn & 7) << 2);
}

auto RSP::instructionMTC0() -> void {
  if((RDn & 8) == 0) rsp.writeSCC((RDn & 7) << 2, RT.u32);
  if((RDn & 8) != 0) rdp.writeSCC((RDn & 7) << 2, RT.u32);
}
