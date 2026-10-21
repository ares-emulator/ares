Datel::Datel(Node::Port parent, ExpansionPort::Device subtype) : subtype(subtype), flash(subtype == ExpansionPort::Datel128ID ? ParallelFlash::Model::SST29EE010 : ParallelFlash::Model::SST29EE020) {
  u32 size = flash.capacity;
  node = parent->append<Node::Peripheral>(size == 128_KiB ? "Datel (128KB)" : "Datel (256KB)");
  auto fp = system.pak->read("expansion.eeprom");
  if(!fp || fp->size() != size) { parent->remove(node); node.reset(); return; }
  flash.memory.resize(size, 0xff);
  fp->seek(0);
  fp->read({flash.memory.data(), flash.memory.size()});
  flash.protection = true;
  switchEnabled = node->append<Node::Setting::Boolean>("Cartridge Switch", true);
  switchEnabled->setDynamic(true);
}

auto Datel::save() -> void {
  if(auto fp = system.pak->write("expansion.eeprom")) {
    fp->seek(0);
    fp->write({flash.memory.data(), flash.memory.size()});
  }
}

auto Datel::power(bool) -> void {
  microsecondClock = 0;
  flash.power();
}

auto Datel::step(u32 clocks) -> void {
  microsecondClock += (u64)clocks * 1'000'000;
  u32 microseconds = microsecondClock / system.frequency();
  microsecondClock %= system.frequency();
  if(flash.remaining) flash.step(microseconds);
}

auto Datel::serialize(serializer& s) -> void {
  u8 serializedBank = 0;
  s(serializedBank);
  s(microsecondClock);
  bool enabled = switchEnabled && switchEnabled->value();
  s(enabled);
  if(s.reading() && switchEnabled) switchEnabled->setValue(enabled);
  s(flash);
}

auto Datel::readByte(u32 address) -> u32 {
  address &= 0x7ffff;
  if(address < 0x20000) return flash.read(address);
  if(address >= 0x40000 && address < 0x60000 && subtype != ExpansionPort::Datel128ID) return flash.read(address - 0x20000);
  if(address == 0x20010) return 0;
  if(address == 0x20018) return switchEnabled && switchEnabled->value();
  return 0xff;
}

auto Datel::writeByte(u32 address, u32 data) -> void {
  address &= 0x7ffff;
  if(address < 0x20000) flash.write(address, data);
  if(address >= 0x40000 && address < 0x60000 && subtype != ExpansionPort::Datel128ID) flash.write(address - 0x20000, data);
}
