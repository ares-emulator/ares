auto System::serialize(bool synchronize) -> serializer {
  if(synchronize) scheduler.enter(Scheduler::Mode::Synchronize);
  serializer s(information.serializeSize[synchronize]);

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

  if(synchronize) power();
  serializeAll(s, synchronize);
  return true;
}

//internal

auto System::serialize(serializer& s) -> void {
  s.integer(bios.size);
  s.integer(bios.mdr);
}

auto System::serializeAll(serializer& s, bool synchronize) -> void {
  scheduler.setSynchronize(synchronize);
  cartridge.serialize(s);
  system.serialize(s);
  cpu.serialize(s);
  ppu.serialize(s);
  apu.serialize(s);
  player.serialize(s);
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
