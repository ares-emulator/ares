struct BUS : Interface {
  using Interface::Interface;

  enum class Revision : u32 { BUS0, BUS1, BUS2, BUS3 };

  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  Revision revision = Revision::BUS0;
  n3 bank;
  n8 mode;
  n2 fastJumpActive;
  n16 styOperand;
  n16 busOverdriveAddress;
  n16 jumpOperand;
  n32 musicCounter[3];
  n32 musicFrequency[3];
  n8 waveformShift[3];
  u16 datastreamBase = 0;
  u16 incrementBase = 0;
  u16 mapBase = 0;
  u16 waveformBase = 0;
  u32 audioPhase = 0;

  static constexpr u8 CommunicationStream = 0x10;
  static constexpr u8 JumpStream = 0x11;

  template<typename MemoryType>
  auto read32(u32 address, const MemoryType& memory) const -> u32 {
    if(address + 3 >= memory.size()) return 0;
    return memory.read(address + 0) << 0 | memory.read(address + 1) << 8
      | memory.read(address + 2) << 16 | memory.read(address + 3) << 24;
  }

  auto write32(u32 address, u32 data) -> void {
    if(address + 3 >= ram.size()) return;
    for(u32 byte : range(4)) ram.write(address + byte, data >> byte * 8);
  }

  auto scan(u32 value) const -> u32 {
    for(u32 offset = 0; offset + 3 < min<u32>(rom.size(), 3_KiB); offset += 4) {
      if(read32(offset, rom) == value) return offset;
    }
    return ~0u;
  }

  auto detectRevision() -> void {
    auto signature = scan(0x00535542);
    if(signature == 0x07f4) revision = Revision::BUS1;
    else if(signature == 0x0778) revision = Revision::BUS2;
    else if(signature == 0x0770) revision = Revision::BUS3;
    else revision = Revision::BUS0;

    if(revision == Revision::BUS0) {
      datastreamBase = 0x0ae0;
      incrementBase = 0x0b20;
      mapBase = 0x0b64;
      waveformBase = 0;
    } else {
      datastreamBase = revision == Revision::BUS3 ? 0x06d8 : 0x06e0;
      incrementBase = 0x0720;
      mapBase = 0x0760;
      waveformBase = 0x07f4;
    }
  }

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    detectRevision();
    ram.allocate(8_KiB, 0x00);
    harmony.load(cartridge.node);
  }

  auto unload() -> void override {
    harmony.unload();
  }

  auto armInvocation() const -> Harmony::Invocation override {
    if(revision == Revision::BUS0) return {.pc = 0x0c08, .lr = 0x0c00, .sp = 0x40001ffc};
    return {.pc = 0x0808, .lr = 0x0800, .sp = 0x40001ffc};
  }

  auto readARM(u32 mode, n32 address, n32& data) -> Harmony::Access override {
    auto romBase = mode & ARM7TDMI::Prefetch ? 0x50u : 0x0750u;
    return readARMMemory(rom, ram, mode, address, romBase, min<u32>(8_KiB, ram.size()), data);
  }

  auto writeARM(u32 mode, n32 address, n32 data) -> Harmony::Access override {
    u32 location = address;
    if(location >= 0x40000000 && location - 0x40000000 < min<u32>(8_KiB, ram.size())) {
      auto offset = location - 0x40000000;
      if(offset > 0x28 && offset < 0x06d8) return Harmony::Access::Fault;
    }
    return writeARMMemory(ram, mode, address, min<u32>(8_KiB, ram.size()), data);
  }

  auto trapARM(u32 address, n32& voice, n32 value) -> bool override {
    constexpr u32 base = 0x06da;
    if(address < base || address > base + 12 || (address - base) % 4) return false;
    if(voice >= 3) {
      if(address == base + 8) voice = 0;
      return true;
    }
    if(address == base + 0) musicFrequency[voice] = value;
    if(address == base + 4) musicCounter[voice] = 0;
    if(address == base + 8) voice = musicCounter[voice];
    if(address == base + 12) waveformShift[voice] = min<u32>(value, 31);
    return true;
  }

  auto stepARM(u32 clocks) -> void override {
    stepAudio(clocks);
  }

  auto programBase() const -> u32 {
    return revision == Revision::BUS0 ? 3_KiB : 4_KiB;
  }

  auto displayBase() const -> u32 {
    return revision == Revision::BUS0 ? 3_KiB : 2_KiB;
  }

  auto program(u32 address) const -> n8 {
    auto offset = programBase() + u32{bank} * 4_KiB + (address & 0x0fff);
    if(offset < rom.size()) return rom.read(offset);
    return 0;
  }

  auto displayRead(u32 address) const -> u8 {
    if(displayBase() + address < ram.size()) return ram.read(displayBase() + address);
    return 0;
  }

  auto displayWrite(u32 address, u8 data) -> void {
    if(displayBase() + address < ram.size()) ram.write(displayBase() + address, data);
  }

  auto streamPointer(u8 index) const -> u32 {
    return read32(datastreamBase + index * 4, ram);
  }

  auto setStreamPointer(u8 index, u32 value) -> void {
    write32(datastreamBase + index * 4, value);
  }

  auto streamIncrement(u8 index) const -> u32 {
    return read32(incrementBase + index * 4, ram);
  }

  auto setStreamIncrement(u8 index, u32 value) -> void {
    write32(incrementBase + index * 4, value);
  }

  auto addressMap(u8 index) const -> u32 {
    return read32(mapBase + index * 4, ram);
  }

  auto setAddressMap(u8 index, u32 value) -> void {
    write32(mapBase + index * 4, value);
  }

  auto nextStreamByte(u8 index) -> u8 {
    auto pointer = streamPointer(index);
    auto result = displayRead(pointer >> 20);
    pointer += streamIncrement(index) << 12;
    setStreamPointer(index, pointer);
    return result;
  }

  auto stepAudio(u32 clocks) -> void {
    audioPhase += clocks;
    auto ticks = audioPhase / 3500;
    audioPhase %= 3500;
    for(u32 voice : range(3)) musicCounter[voice] += u32{musicFrequency[voice]} * ticks;
  }

  auto waveform(u32 voice) const -> u32 {
    auto address = read32(waveformBase + voice * 4, ram);
    if(address < 0x40000800) return 0;
    address -= 0x40000800;
    return address < 4_KiB ? address : 0;
  }

  auto amplitude() -> u8 {
    if((mode & 0xf0) == 0) {
      auto address = read32(waveformBase, ram) + (u32{musicCounter[0]} >> 21);
      u8 sample = 0;
      if(address < rom.size()) sample = rom.read(address);
      else if(address >= 0x40000000 && address - 0x40000000 < ram.size()) {
        sample = ram.read(address - 0x40000000);
      }
      if(!(u32{musicCounter[0]} & 1 << 20)) sample >>= 4;
      return sample & 15;
    }

    u32 result = 0;
    for(u32 voice : range(3)) {
      result += displayRead(waveform(voice) + (u32{musicCounter[voice]} >> waveformShift[voice]));
    }
    return result;
  }

  auto switchBank(u32 address) -> void {
    if(revision == Revision::BUS0) {
      if(address >= 0x0ff6 && address <= 0x0ffb) bank = address - 0x0ff6;
      return;
    }
    if(address >= 0x0ff5 && address <= 0x0ffb) bank = address - 0x0ff5;
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    u32 offset = address & 0x0fff;
    u8 value = program(offset);

    if(revision == Revision::BUS3) {
      if(fastJumpActive && jumpOperand == offset) {
        fastJumpActive--;
        jumpOperand++;
        auto pointer = streamPointer(JumpStream);
        auto result = displayRead(pointer >> 20);
        setStreamPointer(JumpStream, pointer + 0x00100000);
        return result;
      }
      if((mode & 15) == 0 && value == 0x4c && offset <= 0x0ffd
        && program(offset + 1) == 0 && program(offset + 2) == 0) {
        fastJumpActive = 2;
        jumpOperand = offset + 1;
        return value;
      }
      jumpOperand = 0;
    }

    if((mode & 15) == 0 && styOperand == offset) busOverdriveAddress = value;
    styOperand = 0;

    if(revision == Revision::BUS0 && offset < 0x10) return nextStreamByte(offset);
    if((revision == Revision::BUS1 || revision == Revision::BUS2) && offset < 0x20) {
      if(offset < 0x10) return nextStreamByte(offset);
      if(offset == 0x18) return amplitude();
      return 0;
    }
    if(revision == Revision::BUS3 && offset >= 0x0fee && offset <= 0x0ff3) {
      if(offset == 0x0fee) return amplitude();
      if(offset == 0x0fef) return nextStreamByte(CommunicationStream);
    }

    switchBank(offset);
    if((mode & 15) == 0 && value == 0x84) styOperand = offset + 1;
    return value;
  }

  auto writeBelowCartridge(n16 address, n8 data) -> n8 {
    bool matches = address == busOverdriveAddress;
    busOverdriveAddress = 0xff;
    if(!matches) return data;
    auto target = u8(address & 0x7f);
    if(target > 0x24) return data;
    auto mappings = addressMap(target);
    auto stream = u8(mappings & 15);
    auto stuffed = u8(data) & nextStreamByte(stream);
    setAddressMap(target, mappings >> 4 | u32{stream} << 28);
    return stuffed;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return writeBelowCartridge(address, data);
    u32 offset = address & 0x0fff;

    if(revision == Revision::BUS0) {
      auto index = u8(offset & 15);
      if(offset >= 0x10 && offset <= 0x13) {
        auto pointer = streamPointer(index);
        displayWrite(pointer >> 20, data);
        setStreamPointer(index, pointer + 0x00100000);
      }
      if(offset == 0x1b && (data == 254 || data == 255)) harmony.call(data == 254);
      if(offset == 0x1c) mode = data == 0 ? 0 : 0x0f;
      if(offset >= 0x20 && offset <= 0x2f) {
        auto pointer = streamPointer(index) << 8;
        setStreamPointer(index, pointer & 0xf0000000 | u32{data} << 20);
      }
      if(offset >= 0x30 && offset <= 0x3f) setStreamIncrement(index, data);
      switchBank(offset);
      return data;
    }

    switchBank(offset);
    if(revision == Revision::BUS3) {
      if(offset == 0x0ff0) {
        auto pointer = streamPointer(CommunicationStream);
        displayWrite(pointer >> 20, data);
        setStreamPointer(CommunicationStream, pointer + 0x00100000);
      }
      if(offset == 0x0ff1) {
        auto pointer = streamPointer(CommunicationStream) << 8;
        setStreamPointer(CommunicationStream, pointer & 0xf0000000 | u32{data} << 20);
      }
      if(offset == 0x0ff2) mode = data;
      if(offset == 0x0ff3 && (data == 254 || data == 255)) harmony.call(data == 254);
      return data;
    }

    if(offset >= 0x10 && offset <= 0x1f) {
      auto index = u8(offset & 15);
      if(index <= 3) {
        auto pointer = streamPointer(index);
        displayWrite(pointer >> 20, data);
        setStreamPointer(index, pointer + 0x00100000);
      }
      if(index >= 4 && index <= 7) {
        index &= 3;
        auto pointer = streamPointer(index) << 8;
        setStreamPointer(index, pointer & 0xf0000000 | u32{data} << 20);
      }
      if(index == 9) mode = data == 0 ? 0 : 0x0f;
      if(index == 10 && (data == 254 || data == 255)) harmony.call(data == 254);
    }
    return data;
  }

  auto power(bool reset) -> void override {
    ram.fill(0x00);
    auto driverSize = revision == Revision::BUS0 ? 3_KiB : 2_KiB;
    for(u32 offset : range(min<u32>(driverSize, rom.size()))) ram.write(offset, rom.read(offset));
    bank = revision == Revision::BUS0 ? 5 : 6;
    mode = 0xff;
    fastJumpActive = 0;
    styOperand = 0;
    busOverdriveAddress = 0;
    jumpOperand = 0;
    for(auto& item : musicCounter) item = 0;
    for(auto& item : musicFrequency) item = 0;
    for(auto& item : waveformShift) item = 27;
    audioPhase = 0;
    harmony.power();
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(bank);
    s(mode);
    s(fastJumpActive);
    s(styOperand);
    s(busOverdriveAddress);
    s(jumpOperand);
    for(auto& item : musicCounter) s(item);
    for(auto& item : musicFrequency) s(item);
    for(auto& item : waveformShift) s(item);
    s(audioPhase);
    harmony.serialize(s);
  }
};
