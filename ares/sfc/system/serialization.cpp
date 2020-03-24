auto System::serialize(bool synchronize) -> serializer {
  if(synchronize) scheduler.enter(Scheduler::Mode::Synchronize);
  serializer s{information.serializeSize[synchronize]};

  uint signature = 0x31545342;
  uint size = s.capacity();
  char version[16] = {};
  char description[512] = {};
  memory::copy(&version, (const char*)SerializerVersion, SerializerVersion.size());

  s.integer(signature);
  s.integer(size);
  s.integer(synchronize);
  s.array(version);
  s.array(description);
  serializeAll(s, synchronize);
  return s;
}

auto System::unserialize(serializer& s) -> bool {
  uint signature = 0;
  uint size = 0;
  bool synchronize = true;
  char version[16] = {};
  char description[512] = {};

  s.integer(signature);
  s.integer(size);
  s.integer(synchronize);
  s.array(version);
  s.array(description);

  if(signature != 0x31545342) return false;
  if(size != information.serializeSize[synchronize]) return false;
  if(string{version} != SerializerVersion) return false;

  if(synchronize) power(/* reset = */ false);
  serializeAll(s, synchronize);
  return true;
}

//internal

auto System::serialize(serializer& s) -> void {
}

auto System::serializeAll(serializer& s, bool synchronize) -> void {
  scheduler.setSynchronize(synchronize);
  system.serialize(s);
  random.serialize(s);
  cartridge.serialize(s);
  cpu.serialize(s);
  smp.serialize(s);
  ppu.serialize(s);
  dsp.serialize(s);

  if(cartridge.has.ICD) icd.serialize(s);
  if(cartridge.has.MCC) mcc.serialize(s);
  if(cartridge.has.DIP) dip.serialize(s);
  if(cartridge.has.Competition) competition.serialize(s);
  if(cartridge.has.SA1) sa1.serialize(s);
  if(cartridge.has.SuperFX) superfx.serialize(s);
  if(cartridge.has.ARMDSP) armdsp.serialize(s);
  if(cartridge.has.HitachiDSP) hitachidsp.serialize(s);
  if(cartridge.has.NECDSP) necdsp.serialize(s);
  if(cartridge.has.EpsonRTC) epsonrtc.serialize(s);
  if(cartridge.has.SharpRTC) sharprtc.serialize(s);
  if(cartridge.has.SPC7110) spc7110.serialize(s);
  if(cartridge.has.SDD1) sdd1.serialize(s);
  if(cartridge.has.OBC1) obc1.serialize(s);
  if(cartridge.has.MSU1) msu1.serialize(s);

  if(cartridge.has.BSMemorySlot) bsmemory.serialize(s);
  if(cartridge.has.SufamiTurboSlotA) sufamiturboA.serialize(s);
  if(cartridge.has.SufamiTurboSlotB) sufamiturboB.serialize(s);

  controllerPort1.serialize(s);
  controllerPort2.serialize(s);
  expansionPort.serialize(s);
}

auto System::serializeInit(bool synchronize) -> uint {
  serializer s;

  uint signature = 0;
  uint size = 0;
  char version[16] = {};
  char description[512] = {};

  s.integer(signature);
  s.integer(size);
  s.integer(synchronize);
  s.array(version);
  s.array(description);
  serializeAll(s, synchronize);
  return s.size();
}
