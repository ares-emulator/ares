Xplorer::Xplorer(Node::Port parent, ExpansionPort::Device subtype) : subtype(subtype), flash(subtype == ExpansionPort::XplorerID ? ParallelFlash::Model::SST29EE020 : subtype == ExpansionPort::XplorerFXAtmelID ? ParallelFlash::Model::AT29C040A : ParallelFlash::Model::W29C040) {
  bool fx = subtype != ExpansionPort::XplorerID;
  bool atmel = subtype == ExpansionPort::XplorerFXAtmelID;
  node = parent->append<Node::Peripheral>(atmel ? "Xplorer FX (Atmel)" : fx ? "Xplorer FX (Winbond)" : "Xplorer");
  auto fp = system.pak->read("expansion.eeprom");
  if(!fp || fp->size() != flash.capacity) { parent->remove(node); node.reset(); return; }
  flash.memory.resize(flash.capacity, 0xff);
  fp->seek(0);
  fp->read({flash.memory.data(), flash.memory.size()});
  flash.protection = true;
  switchEnabled = node->append<Node::Setting::Boolean>("Cartridge Switch", true);
  switchEnabled->setDynamic(true);
  if(fx) ram.resize(128_KiB);
}

auto Xplorer::save() -> void {
  if(auto fp = system.pak->write("expansion.eeprom")) {
    fp->seek(0);
    fp->write({flash.memory.data(), flash.memory.size()});
  }
}

auto Xplorer::power(bool reset) -> void {
  bank = 0;
  microsecondClock = 0;
  flash.power();
  if(!reset && !ram.empty()) memory::fill<u8>(ram.data(), ram.size(), 0);
}

auto Xplorer::step(u32 clocks) -> void {
  microsecondClock += (u64)clocks * 1'000'000;
  u32 microseconds = microsecondClock / system.frequency();
  microsecondClock %= system.frequency();
  if(flash.remaining) flash.step(microseconds);
}

auto Xplorer::serialize(serializer& s) -> void {
  s(bank);
  s(microsecondClock);
  bool enabled = switchEnabled && switchEnabled->value();
  s(enabled);
  if(s.reading() && switchEnabled) switchEnabled->setValue(enabled);
  s(flash);
  if(!ram.empty()) s(std::span<u8>{ram.data(), ram.size()});
}

auto Xplorer::readByte(u32 address) -> u32 {
  address &= 0x7ffff;
  if(address < 0x40000) return flash.read(address);
  if(address < 0x60000) {
    if(bank & 0x10) {
      if(ram.empty() || !(bank & 0x40)) return 0xff;
      return ram[address & 0x1ffff];
    }
    u32 offset = (address & 0x1ffff) | (bank & 0x20) << 12;
    return flash.read(offset + (subtype == ExpansionPort::XplorerID ? 0 : 0x40000));
  }
  if(address < 0x70000) {
    switch(address & 7) {
    case 0: return 0xfe | u32(switchEnabled && switchEnabled->value());
    case 1: return 0xff;
    case 2: return 1;
    case 6: case 7: return 0xff;
    }
  }
  return 0xff;
}

auto Xplorer::writeByte(u32 address, u32 data) -> void {
  address &= 0x7ffff;
  if(address < 0x40000) {
    flash.write(address, data);
    return;
  }
  if(address < 0x60000) {
    if(bank & 0x10) {
      if(!ram.empty() && (bank & 0x40)) ram[address & 0x1ffff] = data;
    } else {
      u32 offset = (address & 0x1ffff) | (bank & 0x20) << 12;
      flash.write(offset + (subtype == ExpansionPort::XplorerID ? 0 : 0x40000), data);
    }
    return;
  }
  if(address < 0x70000) {
    if((address & 7) == 1) bank = data;
  }
}
