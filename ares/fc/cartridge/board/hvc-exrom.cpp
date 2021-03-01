struct HVC_ExROM : Interface {  //MMC5
  static auto create(string id) -> Interface* {
    if(id == "HVC-EKROM") return new HVC_ExROM(Revision::EKROM);
    if(id == "HVC-ELROM") return new HVC_ExROM(Revision::ELROM);
    if(id == "HVC-ETROM") return new HVC_ExROM(Revision::ETROM);
    if(id == "HVC-EWROM") return new HVC_ExROM(Revision::EWROM);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> exram;
  Node::Audio::Stream stream;

  enum class Revision : u32 {
    EKROM,
    ELROM,
    ETROM,
    EWROM,
  } revision;

  enum class ChipRevision : u32 {
    MMC5,
    MMC5A,
  } chipRevision;

  HVC_ExROM(Revision revision) : revision(revision) {}

  struct Envelope {
    auto volume() const -> u32 {
      return useSpeedAsVolume ? speed : decayVolume;
    }

    auto clock() -> void {
      if(reloadDecay) {
        reloadDecay = 0;
        decayVolume = 0xf;
        decayCounter = speed + 1;
        return;
      }
      if(--decayCounter == 0) {
        decayCounter = speed + 1;
        if(decayVolume || loopMode) decayVolume--;
      }
    }

    auto serialize(serializer& s) -> void {
      s(speed);
      s(useSpeedAsVolume);
      s(loopMode);
      s(reloadDecay);
      s(decayCounter);
      s(decayVolume);
    }

    n4 speed;
    n1 useSpeedAsVolume;
    n1 loopMode;
    n1 reloadDecay;
    n8 decayCounter;
    n4 decayVolume;
  };

  struct Pulse {
    //operates identically to the APU pulse channels; only without sweep support

    auto clockLength() -> void {
      if(envelope.loopMode == 0) {
        //clocked at twice the rate of the APU pulse channels
        if(lengthCounter) lengthCounter--;
        if(lengthCounter) lengthCounter--;
      }
    }

    auto clock() -> n8 {
      if(lengthCounter == 0) return 0;
      static constexpr u32 dutyTable[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},  //12.5%
        {0, 0, 0, 0, 0, 0, 1, 1},  //25.0%
        {0, 0, 0, 0, 1, 1, 1, 1},  //50.0%
        {1, 1, 1, 1, 1, 1, 0, 0},  //25.0% (negated)
      };
      n8 result = dutyTable[duty][dutyCounter] ? envelope.volume() : 0;
      if(--periodCounter == 0) {
        periodCounter = (period + 1) * 2;
        dutyCounter--;
      }
      return result;
    }

    auto serialize(serializer& s) -> void {
      s(envelope);
      s(lengthCounter);
      s(periodCounter);
      s(duty);
      s(dutyCounter);
      s(period);
    }

    Envelope envelope;
    n16 lengthCounter;
    n16 periodCounter;
    n2  duty;
    n3  dutyCounter;
    n11 period;
  };

  struct PCM {
    auto serialize(serializer& s) -> void {
      s(mode);
      s(irqEnable);
      s(irqLine);
      s(dac);
    }

    n1 mode;
    n1 irqEnable;
    n1 irqLine;
    n8 dac;
  };

  struct Pin {
    auto serialize(serializer& s) -> void {
      s(source);
      s(direction);
      s(line);
    }

    n1 source;     //0 = $5208, 1 = $5800-5bff
    n1 direction;  //0 = output, 1 = input
    n1 line;
  };

  auto load() -> void override {
    chipRevision = ChipRevision::MMC5A;

    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    exram.allocate(1_KiB);

    stream = cartridge.node->append<Node::Audio::Stream>("MMC5");
    stream->setChannels(1);
    stream->setFrequency(u32(system.frequency() + 0.5) / cartridge.rate());
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
  }

  auto unload() -> void override {
    cartridge.node->remove(stream);
    stream.reset();
  }

  auto main() -> void override {
    //scanline() resets this; if no scanlines detected, enter video blanking period
    if(cycleCounter >= 200) blank();  //113-114 normal; ~2500 across Vblank period
    else cycleCounter++;

    if(timerCounter && --timerCounter == 0) {
      timerLine = 1;
    }

    i32 output = 0;
    output += apu.pulseDAC[pulse1.clock() + pulse2.clock()];
    output += pcm.dac << 7;
    stream->frame(sclamp<16>(-output) / 32768.0);

    cpu.irqLine((irqLine & irqEnable) || (pcm.irqLine & pcm.irqEnable) || timerLine);

    tick();
  }

  auto blank() -> void {
    inFrame = 0;
  }

  auto scanline() -> void {
    cycleCounter = 0;
    hcounter = 0;

    if(!inFrame) {
      inFrame = 1;
      irqLine = 0;
      vcounter = 0;
    } else {
      if(vcounter == irqCoincidence) irqLine = 1;
      vcounter++;
    }
  }

  auto accessPRG(bool write, n32 address, n8 data = 0x00) -> n8 {
    n8 bank;

    if((address & 0xe000) == 0x6000) {
      bank = ramSelect << 2 | ramBank;
      address &= 0x1fff;
    } else if(programMode == 0) {
      bank = programBank[3] & ~3;
      address &= 0x7fff;
    } else if(programMode == 1) {
      if((address & 0xc000) == 0x8000) bank = programBank[1] & ~1;
      if((address & 0xe000) == 0xc000) bank = programBank[3] & ~1;
      address &= 0x3fff;
    } else if(programMode == 2) {
      if((address & 0xe000) == 0x8000) bank = programBank[1] & ~1 | 0;
      if((address & 0xe000) == 0xa000) bank = programBank[1] & ~1 | 1;
      if((address & 0xe000) == 0xc000) bank = programBank[2];
      if((address & 0xe000) == 0xe000) bank = programBank[3];
      address &= 0x1fff;
    } else if(programMode == 3) {
      if((address & 0xe000) == 0x8000) bank = programBank[0];
      if((address & 0xe000) == 0xa000) bank = programBank[1];
      if((address & 0xe000) == 0xc000) bank = programBank[2];
      if((address & 0xe000) == 0xe000) bank = programBank[3];
      address &= 0x1fff;
    }

    n1 rom = bank.bit(7);
    bank.bit(7) = 0;

    if(!write) {
      if(rom) {
        return programROM.read(bank << 13 | address);
      } else {
        return programRAM.read(bank << 13 | address);
      }
    } else {
      if(rom) {
        programROM.write(bank << 13 | address, data);
      } else {
        if(ramWriteProtect[0] == 2 && ramWriteProtect[1] == 1) {
          programRAM.write(bank << 13 | address, data);
        }
      }
      return 0x00;
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if((address & 0xfc00) == 0x5800) {
      if(chipRevision != ChipRevision::MMC5A) return data;
      if(cl3.direction == 1) cl3.line = 0;  //!M2
      if(sl3.direction == 1) sl3.line = 0;  //!M2
      return data;
    }

    if((address & 0xfc00) == 0x5c00) {
      if(exramMode >= 2) return exram[(n10)address];
      return data;
    }

    if(address >= 0x6000) {
      data = accessPRG(0, address);
      if(pcm.mode == 1 && (address & 0xc000) == 0x8000) pcm.dac = data;
      return data;
    }

    switch(address) {
    case 0x5010: {
      n8 data;
      data.bit(0) = pcm.mode;
      data.bit(7) = pcm.irqLine & pcm.irqEnable;
      pcm.irqLine = 0;
      return data;
    }

    case 0x5015: {
      n8 data;
      data.bit(0) = (bool)pulse1.lengthCounter;
      data.bit(1) = (bool)pulse2.lengthCounter;
      return data;
    }

    case 0x5204: {
      n8 data;
      data.bit(6) = inFrame;
      data.bit(7) = irqLine;
      irqLine = 0;
      return data;
    }

    case 0x5205:
      return multiplier * multiplicand >> 0;

    case 0x5206:
      return multiplier * multiplicand >> 8;

    case 0x5208: {
      if(chipRevision != ChipRevision::MMC5A) break;
      n8 data;
      data.bit(6) = cl3.line;
      data.bit(7) = sl3.line;
      return data;
    }

    case 0x5209: {
      if(chipRevision != ChipRevision::MMC5A) break;
      n8 data;
      data.bit(7) = timerLine;
      timerLine = 0;
      return data;
    }
    }

    return 0x00;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if((address & 0xfc00) == 0x5800) {
      if(chipRevision != ChipRevision::MMC5A) return;
      if(cl3.direction == 1) cl3.line = 0;  //!M2
      if(sl3.direction == 1) sl3.line = 0;  //!M2
      return;
    }

    if((address & 0xfc00) == 0x5c00) {
      //writes 0x00 *during* Vblank (not during screen rendering ...)
      if(exramMode == 0 || exramMode == 1) exram[(n10)address] = inFrame ? data : (n8)0x00;
      if(exramMode == 2) exram[(n10)address] = data;
      return;
    }

    if(address >= 0x6000) {
      accessPRG(1, address, data);
      return;
    }

    switch(address) {
    case 0x2000:
      sprite8x16 = data.bit(5);
      break;

    case 0x2001:
      //if background + sprites are disabled; enter video blanking period
      if(!data.bit(3,4)) blank();
      break;

    case 0x5000:
      pulse1.envelope.speed = data.bit(0,3);
      pulse1.envelope.useSpeedAsVolume = data.bit(4);
      pulse1.envelope.loopMode = data.bit(5);
      pulse1.duty = data.bit(6,7);
      break;

    case 0x5001:
      break;

    case 0x5002:
      pulse1.period.bit(0,7) = data.bit(0,7);
      break;

    case 0x5003:
      pulse1.period.bit(8,10) = data.bit(0,2);
      pulse1.dutyCounter = 0;
      pulse1.envelope.reloadDecay = 1;
      pulse1.lengthCounter = apu.lengthCounterTable[data.bit(3,7)];
      break;

    case 0x5004:
      pulse2.envelope.speed = data.bit(0,3);
      pulse2.envelope.useSpeedAsVolume = data.bit(4);
      pulse2.envelope.loopMode = data.bit(5);
      pulse2.duty = data.bit(6,7);
      break;

    case 0x5005:
      break;

    case 0x5006:
      pulse2.period.bit(0,7) = data.bit(0,7);
      break;

    case 0x5007:
      pulse2.period.bit(8,10) = data.bit(0,2);
      pulse2.dutyCounter = 0;
      pulse2.envelope.reloadDecay = 1;
      pulse2.lengthCounter = apu.lengthCounterTable[data.bit(3,7)];
      break;

    case 0x5010:
      pcm.mode = data.bit(0);
      pcm.irqEnable = data.bit(7);
      break;

    case 0x5011:
      if(pcm.mode == 0) {
        if(data == 0x00) pcm.irqLine = 1;
        if(data != 0x00) pcm.dac = data;
      }
      break;

    case 0x5100:
      programMode = data.bit(0,1);
      break;

    case 0x5101:
      characterMode = data.bit(0,1);
      break;

    case 0x5102:
      ramWriteProtect[0] = data.bit(0,1);
      break;

    case 0x5103:
      ramWriteProtect[1] = data.bit(0,1);
      break;

    case 0x5104:
      exramMode = data.bit(0,1);
      break;

    case 0x5105:
      nametableMode[0] = data.bit(0,1);
      nametableMode[1] = data.bit(2,3);
      nametableMode[2] = data.bit(4,5);
      nametableMode[3] = data.bit(6,7);
      break;

    case 0x5106:
      fillmodeTile = data;
      break;

    case 0x5107:
      fillmodeColor.bit(0,1) = data.bit(0,1);
      fillmodeColor.bit(2,3) = data.bit(0,1);
      fillmodeColor.bit(4,5) = data.bit(0,1);
      fillmodeColor.bit(6,7) = data.bit(0,1);
      break;

    case 0x5113:
      ramBank   = data.bit(0,1);
      ramSelect = data.bit(2);
      break;

    case 0x5114:
      programBank[0] = data;
      break;

    case 0x5115:
      programBank[1] = data;
      break;

    case 0x5116:
      programBank[2] = data;
      break;

    case 0x5117:
      programBank[3] = data | 0x80;
      break;

    case 0x5120:
      characterSpriteBank[0] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5121:
      characterSpriteBank[1] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5122:
      characterSpriteBank[2] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5123:
      characterSpriteBank[3] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5124:
      characterSpriteBank[4] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5125:
      characterSpriteBank[5] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5126:
      characterSpriteBank[6] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5127:
      characterSpriteBank[7] = characterBankHi << 8 | data;
      characterActive = 0;
      break;

    case 0x5128:
      characterBackgroundBank[0] = characterBankHi << 8 | data;
      characterActive = 1;
      break;

    case 0x5129:
      characterBackgroundBank[1] = characterBankHi << 8 | data;
      characterActive = 1;
      break;

    case 0x512a:
      characterBackgroundBank[2] = characterBankHi << 8 | data;
      characterActive = 1;
      break;

    case 0x512b:
      characterBackgroundBank[3] = characterBankHi << 8 | data;
      characterActive = 1;
      break;

    case 0x5130:
      characterBankHi = data.bit(0,1);
      break;

    case 0x5200:
      vsplitTile   = data.bit(0,4);
      vsplitSide   = data.bit(6);
      vsplitEnable = data.bit(7);
      break;

    case 0x5201:
      vsplitScroll = data;
      break;

    case 0x5202:
      vsplitBank = data;
      break;

    case 0x5203:
      irqCoincidence = data;
      break;

    case 0x5204:
      irqEnable = data.bit(7);
      break;

    case 0x5205:
      multiplicand = data;
      break;

    case 0x5206:
      multiplier = data;
      break;

    case 0x5207:
      if(chipRevision != ChipRevision::MMC5A) return;
      cl3.source    = data.bit(0);
      sl3.source    = data.bit(1);
      cl3.direction = data.bit(6);
      sl3.direction = data.bit(7);
      break;

    case 0x5208:
      if(chipRevision != ChipRevision::MMC5A) return;
      if(cl3.source == 0 && cl3.direction == 0) cl3.line = data.bit(6);
      if(sl3.source == 0 && sl3.direction == 0) sl3.line = data.bit(7);
      break;

    case 0x5209:
      if(chipRevision != ChipRevision::MMC5A) return;
      timerCounter.bit(0,7) = data.bit(0,7);
      break;

    case 0x520a:
      if(chipRevision != ChipRevision::MMC5A) return;
      timerCounter.bit(8,15) = data.bit(0,7);
      break;
    }
  }

  auto readCIRAM(n32 address) -> n8 {
    if(vsplitFetch && (hcounter & 2) == 0) return exram[vsplitVoffset / 8 * 32 + vsplitHoffset / 8];
    if(vsplitFetch && (hcounter & 2) != 0) return exram[vsplitVoffset / 32 * 8 + vsplitHoffset / 32 + 0x03c0];

    switch(nametableMode[address >> 10 & 3]) {
    case 0: return ppu.readCIRAM(0x0000 | (n10)address);
    case 1: return ppu.readCIRAM(0x0400 | (n10)address);
    case 2: return exramMode < 2 ? exram[(n10)address] : (n8)0x00;
    case 3: return (hcounter & 2) == 0 ? fillmodeTile : fillmodeColor;
    }
    unreachable;
  }

  auto characterSpriteAddress(n32 address) -> n32 {
    if(characterMode == 0) {
      auto bank = characterSpriteBank[7];
      return bank << 13 | (n13)address;
    }

    if(characterMode == 1) {
      auto bank = characterSpriteBank[(address >> 12) * 4 + 3];
      return bank << 12 | (n12)address;
    }

    if(characterMode == 2) {
      auto bank = characterSpriteBank[(address >> 11) * 2 + 1];
      return bank << 11 | (n11)address;
    }

    if(characterMode == 3) {
      auto bank = characterSpriteBank[(address >> 10)];
      return bank << 10 | (n10)address;
    }

    unreachable;
  }

  auto characterBackgroundAddress(n32 address) -> n32 {
    address &= 0x0fff;

    if(characterMode == 0) {
      auto bank = characterBackgroundBank[3];
      return bank << 13 | (n12)address;
    }

    if(characterMode == 1) {
      auto bank = characterBackgroundBank[3];
      return bank << 12 | (n12)address;
    }

    if(characterMode == 2) {
      auto bank = characterBackgroundBank[(address >> 11) * 2 + 1];
      return bank << 11 | (n11)address;
    }

    if(characterMode == 3) {
      auto bank = characterBackgroundBank[(address >> 10)];
      return bank << 10 | (n10)address;
    }

    unreachable;
  }

  auto characterVsplitAddress(n32 address) -> n32 {
    return vsplitBank << 12 | address & 0x0ff8 | vsplitVoffset & 7;
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    characterAccess[0] = characterAccess[1];
    characterAccess[1] = characterAccess[2];
    characterAccess[2] = characterAccess[3];
    characterAccess[3] = address;

    //detect two unused nametable fetches at end of each scanline
    if(characterAccess[0].bit(13) == 0
    && characterAccess[1].bit(13) == 1
    && characterAccess[2].bit(13) == 1
    && characterAccess[3].bit(13) == 1) scanline();

    if(inFrame == false) {
      vsplitFetch = 0;
      if(address & 0x2000) return readCIRAM(address);
      return characterROM.read(characterActive ? characterBackgroundAddress(address) : characterSpriteAddress(address));
    }

    n1 backgroundFetch = (hcounter < 256 || hcounter >= 320);
    data = 0x00;

    if((hcounter & 7) == 0) {
      vsplitHoffset = hcounter >= 320 ? hcounter - 320 : hcounter + 16;
      vsplitVoffset = vcounter + vsplitScroll;
      vsplitFetch = vsplitEnable && backgroundFetch && exramMode < 2
      && (vsplitSide ? vsplitHoffset / 8 >= vsplitTile : vsplitHoffset / 8 < vsplitTile);
      if(vsplitVoffset >= 240) vsplitVoffset -= 240;

      data = readCIRAM(address);

      exbank = characterBankHi << 6 | exram[(n10)address].bit(0,5);
      exattr.bit(0,1) = exram[(n10)address].bit(6,7);
      exattr.bit(2,3) = exram[(n10)address].bit(6,7);
      exattr.bit(4,5) = exram[(n10)address].bit(6,7);
      exattr.bit(6,7) = exram[(n10)address].bit(6,7);
    } else if((hcounter & 7) == 2) {
      data = readCIRAM(address);
      if(backgroundFetch && exramMode == 1) data = exattr;
    } else {
      if(vsplitFetch) data = characterROM.read(characterVsplitAddress(address));
      else if(sprite8x16 ? backgroundFetch : characterActive) data = characterROM.read(characterBackgroundAddress(address));
      else data = characterROM.read(characterSpriteAddress(address));
      if(backgroundFetch && exramMode == 1) data = characterROM.read(exbank << 12 | (n12)address);
    }

    hcounter += 2;
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) {
      switch(nametableMode[address >> 10 & 3]) {
      case 0: return ppu.writeCIRAM(0x0000 | (n10)address, data);
      case 1: return ppu.writeCIRAM(0x0400 | (n10)address, data);
      case 2: exram[(n10)address] = data; break;
      }
    }
  }

  auto power() -> void override {
    for(auto& byte : exram) byte = 0xff;
    programMode = 3;
    programBank[3] = 0xff;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(exram);
    s(pulse1);
    s(pulse2);
    s(pcm);
    s(cl3);
    s(sl3);
    s(programMode);
    s(characterMode);
    s(ramWriteProtect);
    s(exramMode);
    s(nametableMode);
    s(fillmodeTile);
    s(fillmodeColor);
    s(ramSelect);
    s(ramBank);
    s(programBank);
    s(characterSpriteBank);
    s(characterBackgroundBank);
    s(characterBankHi);
    s(vsplitEnable);
    s(vsplitSide);
    s(vsplitTile);
    s(vsplitScroll);
    s(vsplitBank);
    s(irqCoincidence);
    s(irqEnable);
    s(multiplicand);
    s(multiplier);
    s(timerCounter);
    s(timerLine);
    s(cycleCounter);
    s(irqLine);
    s(inFrame);
    s(vcounter);
    s(hcounter);
    s(characterAccess);
    s(characterActive);
    s(sprite8x16);
    s(exbank);
    s(exattr);
    s(vsplitFetch);
    s(vsplitVoffset);
    s(vsplitHoffset);
  }

  //programmable registers

  Pulse pulse1;  //$5000-5003
  Pulse pulse2;  //$5004-5007
  PCM   pcm;     //$5010-5011

  Pin cl3;  //$5207-5208
  Pin sl3;  //$5207-5208

  n2  programMode;    //$5100
  n2  characterMode;  //$5101

  n2  ramWriteProtect[2];  //$5102-$5103

  n2  exramMode;         //$5104
  n2  nametableMode[4];  //$5105
  n8  fillmodeTile;      //$5106
  n8  fillmodeColor;     //$5107

  n1  ramSelect;                   //$5113
  n2  ramBank;                     //$5113
  n8  programBank[4];              //$5114-5117
  n10 characterSpriteBank[8];      //$5120-5127
  n10 characterBackgroundBank[4];  //$5128-512b
  n2  characterBankHi;             //$5130

  n1  vsplitEnable;  //$5200
  n1  vsplitSide;    //$5200
  n5  vsplitTile;    //$5200
  n8  vsplitScroll;  //$5201
  n8  vsplitBank;    //$5202

  n8  irqCoincidence;  //$5203
  n1  irqEnable;       //$5204

  n8  multiplicand;  //$5205
  n8  multiplier;    //$5206

  n16 timerCounter;  //$5209-520a
  n1  timerLine;

  //status registers

  n8  cycleCounter;
  n1  irqLine;
  n1  inFrame;

  n16 vcounter;
  n16 hcounter;
  n16 characterAccess[4];
  n1  characterActive;
  n1  sprite8x16;

  n8  exbank;
  n8  exattr;

  n1  vsplitFetch;
  n8  vsplitVoffset;
  n8  vsplitHoffset;
};
