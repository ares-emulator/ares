struct FA2 : Interface, Thread {
  using Interface::Interface;

  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  PersistentMemory persistent;
  n3 bank;
  n9 busyTicks;
  n1 operationCompleted;
  n1 operationSucceeded;

  auto load() -> void override {
    Memory::Readable<n8> image;
    Interface::load(image, "program.rom");
    if(image.size() >= 29_KiB) {
      rom.allocate(28_KiB, 0x00);
      for(u32 offset : range(28_KiB)) rom.program(offset, image.read(1_KiB + offset));
    } else {
      rom.allocate(image.size(), 0x00);
      for(u32 offset : range(image.size())) rom.program(offset, image.read(offset));
    }
    persistent.load(pak, "save.flash", 256, 0x00);
  }

  auto save() -> void override {
    persistent.flush(pak);
  }

  auto unload() -> void override {
    persistent.flush(pak);
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    if(address == 0x1ff4 && rom.size() == 28_KiB) return operate();
    bankswitch(address);
    if(address <= 0x10ff) {
      ram.write(address & 0xff, data);
      return data;
    }
    if(address <= 0x11ff) return ram.read(address & 0xff);
    return rom.read(bank * 4_KiB + (address & 0x0fff));
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    if(address == 0x1ff4 && rom.size() == 28_KiB) {
      operate();
      return data;
    }
    bankswitch(address);
    if(address <= 0x10ff) ram.write(address & 0xff, data);
    return data;
  }

  auto main() -> void {
    if(busyTicks && !--busyTicks) {
      operationCompleted = 1;
      ram.write(255, operationSucceeded ? 0x00 : 0xff);
    }
    step(1);
    Thread::synchronize(cpu);
  }

  auto power(bool reset) -> void override {
    Thread::create(2'000, std::bind_front(&FA2::main, this));
    bank = 0;
    busyTicks = 0;
    operationCompleted = 0;
    operationSucceeded = 0;
    ram.allocate(256, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    Thread::serialize(s);
    s(bank);
    s(busyTicks);
    s(operationCompleted);
    s(operationSucceeded);
    s(ram);
  }

private:
  auto bankswitch(n16 address) -> void {
    if(address < 0x1ff5 || address > 0x1ffb) return;
    auto selected = address - 0x1ff5;
    if(selected < rom.size() / 4_KiB) bank = selected;
  }

  auto operate() -> n8 {
    auto value = rom.read(bank * 4_KiB + 0x0ff4);
    if(busyTicks) return value | 0x40;
    if(operationCompleted) {
      operationCompleted = 0;
      return value & ~0x40;
    }

    operationSucceeded = 1;
    auto command = ram.read(255);
    if(command == 0) {
      persistent.erase();
      busyTicks = 202;
    } else if(command == 1) {
      for(u32 offset : range(256)) ram.write(offset, persistent.read(offset));
      busyTicks = 1;
    } else if(command == 2) {
      persistent.replace({(const u8*)ram.data(), 256});
      busyTicks = 202;
    } else {
      operationSucceeded = 0;
      ram.write(255, 0xff);
      return value & ~0x40;
    }
    return value | 0x40;
  }
};
