static const string SerializerVersion = "v153";

auto System::stateVersion() const -> string { return SerializerVersion; }

auto System::serializeHeader(serializer& s, bool synchronize) -> void {

  u32  signature = SerializerSignature;
  char version[16] = {};
  char description[512] = {};
  memory::copy(&version, (const char*)SerializerVersion, SerializerVersion.size());

  s(signature);
  s(synchronize);
  s(version);
  s(description);

}

auto System::serialize(bool synchronize) -> serializer {
  serializer s;
  serializeHeader(s, synchronize);
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

auto System::serializePrefix(serializer& s) -> void {
  s(random);
  s(queue);
  s(cartridge);
  s(controllerPort1);
  s(controllerPort2);
  s(controllerPort3);
  s(controllerPort4);
}

auto System::inspectStateRdramLayout() -> std::pair<u64, u64> {
  serializer s;
  serializeHeader(s, true);
  serializePrefix(s);
  return {s.size(), rdram.ram.size};
}

auto System::serialize(serializer& s, bool synchronize) -> void {
  serializePrefix(s);
  s(rdram);
  s(mi);
  s(vi);
  s(ai);
  s(pi);
  s(pif);
  s(cic);
  s(ri);
  s(si);
  s(cpu);
  s(rdp);
  s(rsp);
  s(dd);
  s(aleck64);
}
