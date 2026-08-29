SaveKey::SaveKey(Node::Port parent, string name, string filename) {
  node = parent->append<Node::Peripheral>(name);
  node->setPak(pak = platform->pak(node));

  persistent.load(pak, filename, 32_KiB, 0xff);
  eeprom.load(M24C::Type::M24C256);
  synchronizeEeprom();
  eeprom.power();
}

auto SaveKey::save() -> void {
  synchronizePersistent();
  persistent.flush(pak);
}

auto SaveKey::power(bool reset) -> void {
  eeprom.power();
}

auto SaveKey::read() -> n8 {
  n8 data = 0xff;
  data.bit(2) = eeprom.read();
  return data;
}

auto SaveKey::write(n8 data) -> void {
  auto transactionBoundary = eeprom.clock() && data.bit(3) && eeprom.data() != data.bit(2);
  eeprom.clock = eeprom.clock();
  eeprom.data = eeprom.data();
  eeprom.clock = data.bit(3);
  eeprom.data = data.bit(2);
  eeprom.write();
  if(transactionBoundary) synchronizePersistent();
}

auto SaveKey::serialize(serializer& s) -> void {
  if(s.writing()) synchronizePersistent();
  eeprom.serialize(s, false);
  if(s.reading()) synchronizeEeprom();
}

auto SaveKey::synchronizeEeprom() -> void {
  std::copy(persistent.memory.begin(), persistent.memory.end(), std::begin(eeprom.memory));
}

auto SaveKey::synchronizePersistent() -> void {
  persistent.replace({(const u8*)eeprom.memory, eeprom.size()});
}
