auto System::serialize(bool synchronize) -> serializer {
  if(synchronize) scheduler.enter(Scheduler::Mode::Synchronize);
  serializer s;

  u32  signature = SerializerSignature;
  char version[16] = {};
  char description[512] = {};
  memory::copy(&version, (const char*)SerializerVersion, SerializerVersion.size());

  s(signature);
  s(synchronize);
  s(version);
  s(description);

  serialize(s, synchronize);
  return s;
}

auto System::unserialize(serializer& s) -> bool {
  u32  signature = 0;
  bool synchronize = true;
  char version[16] = {};
  char description[512] = {};

  s(signature);
  s(synchronize);
  s(version);
  s(description);

  if(signature != SerializerSignature) return false;
  if(string{version} != SerializerVersion) return false;

  if(synchronize) power(/* reset = */ false);
  serialize(s, synchronize);
  return true;
}

auto System::serialize(serializer& s, bool synchronize) -> void {
  scheduler.setSynchronize(synchronize);

  s(random);
  s(cartridge);
  s(cpu);
  s(smp);
  s(ppu);
  s(dsp);

  if(cartridge.has.ICD) s(icd);
  if(cartridge.has.MCC) s(mcc);
  if(cartridge.has.DIP) s(dip);
  if(cartridge.has.Competition) s(competition);
  if(cartridge.has.SA1) s(sa1);
  if(cartridge.has.SuperFX) s(superfx);
  if(cartridge.has.ARMDSP) s(armdsp);
  if(cartridge.has.HitachiDSP) s(hitachidsp);
  if(cartridge.has.NECDSP) s(necdsp);
  if(cartridge.has.EpsonRTC) s(epsonrtc);
  if(cartridge.has.SharpRTC) s(sharprtc);
  if(cartridge.has.SPC7110) s(spc7110);
  if(cartridge.has.SDD1) s(sdd1);
  if(cartridge.has.OBC1) s(obc1);
  if(cartridge.has.MSU1) s(msu1);

  if(cartridge.has.BSMemorySlot) s(bsmemory);
  if(cartridge.has.SufamiTurboSlotA) s(sufamiturboA);
  if(cartridge.has.SufamiTurboSlotB) s(sufamiturboB);

  s(controllerPort1);
  s(controllerPort2);
  s(expansionPort);
}
