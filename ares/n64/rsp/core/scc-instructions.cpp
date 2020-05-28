auto RSP::instructionMFC0() -> void {
  if((RDn & 8) == 0) RT.u32 = rsp.readWord((RDn & 7) << 2);
  if((RDn & 8) != 0) RT.u32 = rdp.readWord((RDn & 7) << 2);
}

auto RSP::instructionMTC0() -> void {
  if((RDn & 8) == 0) rsp.writeWord((RDn & 7) << 2, RT.u32);
  if((RDn & 8) != 0) rdp.writeWord((RDn & 7) << 2, RT.u32);
}
