struct CDF : Interface {
  using Interface::Interface;

  enum class Revision : u32 { CDF0, CDF1, CDFJ, CDFJPlus };

  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  Revision revision = Revision::CDF0;
  n3 bank;
  n8 mode;
  n2 fastJumpActive;
  n6 fastJumpStream;
  n16 immediateOperand;
  n16 jumpOperand;
  n32 musicCounter[3];
  n32 musicFrequency[3];
  n8 waveformShift[3];
  u16 datastreamBase = 0;
  u16 incrementBase = 0;
  u16 waveformBase = 0;
  u16 fastFetcherOffset = 0;
  u8 amplitudeStream = 0;
  u8 fastJumpMask = 0;
  bool ldxEnabled = false;
  bool ldyEnabled = false;
  u32 audioPhase = 0;

  static constexpr u8 CommunicationStream = 0x20;
  static constexpr u8 JumpStreamBase = 0x21;
  static constexpr u16 NoImmediate = 0xffff;

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
    if(rom.size() < 2_KiB) return ~0u;
    for(u32 offset = 0; offset < 2_KiB; offset += 4) {
      if(read32(offset, rom) == value) return offset;
    }
    return ~0u;
  }

  auto detectRevision() -> void {
    auto plusSignature = scan(0x53554c50);
    if(plusSignature != ~0u && read32(plusSignature + 4, rom) == 0x4a464443
      && read32(plusSignature + 8, rom) == 1) {
      revision = Revision::CDFJPlus;
      amplitudeStream = 0x23;
      fastJumpMask = 0xfe;
      datastreamBase = 0x0098;
      incrementBase = 0x0124;
      waveformBase = 0x01b0;
      for(u32 offset = 0; offset < 2_KiB; offset += 4) {
        auto word = read32(offset, rom);
        if(word == 0x135200a2) ldxEnabled = true;
        if(word == 0x135200a0) ldyEnabled = true;
        if((word & 0xffffff00) == 0xe2422000) fastFetcherOffset = offset;
      }
      return;
    }

    u8 subversion = 0;
    for(u32 offset = 0; offset + 11 < min<u32>(rom.size(), 2_KiB); offset += 4) {
      if(rom.read(offset + 0) == 'C' && rom.read(offset + 1) == 'D' && rom.read(offset + 2) == 'F'
        && rom.read(offset + 4) == 'C' && rom.read(offset + 5) == 'D' && rom.read(offset + 6) == 'F'
        && rom.read(offset + 8) == 'C' && rom.read(offset + 9) == 'D' && rom.read(offset + 10) == 'F') {
        subversion = rom.read(offset + 3);
        break;
      }
    }

    if(subversion == 0x4a) {
      revision = Revision::CDFJ;
      amplitudeStream = 0x23;
      fastJumpMask = 0xfe;
      datastreamBase = 0x0098;
      incrementBase = 0x0124;
      waveformBase = 0x01b0;
    } else if(subversion == 0) {
      revision = Revision::CDF0;
      amplitudeStream = 0x22;
      fastJumpMask = 0xff;
      datastreamBase = 0x06e0;
      incrementBase = 0x0768;
      waveformBase = 0x07f0;
    } else {
      revision = Revision::CDF1;
      amplitudeStream = 0x22;
      fastJumpMask = 0xff;
      datastreamBase = 0x00a0;
      incrementBase = 0x0128;
      waveformBase = 0x01b0;
    }
  }

  auto isPlus() const -> bool {
    return revision == Revision::CDFJPlus;
  }

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    detectRevision();
    ram.allocate(isPlus() ? 32_KiB : 8_KiB, 0x00);
    harmony.load(cartridge.node);
  }

  auto unload() -> void override {
    harmony.unload();
  }

  auto armInvocation() const -> Harmony::Invocation override {
    if(isPlus()) {
      auto address = read32(0x17f8, rom) & ~1u;
      return {.pc = address, .lr = address, .sp = read32(0x17f4, rom)};
    }
    return {.pc = 0x0808, .lr = 0x0800, .sp = 0x40001ffc};
  }

  auto armRamLimit() const -> u32 {
    if(!isPlus()) return min<u32>(8_KiB, ram.size());
    if(rom.size() == 64_KiB || rom.size() == 128_KiB) return min<u32>(16_KiB, ram.size());
    if(rom.size() == 256_KiB || rom.size() == 512_KiB) return min<u32>(32_KiB, ram.size());
    return min<u32>(8_KiB, ram.size());
  }

  auto readARM(u32 mode, n32 address, n32& data) -> Harmony::Access override {
    auto romBase = mode & ARM7TDMI::Prefetch ? 0x50u : 0x0750u;
    return readARMMemory(rom, ram, mode, address, romBase, armRamLimit(), data);
  }

  auto writeProtectedARM(u32 offset) const -> bool {
    if(revision == Revision::CDF0) {
      return offset > 0x28 && offset < 0x0800 && !(offset >= 0x06e0 && offset < 0x07fc);
    }
    if(revision == Revision::CDF1) {
      return offset > 0x28 && offset < 0x0800 && !(offset >= 0x00a0 && offset < 0x01bc);
    }
    return offset > 0x28 && offset < 0x0800 && !(offset >= 0x0098 && offset < 0x01bc)
      && !(isPlus() && offset == 0x03e0);
  }

  auto writeARM(u32 mode, n32 address, n32 data) -> Harmony::Access override {
    u32 location = address;
    if(location >= 0x40000000 && location - 0x40000000 < armRamLimit()) {
      if(writeProtectedARM(location - 0x40000000)) return Harmony::Access::Fault;
    }
    return writeARMMemory(ram, mode, address, armRamLimit(), data);
  }

  auto trapARM(u32 address, n32& voice, n32 value) -> bool override {
    u32 base = revision == Revision::CDF0 ? 0x06e2 : 0x0752;
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
    return isPlus() ? 2_KiB : 4_KiB;
  }

  auto program(u32 address) const -> n8 {
    u32 offset = programBase() + u32{bank} * 4_KiB + (address & 0x0fff);
    if(offset < rom.size()) return rom.read(offset);
    return 0;
  }

  auto displaySize() const -> u32 {
    return ram.size() > 2_KiB ? ram.size() - 2_KiB : 0;
  }

  auto displayRead(u32 address) const -> u8 {
    if(address < displaySize()) return ram.read(2_KiB + address);
    return 0;
  }

  auto displayWrite(u32 address, u8 data) -> void {
    if(address < displaySize()) ram.write(2_KiB + address, data);
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

  auto nextStreamByte(u8 index) -> u8 {
    if(index > amplitudeStream) return 0;
    auto pointer = streamPointer(index);
    u32 location = isPlus() ? pointer >> 16 : pointer >> 20;
    auto data = displayRead(location);
    pointer += streamIncrement(index) << (isPlus() ? 8 : 12);
    setStreamPointer(index, pointer);
    return data;
  }

  auto waveform(u32 voice) const -> u32 {
    u32 address = read32(waveformBase + voice * 4, ram);
    address -= 0x40000800;
    if(!isPlus() && address >= 4_KiB) address &= 0x0fff;
    if(isPlus() && address >= displaySize()) return 0;
    return address;
  }

  auto stepAudio(u32 clocks) -> void {
    audioPhase += clocks;
    auto ticks = audioPhase / 3500;
    audioPhase %= 3500;
    for(u32 voice : range(3)) musicCounter[voice] += u32{musicFrequency[voice]} * ticks;
  }

  auto audioValue() -> u8 {
    if((mode & 0xf0) == 0) {
      u32 address = read32(waveformBase, ram) + (u32{musicCounter[0]} >> (isPlus() ? 13 : 21));
      u8 sample = 0;
      if(address < rom.size()) sample = rom.read(address);
      else if(address >= 0x40000000 && address - 0x40000000 < ram.size()) sample = ram.read(address - 0x40000000);
      if(!(u32{musicCounter[0]} & 1 << (isPlus() ? 12 : 20))) sample >>= 4;
      return sample & 15;
    }

    u32 result = 0;
    for(u32 voice : range(3)) {
      auto location = waveform(voice) + (u32{musicCounter[voice]} >> waveformShift[voice]);
      result += displayRead(location);
    }
    return result;
  }

  auto switchBank(u32 address) -> void {
    if(address < 0x0ff4 || address > 0x0ffb) return;
    if(isPlus()) bank = address == 0x0ffb ? 0 : address - 0x0ff4;
    else bank = address == 0x0ff4 || address == 0x0ffb ? 6 : address - 0x0ff5;
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    u32 offset = address & 0x0fff;
    u8 value = program(offset);

    if(fastJumpActive && jumpOperand == offset) {
      fastJumpActive--;
      jumpOperand++;
      auto pointer = streamPointer(fastJumpStream);
      auto result = displayRead(isPlus() ? pointer >> 16 : pointer >> 20);
      pointer += isPlus() ? 0x00010000 : 0x00100000;
      setStreamPointer(fastJumpStream, pointer);
      return result;
    }

    if((mode & 15) == 0 && value == 0x4c && offset <= 0x0ffd
      && !(program(offset + 1) & fastJumpMask) && program(offset + 2) == 0) {
      fastJumpActive = 2;
      jumpOperand = offset + 1;
      fastJumpStream = program(offset + 1) + JumpStreamBase;
      return value;
    }
    jumpOperand = 0;

    bool fastFetch = false;
    if((mode & 15) == 0 && immediateOperand == offset) {
      u8 base = 0;
      if(fastFetcherOffset && fastFetcherOffset < ram.size()) base = ram.read(fastFetcherOffset);
      fastFetch = value >= base && u32{value} <= u32{base} + amplitudeStream;
      if(fastFetch && fastFetcherOffset) value -= base;
    }
    immediateOperand = NoImmediate;
    if(fastFetch) return value == amplitudeStream ? audioValue() : nextStreamByte(value);

    switchBank(offset);
    if((mode & 15) == 0 && (value == 0xa9 || (ldxEnabled && value == 0xa2) || (ldyEnabled && value == 0xa0))) {
      immediateOperand = offset + 1;
    }
    return value;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    u32 offset = address & 0x0fff;
    if(offset == 0x0ff0) {
      auto pointer = streamPointer(CommunicationStream);
      displayWrite(isPlus() ? pointer >> 16 : pointer >> 20, data);
      setStreamPointer(CommunicationStream, pointer + (isPlus() ? 0x00010000 : 0x00100000));
    }
    if(offset == 0x0ff1) {
      auto pointer = streamPointer(CommunicationStream) << 8;
      pointer = isPlus() ? (pointer & 0xff000000) | u32{data} << 16
        : (pointer & 0xf0000000) | u32{data} << 20;
      setStreamPointer(CommunicationStream, pointer);
    }
    if(offset == 0x0ff2) mode = data;
    if(offset == 0x0ff3 && (data == 254 || data == 255)) harmony.call(data == 254);
    switchBank(offset);
    return data;
  }

  auto power(bool reset) -> void override {
    ram.fill(0x00);
    for(u32 offset : range(min<u32>(2_KiB, rom.size()))) ram.write(offset, rom.read(offset));
    bank = isPlus() ? 0 : 6;
    mode = 0xff;
    fastJumpActive = 0;
    fastJumpStream = 0;
    immediateOperand = NoImmediate;
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
    s(fastJumpStream);
    s(immediateOperand);
    s(jumpOperand);
    for(auto& item : musicCounter) s(item);
    for(auto& item : musicFrequency) s(item);
    for(auto& item : waveformShift) s(item);
    s(audioPhase);
    harmony.serialize(s);
  }
};
