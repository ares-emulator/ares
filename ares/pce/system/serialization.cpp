static const string SerializerVersion = "v133";

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
  s(vdp.accurate);

  serialize(s, synchronize);
  return s;
}

auto System::unserialize(serializer& s) -> bool {
  u32  signature = 0;
  bool synchronize = true;
  char version[16] = {};
  char description[512] = {};
  bool vdpAccurate = false;

  s(signature);
  s(synchronize);
  s(version);
  s(description);
  s(vdpAccurate);

  if(signature != SerializerSignature) return false;
  if(string{version} != SerializerVersion) return false;
  if(vdpAccurate != vdp.accurate) return false;

  if(synchronize) power();
  serialize(s, synchronize);
  return true;
}

auto System::serialize(serializer& s, bool synchronize) -> void {
  scheduler.setSynchronize(synchronize);
  s(cpu);
  s(vdp);
  s(psg);
  s(cartridgeSlot);
  s(controllerPort);
  if(PCD::Present()) s(pcd);
}
