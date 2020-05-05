auto RSP::instructionCFC2() -> void {
  //todo
}

auto RSP::instructionCTC2() -> void {
  //todo
}

auto RSP::instructionLQV() -> void {
  u32 address = RS.u64 + int7(OP) * 16;
  VT.u128 = dmem.readQuad(address);
}

auto RSP::instructionLTV() -> void {
   u32 address = RS.u64 + int7(OP) * 16;
   u32 element = OP >> 8 & 7;
   u32 vt = VTn & ~7;
  v128 vs{dmem.readQuad(address)};
  for(u32 index : range(8)) {
    VPR[vt++].element(index - element & 7) = vs.element(index);
  }
}

auto RSP::instructionLWC2() -> void {
  if(!STATUS_COP2) return exception(CoprocessorUnusable);
  switch(OP >> 11 & 31) {
  case 0x04: return instructionLQV();
  case 0x0b: return instructionLTV();
  }
}

auto RSP::instructionMFC2() -> void {
  //todo
}

auto RSP::instructionMTC2() -> void {
  //todo
}

auto RSP::instructionSQV() -> void {
  u32 address = RS.u64 + int7(OP) * 16;
  dmem.writeQuad(address, VT.u128);
}

auto RSP::instructionSTV() -> void {
   u32 address = RS.u64 + int7(OP) * 16;
   u32 element = OP >> 8 & 7;
   u32 vt = VTn & ~7;
  v128 vs;
  for(u32 index : range(8)) {
    vs.element(index - element & 7) = VPR[vt++].element(index - element & 7);
  }
  dmem.writeQuad(address, vs.u128);
}

auto RSP::instructionSWC2() -> void {
  if(!STATUS_COP2) return exception(CoprocessorUnusable);
  switch(OP >> 11 & 31) {
  case 0x04: return instructionSQV();
  case 0x0a: return instructionSWV();
  case 0x0b: return instructionSTV();
  }
}

auto RSP::instructionSWV() -> void {
   u32 address = RS.u64 + int7(OP) * 16;
   u32 rotate = (OP >> 8 & 7) * 16;
  v128 vt{VPR[VTn].u128};
  v128 vs{vt.u128 << rotate | vt.u128 >> 128 - rotate};
  dmem.writeQuad(address, vs.u128);
}

auto RSP::instructionVMOV() -> void {
  u32 de = OP >> 11 & 31;
  u32 e  = OP >> 21 & 15;
  e = e < 2 ? de & 7 : e < 4 ? e & 1 | de & 6 : e < 8 ? e & 3 | de & 4 : e & 7;
  VD.element(de & 7) = VT.element(e);
  ACCL.u128 = VT.u128;
}
