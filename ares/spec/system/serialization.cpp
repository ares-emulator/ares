auto System::serialize(bool synchronize) -> serializer {
  if(synchronize) scheduler.enter(Scheduler::Mode::Synchronize);
  serializer s;

  uint signature = 0x31545342;
  uint size = s.capacity();
  char version[16] = {};
  char description[512] = {};
  memory::copy(&version, (const char*)SerializerVersion, SerializerVersion.size());

  s(signature);
  s(size);
  s(synchronize);
  s(version);
  s(description);
  serializeAll(s, synchronize);
  return s;
}

auto System::unserialize(serializer& s) -> bool {
  uint signature = 0;
  uint size = 0;
  bool synchronize = true;
  char version[16] = {};
  char description[512] = {};

  s(signature);
  s(size);
  s(synchronize);
  s(version);
  s(description);

  if(signature != 0x31545342) return false;
  if(size != information.serializeSize[synchronize]) return false;
  if(string{version} != SerializerVersion) return false;

  if(synchronize) power(/* reset =*/ false);
  serializeAll(s, synchronize);
  return true;
}

//internal

auto System::serialize(serializer& s) -> void {
}

auto System::serializeAll(serializer& s, bool synchronize) -> void {
  scheduler.setSynchronize(synchronize);
  system.serialize(s);
  //keyboard.serialize(s);
  cpu.serialize(s);
  //vdp.serialize(s);
  //psg.serialize(s);
}

auto System::serializeInit(bool synchronize) -> uint {
  serializer s;

  uint signature = 0;
  uint size = 0;
  char version[16] = {};
  char description[512] = {};

  s(signature);
  s(size);
  s(synchronize);
  s(version);
  s(description);
  serializeAll(s, synchronize);
  return s.size();
}
