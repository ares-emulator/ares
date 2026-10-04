struct Chetiry : Interface, Thread {
  using Interface::Interface;

  Memory::Readable<n8> rom;
  Memory::Readable<n8> tunes;
  Memory::Writable<n8> ram;
  PersistentMemory persistent;
  n3 bank;
  n8 operation;
  n16 tunePosition;
  n1 ldaImmediate;
  n32 random = 0x2b435044;
  n32 musicCounter[3];
  n32 musicFrequency[3];
  n15 busyTicks;
  n1 operationCompleted;
  n1 operationSucceeded;
  n3 tune;

  auto load() -> void override {
    Memory::Readable<n8> image;
    Interface::load(image, "program.rom");
    rom.allocate(32_KiB, 0x00);
    for(u32 offset : range(min<u32>(image.size(), 32_KiB))) rom.program(offset, image.read(offset));
    tunes.allocate(28_KiB, 0x00);
    if(image.size() > 32_KiB) {
      for(u32 offset : range(min<u32>(image.size() - 32_KiB, 28_KiB))) {
        tunes.program(offset, image.read(32_KiB + offset));
      }
    }
    persistent.load(pak, "save.eeprom", 256, 0x00);
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
    auto value = rom.read(bank * 4_KiB + (address & 0x0fff));
    if(ldaImmediate && value == 0xf2) {
      ldaImmediate = 0;
      return (musicCounter[0] >> 31) + (musicCounter[1] >> 31) + (musicCounter[2] >> 31) << 2;
    }
    ldaImmediate = 0;

    if(address <= 0x103f) {
      ram.write(address & 0x3f, data);
      return data;
    }
    if(address <= 0x107f) return readRAM(address & 0x3f);
    if(address == 0x1ff4) return operate(value);
    bankswitch(address);
    ldaImmediate = value == 0xa9;
    return value;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    if(address <= 0x103f) {
      writeRAM(address & 0x3f, data);
      return data;
    }
    if(address == 0x1ff4) {
      operate(rom.read(bank * 4_KiB + 0x0ff4));
      return data;
    }
    bankswitch(address);
    return data;
  }

  auto main() -> void {
    for(u32 channel : range(3)) musicCounter[channel] += musicFrequency[channel];
    if(busyTicks && !--busyTicks) {
      operationCompleted = 1;
      ram.write(0, operationSucceeded ? 0x00 : 0xff);
    }
    step(1);
    Thread::synchronize(cpu);
  }

  auto power(bool reset) -> void override {
    Thread::create(20'000, std::bind_front(&Chetiry::main, this));
    bank = 1;
    operation = 0;
    tunePosition = 0;
    ldaImmediate = 0;
    random = 0x2b435044;
    busyTicks = 0;
    operationCompleted = 0;
    operationSucceeded = 0;
    tune = 0;
    for(auto& counter : musicCounter) counter = 0;
    for(auto& frequency : musicFrequency) frequency = 0;
    ram.allocate(64, 0x00);
    for(u32 offset : range(4)) ram.write(offset, 0xff);
  }

  auto serialize(serializer& s) -> void override {
    Thread::serialize(s);
    s(bank);
    s(operation);
    s(tunePosition);
    s(ldaImmediate);
    s(random);
    s(musicCounter);
    s(musicFrequency);
    s(busyTicks);
    s(operationCompleted);
    s(operationSucceeded);
    s(tune);
    s(ram);
  }

private:
  static auto noteFrequency(u32 note) -> u32 {
    static constexpr u32 table[63] = {
      0, 0, 0, 11811160, 12513387, 13257490, 14045832, 14881203, 15765966,
      16703557, 17696768, 18749035, 19864009, 21045125, 22296464, 23622320,
      25026989, 26515195, 28091878, 29762191, 31531932, 33406900, 35393537,
      37498071, 39727803, 42090250, 44592927, 47244640, 50053978, 53030391,
      56183756, 59524596, 63064079, 66814014, 70787074, 74996142, 79455606,
      84180285, 89186069, 94489281, 100107957, 106060567, 112367297,
      119048977, 126128157, 133628029, 141573933, 149992288, 158911428,
      168360785, 178371925, 188978561, 200215913, 212121348, 224734593,
      238098169, 252256099, 267256058, 283147866, 299984783, 317822855,
      336721571, 356744064,
    };
    return note < 63 ? table[note] : 0;
  }

  auto bankswitch(n16 address) -> void {
    if(address >= 0x1ff5 && address <= 0x1ffb) bank = address - 0x1ff4;
  }

  auto readRAM(u32 address) -> n8 {
    if(address == 0) return ram.read(0);
    if(address == 1) {
      random = (random.bit(10) ? 0x10adab1e : 0) ^ (u32{random} >> 11 | u32{random} << 21);
      return random.byte(0);
    }
    if(address == 2) return tunePosition.byte(0);
    if(address == 3) return tunePosition.byte(1);
    return ram.read(address);
  }

  auto writeRAM(u32 address, n8 data) -> void {
    if(address == 0) operation = data;
    else if(address == 1) random = 0x2b435044;
    else if(address == 2) {
      tunePosition = 0;
      for(auto& counter : musicCounter) counter = 0;
      for(auto& frequency : musicFrequency) frequency = 0;
    } else if(address == 3) {
      updateTune();
    } else {
      ram.write(address, data);
    }
  }

  auto updateTune() -> void {
    auto position = u32{tune} * 4_KiB + u32{tunePosition} * 3;
    if(position + 2 >= tunes.size() || position + 2 >= (u32{tune} + 1) * 4_KiB) {
      tunePosition = 0;
      return;
    }
    tunePosition++;
    auto note0 = tunes.read(position + 0);
    auto note1 = tunes.read(position + 1);
    auto note2 = tunes.read(position + 2);
    if(note0) musicFrequency[0] = noteFrequency(note0);
    if(note1) musicFrequency[1] = noteFrequency(note1);
    if(note2 == 1) tunePosition = 0;
    else musicFrequency[2] = noteFrequency(note2);
  }

  auto operate(n8 value) -> n8 {
    if(busyTicks) return value | 0x40;
    if(operationCompleted) {
      operationCompleted = 0;
      return value & ~0x40;
    }
    auto index = u32{operation} >> 4;
    auto command = u32{operation} & 15;
    operationSucceeded = 1;
    if(command == 1 && index < 7) {
      tune = index;
      tunePosition = 0;
      busyTicks = 10'000;
    } else if(command == 2 && index < 4) {
      for(u32 offset : range(4, 64)) ram.write(offset, persistent.read(index * 64 + offset));
      busyTicks = 10'000;
    } else if(command == 3 && index < 4) {
      for(u32 offset : range(4, 64)) persistent.write(index * 64 + offset, ram.read(offset));
      busyTicks = 20'000;
    } else if(command == 4) {
      persistent.erase();
      busyTicks = 20'000;
    } else {
      operationSucceeded = 0;
      ram.write(0, 0xff);
      return value & ~0x40;
    }
    return value | 0x40;
  }
};
