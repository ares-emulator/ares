static const string SerializerVersion = "v134";

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
  s(cartridge);
  s(controllerPort1);
  s(controllerPort2);
  s(cardSlot);
  s(cpu);
  s(apu);
  s(lspc);
  s(opnb);
  s(wram);
  s(sram);
  s(spriteRam);
  s(pcmRam);
  s(fixRam);
  s(io.sramLock);
  s(io.slotSelect);
  s(io.ledMarquee);
  s(io.ledLatch1);
  s(io.ledLatch2);
  s(io.ledData);
  s(io.uploadZone);
  s(io.spriteUploadBank);
  s(io.pcmUploadBank);
}
