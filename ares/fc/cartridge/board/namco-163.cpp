struct Namco163 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "NAMCO-163") return new Namco163;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> soundRAM;
  Node::Audio::Stream stream;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    soundRAM.allocate(128);

    stream = cartridge.node->append<Node::Audio::Stream>("N163");
    stream->setChannels(1);
    stream->setFrequency(u32(system.frequency() + 0.5) / cartridge.rate() / 15);
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto unload() -> void override {
    cartridge.node->remove(stream);
    stream.reset();
  }

  auto main() -> void override {
    cpu.irqLine(irqLine);
    if(irqEnable) {
      if(irqCounter != 0x7fff && ++irqCounter == 0x7fff) irqLine = 1;
    }

    if(++soundDivider == 15) {
      soundDivider = 0;
      double output = clockSound();
      stream->frame(output / 255.0 * 0.5);
    }

    tick();
  }

  auto clockSound() -> double {
    if(!soundEnable) return 0;

    n7 address = 0x40 | (7 - soundChannel) << 3;
    n24 phase = soundRAM[address + 5] << 16 | soundRAM[address + 3] << 8 | soundRAM[address + 1];
    n18 freq  = soundRAM[address + 4] << 16 | soundRAM[address + 2] << 8 | soundRAM[address + 0];
    n8 length = 256 - (soundRAM[address + 4] & 0xfc);
    n8 offset = soundRAM[address + 6];
    n4 volume = soundRAM[address + 7];

    phase = length ? (phase + freq) % (length << 16) : 0;
    offset = (phase >> 16) + offset;
    n4 sample = soundRAM[offset >> 1] >> (offset.bit(0) ? 4 : 0);

    soundOutput[soundChannel] = (sample - 8) * volume;
    soundRAM[address + 1] = phase >> 0;
    soundRAM[address + 3] = phase >> 8;
    soundRAM[address + 5] = phase >> 16;

    n4 channels = soundRAM[0x7f].bit(4,6) + 1;
    if(++soundChannel == channels) soundChannel = 0;

    double output = 0;
    for(u32 i : range(channels)) output += soundOutput[i];
    return output / channels;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x4800) return data;
    if(address < 0x5000) {
      data = soundRAM.read(soundAddress);
      if(soundIncrement) soundAddress++;
      return data;
    }

    if(address < 0x6000) {
      if(address < 0x5800) return irqCounter.bit(0,7);
      if(address < 0x6000) return irqCounter.bit(8,14) | irqEnable << 7;
    }

    if(address < 0x8000) {
      if(!programRAM) return data;
      return programRAM.read((n13)address);
    }

    n6 bank;
    switch(address >> 13 & 3) {
    case 0: bank = programBank[0]; break;
    case 1: bank = programBank[1]; break;
    case 2: bank = programBank[2]; break;
    case 3: bank = 0x3f; break;
    }
    return programROM.read(bank << 13 | (n13)address);
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x6000 && address < 0x8000) {
      if(!programRAM) return;
      if(!ramEnable[address >> 11 & 3]) return;
      return programRAM.write((n13)address, data);
    }

    switch(address & 0xf800) {
    case 0x4800:
      soundRAM.write(soundAddress, data);
      if(soundIncrement) soundAddress++;
      break;
    case 0x5000:
      irqCounter.bit(0,7) = data;
      irqLine = 0;
      break;
    case 0x5800:
      irqCounter.bit(8,14) = data.bit(0,6);
      irqEnable = data.bit(7);
      irqLine = 0;
      break;
    case 0x8000: characterBank[0]  = data; break;
    case 0x8800: characterBank[1]  = data; break;
    case 0x9000: characterBank[2]  = data; break;
    case 0x9800: characterBank[3]  = data; break;
    case 0xa000: characterBank[4]  = data; break;
    case 0xa800: characterBank[5]  = data; break;
    case 0xb000: characterBank[6]  = data; break;
    case 0xb800: characterBank[7]  = data; break;
    case 0xc000: characterBank[8]  = data; break;
    case 0xc800: characterBank[9]  = data; break;
    case 0xd000: characterBank[10] = data; break;
    case 0xd800: characterBank[11] = data; break;
    case 0xe000:
      programBank[0] = data.bit(0,5);
      soundEnable = !data.bit(6);
      break;
    case 0xe800:
      programBank[1] = data.bit(0,5);
      ciramEnable[0] = !data.bit(6);
      ciramEnable[1] = !data.bit(7);
      break;
    case 0xf000:
      programBank[2] = data.bit(0,5);
      break;
    case 0xf800:
      ramEnable[0] = data.bit(4,7) == 4 && !data.bit(0);
      ramEnable[1] = data.bit(4,7) == 4 && !data.bit(1);
      ramEnable[2] = data.bit(4,7) == 4 && !data.bit(2);
      ramEnable[3] = data.bit(4,7) == 4 && !data.bit(3);
      soundAddress = data.bit(0,6);
      soundIncrement = data.bit(7);
      break;
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) address.bit(12) = 0;
    bool ciramSelect = address & 0x2000 || ciramEnable[address >> 12 & 1];
    n8 bank = characterBank[address >> 10];
    if(bank >= 0xe0 && ciramSelect) {
      return ppu.readCIRAM(bank.bit(0) << 10 | (n10)address);
    }
    return characterROM.read(bank << 10 | (n10)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) address.bit(12) = 0;
    bool ciramSelect = address & 0x2000 || ciramEnable[address >> 12 & 1];
    n8 bank = characterBank[address >> 10];
    if(bank >= 0xe0 && ciramSelect) {
      return ppu.writeCIRAM(bank.bit(0) << 10 | (n10)address, data);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(soundRAM);
    s(ramEnable);
    s(ciramEnable);
    s(programBank);
    s(characterBank);
    s(soundOutput);
    s(soundAddress);
    s(soundIncrement);
    s(soundEnable);
    s(soundChannel);
    s(soundDivider);
    s(irqCounter);
    s(irqEnable);
    s(irqLine);
  }

  n1  ramEnable[4];
  n1  ciramEnable[2];
  n6  programBank[3];
  n8  characterBank[12];
  i16 soundOutput[8];
  n7  soundAddress;
  n1  soundIncrement;
  n1  soundEnable;
  n3  soundChannel;
  n4  soundDivider;
  n15 irqCounter;
  n1  irqEnable;
  n1  irqLine;
};
