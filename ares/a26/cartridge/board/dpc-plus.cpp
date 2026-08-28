struct DPCPlus : Interface {
  using Interface::Interface;

  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n3 bank;
  n8 top[8];
  n8 bottom[8];
  n12 counter[8];
  n20 fractionalCounter[8];
  n8 fractionalIncrement[8];
  n32 musicCounter[3];
  n32 musicFrequency[3];
  n7 musicWaveform[3];
  n32 random;
  n1 fastFetch;
  n1 ldaImmediate;
  n8 parameter[8];
  n4 parameterPointer;
  u32 fractionalLowMask = 0x0f00ff;
  u32 audioPhase = 0;

  auto load() -> void override {
    Memory::Readable<n8> image;
    Interface::load(image, "program.rom");
    auto size = min<u32>(image.size(), 32_KiB);
    rom.allocate(32_KiB, 0x00);
    auto destination = 32_KiB - size;
    for(u32 offset : range(size)) rom.program(destination + offset, image.read(offset));

    if(image.size() >= 3_KiB) {
      Hash::SHA256 hash;
      for(u32 offset : range(3_KiB)) hash.input((u8)image.read(offset));
      auto driver = hash.digest();
      if(driver == "bcbe33dc02ad5104f3a3a53a856a23c57f435344790843f4fb6e746193468d0c"
        || driver == "445ca7866a0efdf0e35076a25e963708dc630f80fd82dcdaf775fb80c8b0c58d") {
        fractionalLowMask = 0x0f0000;
      }
    }
    ram.allocate(8_KiB, 0x00);
    harmony.load(cartridge.node);
  }

  auto unload() -> void override {
    harmony.unload();
  }

  auto armInvocation() const -> Harmony::Invocation override {
    return {.pc = 0x0c08, .lr = 0x0c00, .sp = 0x40001ffc};
  }

  auto readARM(u32 mode, n32 address, n32& data) -> Harmony::Access override {
    auto romBase = mode & ARM7TDMI::Prefetch ? 0x50u : 0x0c00u;
    return readARMMemory(rom, ram, mode, address, romBase, min<u32>(8_KiB, ram.size()), data);
  }

  auto writeARM(u32 mode, n32 address, n32 data) -> Harmony::Access override {
    u32 location = address;
    if(location >= 0x40000000 && location - 0x40000000 < min<u32>(8_KiB, ram.size())) {
      auto offset = location - 0x40000000;
      if(offset > 0x28 && offset < 0x0c00) return Harmony::Access::Fault;
    }
    return writeARMMemory(ram, mode, address, min<u32>(8_KiB, ram.size()), data);
  }

  auto stepARM(u32 clocks) -> void override {
    stepAudio(clocks);
  }

  auto program(u32 address) const -> n8 {
    return rom.read(3_KiB + bank * 4_KiB + (address & 0x0fff));
  }

  auto displayRead(u32 address) const -> n8 {
    return ram.read(3_KiB + (address & 0x0fff));
  }

  auto displayWrite(u32 address, n8 data) -> void {
    ram.write(3_KiB + (address & 0x0fff), data);
  }

  auto frequency(u32 note) const -> u32 {
    u32 address = 7_KiB + (note << 2);
    return ram.read(address + 0) << 0 | ram.read(address + 1) << 8
      | ram.read(address + 2) << 16 | ram.read(address + 3) << 24;
  }

  auto clockRandom() -> void {
    random = (random.bit(10) ? 0x10adab1e : 0) ^ (u32{random} >> 11 | u32{random} << 21);
  }

  auto priorRandom() -> void {
    u32 value = random;
    if(value >> 31) value ^= 0x10adab1e;
    random = value << 11 | value >> 21;
  }

  auto stepAudio(u32 clocks) -> void {
    audioPhase += clocks;
    auto ticks = audioPhase / 3500;
    audioPhase %= 3500;
    for(u32 channel : range(3)) musicCounter[channel] += u32{musicFrequency[channel]} * ticks;
  }

  auto amplitude() -> n8 {
    u32 value = 0;
    for(u32 channel : range(3)) {
      value += displayRead((u32{musicWaveform[channel]} << 5) + (u32{musicCounter[channel]} >> 27));
    }
    return value;
  }

  auto readRegister(u32 address) -> n8 {
    u32 index = address & 7;
    u32 function = address >> 3 & 7;
    u8 topDistance = u8(u32{top[index]} - (u32{counter[index]} & 0xff));
    u8 window = u8(u32{top[index]} - u32{bottom[index]});
    n8 flag = topDistance > window ? 0xff : 0x00;

    if(function == 0) {
      if(index == 0) return clockRandom(), random.byte(0);
      if(index == 1) return priorRandom(), random.byte(0);
      if(index == 2) return random.byte(1);
      if(index == 3) return random.byte(2);
      if(index == 4) return random.byte(3);
      if(index == 5) return amplitude();
      return 0;
    }
    if(function == 1) {
      auto data = displayRead(counter[index]);
      counter[index]++;
      return data;
    }
    if(function == 2) {
      auto data = displayRead(counter[index]) & flag;
      counter[index]++;
      return data;
    }
    if(function == 3) {
      auto data = displayRead(u32{fractionalCounter[index]} >> 8);
      fractionalCounter[index] += fractionalIncrement[index];
      return data;
    }
    if(function == 4 && index < 4) return flag;
    return 0;
  }

  auto callFunction(n8 function) -> void {
    u32 source = parameter[0] | parameter[1] << 8;
    if(function == 0) parameterPointer = 0;
    if(function == 1 || function == 2) {
      u32 destination = counter[parameter[2] & 7];
      u32 count = parameter[3];
      if(destination < 4_KiB) {
        count = min(count, 4_KiB - destination);
        if(function == 1 && source < rom.size() - 3_KiB) {
          count = min(count, rom.size() - 3_KiB - source);
          for(u32 offset : range(count)) displayWrite(destination + offset, rom.read(3_KiB + source + offset));
        }
        if(function == 2) {
          for(u32 offset : range(count)) displayWrite(destination + offset, parameter[0]);
        }
      }
      parameterPointer = 0;
    }
    if(function == 254 || function == 255) {
      harmony.call(function == 254);
    }
  }

  auto writeRegister(u32 address, n8 data) -> void {
    u32 index = address & 7;
    u32 function = (address - 0x28) >> 3 & 15;
    if(function == 0) fractionalCounter[index] =
      (u32{fractionalCounter[index]} & fractionalLowMask) | u32{data} << 8;
    if(function == 1) fractionalCounter[index] =
      ((u32{data} & 15) << 16) | (u32{fractionalCounter[index]} & 0xffff);
    if(function == 2) {
      fractionalIncrement[index] = data;
      fractionalCounter[index] = u32{fractionalCounter[index]} & 0x0fff00;
    }
    if(function == 3) top[index] = data;
    if(function == 4) bottom[index] = data;
    if(function == 5) counter[index] = (u32{counter[index]} & 0x0f00) | data;
    if(function == 6) {
      if(index == 0) fastFetch = data == 0;
      if(index == 1 && parameterPointer < 8) parameter[parameterPointer++] = data;
      if(index == 2) callFunction(data);
      if(index >= 5) musicWaveform[index - 5] = data & 0x7f;
    }
    if(function == 7) {
      counter[index]--;
      displayWrite(counter[index], data);
    }
    if(function == 8) counter[index] = ((u32{data} & 15) << 8) | (u32{counter[index]} & 0xff);
    if(function == 9) {
      if(index == 0) random = 0x2b435044;
      if(index >= 1 && index <= 4) random.byte(index - 1) = data;
      if(index >= 5) musicFrequency[index - 5] = frequency(data);
    }
    if(function == 10) {
      displayWrite(counter[index], data);
      counter[index]++;
    }
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    u32 offset = address & 0x0fff;
    n8 result = program(offset);

    if(fastFetch && ldaImmediate && result < 0x28) offset = result;
    ldaImmediate = 0;
    if(offset < 0x28) return readRegister(offset);

    if(offset >= 0x0ff6 && offset <= 0x0ffb) bank = offset - 0x0ff6;
    if(fastFetch) ldaImmediate = result == 0xa9;
    return result;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    u32 offset = address & 0x0fff;
    if(offset >= 0x28 && offset < 0x80) {
      writeRegister(offset, data);
      return data;
    }
    if(offset >= 0x0ff6 && offset <= 0x0ffb) bank = offset - 0x0ff6;
    return data;
  }

  auto power(bool reset) -> void override {
    ram.fill(0x00);
    if(rom.size() >= 32_KiB) {
      for(u32 offset : range(5_KiB)) ram.write(3_KiB + offset, rom.read(27_KiB + offset));
    }
    bank = 5;
    for(auto& item : top) item = 0;
    for(auto& item : bottom) item = 0;
    for(auto& item : counter) item = 0;
    for(auto& item : fractionalCounter) item = 0;
    for(auto& item : fractionalIncrement) item = 0;
    for(auto& item : musicCounter) item = 0;
    for(auto& item : musicFrequency) item = 0;
    for(auto& item : musicWaveform) item = 0;
    for(auto& item : parameter) item = 0;
    random = 0x2b435044;
    fastFetch = 0;
    ldaImmediate = 0;
    parameterPointer = 0;
    audioPhase = 0;
    harmony.power();
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(bank);
    for(auto& item : top) s(item);
    for(auto& item : bottom) s(item);
    for(auto& item : counter) s(item);
    for(auto& item : fractionalCounter) s(item);
    for(auto& item : fractionalIncrement) s(item);
    for(auto& item : musicCounter) s(item);
    for(auto& item : musicFrequency) s(item);
    for(auto& item : musicWaveform) s(item);
    s(random);
    s(fastFetch);
    s(ldaImmediate);
    for(auto& item : parameter) s(item);
    s(parameterPointer);
    s(audioPhase);
    harmony.serialize(s);
  }
};
