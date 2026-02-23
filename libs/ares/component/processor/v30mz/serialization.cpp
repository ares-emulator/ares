auto V30MZ::serialize(serializer& s) -> void {
  s(state.halt);
  s(state.poll);
  s(state.prefix);
  s(state.interrupt);
  s(state.brk);
  s(state.nmi);

  s(opcode);

  s(prefix.count);
  s(prefix.lock);
  s(prefix.repeat);
  s(prefix.segment);

  s(modrm.mod);
  s(modrm.reg);
  s(modrm.mem);
  s(modrm.segment);
  s(modrm.address);
  s(modrm.useAddress);

  s(AW);
  s(CW);
  s(DW);
  s(BW);
  s(SP);
  s(BP);
  s(IX);
  s(IY);
  s(DS1);
  s(PS);
  s(SS);
  s(DS0);
  s(PC);
  s(PFP);
  s(PF);
  s(PSW.data);
}
