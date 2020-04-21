struct HVC_ExROM : Interface {  //MMC5
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> exram;
  Node::Stream stream;

  enum class Revision : uint {
    EKROM,
    ELROM,
    ETROM,
    EWROM,
  } revision;

  enum class ChipRevision : uint {
    MMC5,
    MMC5A,
  } chipRevision;

  HVC_ExROM(Markup::Node document, Revision revision) : Interface(document), revision(revision) {}

  struct Envelope {
    auto volume() const -> uint {
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
      s.integer(speed);
      s.integer(useSpeedAsVolume);
      s.integer(loopMode);
      s.integer(reloadDecay);
      s.integer(decayCounter);
      s.integer(decayVolume);
    }

    uint4 speed;
    uint1 useSpeedAsVolume;
    uint1 loopMode;
    uint1 reloadDecay;
    uint8 decayCounter;
    uint4 decayVolume;
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

    auto clock() -> uint8 {
      if(lengthCounter == 0) return 0;
      static const uint dutyTable[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},  //12.5%
        {0, 0, 0, 0, 0, 0, 1, 1},  //25.0%
        {0, 0, 0, 0, 1, 1, 1, 1},  //50.0%
        {1, 1, 1, 1, 1, 1, 0, 0},  //25.0% (negated)
      };
      uint8 result = dutyTable[duty][dutyCounter] ? envelope.volume() : 0;
      if(--periodCounter == 0) {
        periodCounter = (period + 1) * 2;
        dutyCounter--;
      }
      return result;
    }

    auto serialize(serializer& s) -> void {
      envelope.serialize(s);
      s.integer(lengthCounter);
      s.integer(periodCounter);
      s.integer(duty);
      s.integer(dutyCounter);
      s.integer(period);
    }

    Envelope envelope;
    uint16 lengthCounter;
    uint16 periodCounter;
     uint2 duty;
     uint3 dutyCounter;
    uint11 period;
  };

  struct PCM {
    auto serialize(serializer& s) -> void {
      s.integer(mode);
      s.integer(irqEnable);
      s.integer(irqLine);
      s.integer(dac);
    }

    uint1 mode;
    uint1 irqEnable;
    uint1 irqLine;
    uint8 dac;
  };

  struct Pin {
    auto serialize(serializer& s) -> void {
      s.integer(source);
      s.integer(direction);
      s.integer(line);
    }

    uint1 source;     //0 = $5208, 1 = $5800-5bff
    uint1 direction;  //0 = output, 1 = input
    uint1 line;
  };

  auto load(Markup::Node document) -> void override {
    chipRevision = ChipRevision::MMC5A;

    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    exram.allocate(1_KiB);

    stream = cartridge.node->append<Node::Stream>("MMC5");
    stream->setChannels(1);
    stream->setFrequency(uint(system.frequency() + 0.5) / cartridge.rate());
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
  }

  auto main() -> void override {
    //scanline() resets this; if no scanlines detected, enter video blanking period
    if(cycleCounter >= 200) blank();  //113-114 normal; ~2500 across Vblank period
    else cycleCounter++;

    if(timerCounter && --timerCounter == 0) {
      timerLine = 1;
    }

    int output = 0;
    output += apu.pulseDAC[pulse1.clock() + pulse2.clock()];
    output += pcm.dac << 7;
    stream->sample(sclamp<16>(-output) / 32768.0);

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

  auto accessPRG(bool write, uint address, uint8 data = 0x00) -> uint8 {
    uint8 bank;

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

    uint1 rom = bank.bit(7);
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

  auto readPRG(uint address) -> uint8 {
    if((address & 0xfc00) == 0x5800) {
      if(chipRevision != ChipRevision::MMC5A) return cpu.mdr();
      if(cl3.direction == 1) cl3.line = 0;  //!M2
      if(sl3.direction == 1) sl3.line = 0;  //!M2
      return cpu.mdr();
    }

    if((address & 0xfc00) == 0x5c00) {
      if(exramMode >= 2) return exram[(uint10)address];
      return cpu.mdr();
    }

    if(address >= 0x6000) {
      uint8 data = accessPRG(0, address);
      if(pcm.mode == 1 && (address & 0xc000) == 0x8000) pcm.dac = data;
      return data;
    }

    switch(address) {
    case 0x5010: {
      uint8 data;
      data.bit(0) = pcm.mode;
      data.bit(7) = pcm.irqLine & pcm.irqEnable;
      pcm.irqLine = 0;
      return data;
    }

    case 0x5015: {
      uint8 data;
      data.bit(0) = (bool)pulse1.lengthCounter;
      data.bit(1) = (bool)pulse2.lengthCounter;
      return data;
    }

    case 0x5204: {
      uint8 data;
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
      uint8 data;
      data.bit(6) = cl3.line;
      data.bit(7) = sl3.line;
      return data;
    }

    case 0x5209: {
      if(chipRevision != ChipRevision::MMC5A) break;
      uint8 data;
      data.bit(7) = timerLine;
      timerLine = 0;
      return data;
    }
    }

    return 0x00;
  }

  auto writePRG(uint address, uint8 data) -> void {
    if((address & 0xfc00) == 0x5800) {
      if(chipRevision != ChipRevision::MMC5A) return;
      if(cl3.direction == 1) cl3.line = 0;  //!M2
      if(sl3.direction == 1) sl3.line = 0;  //!M2
      return;
    }

    if((address & 0xfc00) == 0x5c00) {
      //writes 0x00 *during* Vblank (not during screen rendering ...)
      if(exramMode == 0 || exramMode == 1) exram[(uint10)address] = inFrame ? data : (uint8)0x00;
      if(exramMode == 2) exram[(uint10)address] = data;
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

  auto readCIRAM(uint address) -> uint8 {
    if(vsplitFetch && (hcounter & 2) == 0) return exram[vsplitVoffset / 8 * 32 + vsplitHoffset / 8];
    if(vsplitFetch && (hcounter & 2) != 0) return exram[vsplitVoffset / 32 * 8 + vsplitHoffset / 32 + 0x03c0];

    switch(nametableMode[address >> 10 & 3]) {
    case 0: return ppu.readCIRAM(0x0000 | address & 0x03ff);
    case 1: return ppu.readCIRAM(0x0400 | address & 0x03ff);
    case 2: return exramMode < 2 ? exram[(uint10)address] : (uint8)0x00;
    case 3: return (hcounter & 2) == 0 ? fillmodeTile : fillmodeColor;
    }
    unreachable;
  }

  auto characterSpriteAddress(uint address) -> uint {
    if(characterMode == 0) {
      auto bank = characterSpriteBank[7];
      return bank << 13 | (uint13)address;
    }

    if(characterMode == 1) {
      auto bank = characterSpriteBank[(address >> 12) * 4 + 3];
      return bank << 12 | (uint12)address;
    }

    if(characterMode == 2) {
      auto bank = characterSpriteBank[(address >> 11) * 2 + 1];
      return bank << 11 | (uint11)address;
    }

    if(characterMode == 3) {
      auto bank = characterSpriteBank[(address >> 10)];
      return bank << 10 | (uint10)address;
    }

    unreachable;
  }

  auto characterBackgroundAddress(uint address) -> uint {
    address &= 0x0fff;

    if(characterMode == 0) {
      auto bank = characterBackgroundBank[3];
      return bank << 13 | (uint12)address;
    }

    if(characterMode == 1) {
      auto bank = characterBackgroundBank[3];
      return bank << 12 | (uint12)address;
    }

    if(characterMode == 2) {
      auto bank = characterBackgroundBank[(address >> 11) * 2 + 1];
      return bank << 11 | (uint11)address;
    }

    if(characterMode == 3) {
      auto bank = characterBackgroundBank[(address >> 10)];
      return bank << 10 | (uint10)address;
    }

    unreachable;
  }

  auto characterVsplitAddress(uint address) -> uint {
    return vsplitBank << 12 | address & 0x0ff8 | vsplitVoffset & 7;
  }

  auto readCHR(uint address) -> uint8 {
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

    uint1 backgroundFetch = (hcounter < 256 || hcounter >= 320);
    uint8 data = 0x00;

    if((hcounter & 7) == 0) {
      vsplitHoffset = hcounter >= 320 ? hcounter - 320 : hcounter + 16;
      vsplitVoffset = vcounter + vsplitScroll;
      vsplitFetch = vsplitEnable && backgroundFetch && exramMode < 2
      && (vsplitSide ? vsplitHoffset / 8 >= vsplitTile : vsplitHoffset / 8 < vsplitTile);
      if(vsplitVoffset >= 240) vsplitVoffset -= 240;

      data = readCIRAM(address);

      exbank = characterBankHi << 6 | exram[(uint10)address].bit(0,5);
      exattr.bit(0,1) = exram[(uint10)address].bit(6,7);
      exattr.bit(2,3) = exram[(uint10)address].bit(6,7);
      exattr.bit(4,5) = exram[(uint10)address].bit(6,7);
      exattr.bit(6,7) = exram[(uint10)address].bit(6,7);
    } else if((hcounter & 7) == 2) {
      data = readCIRAM(address);
      if(backgroundFetch && exramMode == 1) data = exattr;
    } else {
      if(vsplitFetch) data = characterROM.read(characterVsplitAddress(address));
      else if(sprite8x16 ? backgroundFetch : characterActive) data = characterROM.read(characterBackgroundAddress(address));
      else data = characterROM.read(characterSpriteAddress(address));
      if(backgroundFetch && exramMode == 1) data = characterROM.read(exbank << 12 | (uint12)address);
    }

    hcounter += 2;
    return data;
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address & 0x2000) {
      switch(nametableMode[address >> 10 & 3]) {
      case 0: return ppu.writeCIRAM(0x0000 | address & 0x03ff, data);
      case 1: return ppu.writeCIRAM(0x0400 | address & 0x03ff, data);
      case 2: exram[(uint10)address] = data; break;
      }
    }
  }

  auto power() -> void {
    pulse1 = {};
    pulse2 = {};
    pcm = {};
    for(auto& byte : exram) byte = 0xff;
    programMode = 3;
    characterMode = 0;
    for(auto& writeProtect : ramWriteProtect) writeProtect = 0;
    exramMode = 0;
    for(auto& mode : nametableMode) mode = 0;
    fillmodeTile = 0;
    fillmodeColor = 0;
    ramSelect = 0;
    ramBank = 0;
    programBank[0] = 0x00;
    programBank[1] = 0x00;
    programBank[2] = 0x00;
    programBank[3] = 0xff;
    for(auto& bank : characterSpriteBank) bank = 0;
    for(auto& bank : characterBackgroundBank) bank = 0;
    characterBankHi = 0;
    vsplitEnable = 0;
    vsplitSide = 0;
    vsplitTile = 0;
    vsplitScroll = 0;
    vsplitBank = 0;
    irqCoincidence = 0;
    irqEnable = 0;
    multiplicand = 0;
    multiplier = 0;
    timerCounter = 0;
    timerLine = 0;
    cycleCounter = 0;
    irqLine = 0;
    inFrame = 0;
    vcounter = 0;
    hcounter = 0;
    for(auto& access : characterAccess) access = 0;
    characterActive = 0;
    sprite8x16 = 0;
    exbank = 0;
    exattr = 0;
    vsplitFetch = 0;
    vsplitVoffset = 0;
    vsplitHoffset = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    exram.serialize(s);
    pulse1.serialize(s);
    pulse2.serialize(s);
    pcm.serialize(s);
    cl3.serialize(s);
    sl3.serialize(s);
    s.integer(programMode);
    s.integer(characterMode);
    s.array(ramWriteProtect);
    s.integer(exramMode);
    s.array(nametableMode);
    s.integer(fillmodeTile);
    s.integer(fillmodeColor);
    s.integer(ramSelect);
    s.integer(ramBank);
    s.array(programBank);
    s.array(characterSpriteBank);
    s.array(characterBackgroundBank);
    s.integer(characterBankHi);
    s.integer(vsplitEnable);
    s.integer(vsplitSide);
    s.integer(vsplitTile);
    s.integer(vsplitScroll);
    s.integer(vsplitBank);
    s.integer(irqCoincidence);
    s.integer(irqEnable);
    s.integer(multiplicand);
    s.integer(multiplier);
    s.integer(timerCounter);
    s.integer(timerLine);
    s.integer(cycleCounter);
    s.integer(irqLine);
    s.integer(inFrame);
    s.integer(vcounter);
    s.integer(hcounter);
    s.array(characterAccess);
    s.integer(characterActive);
    s.integer(sprite8x16);
    s.integer(exbank);
    s.integer(exattr);
    s.integer(vsplitFetch);
    s.integer(vsplitVoffset);
    s.integer(vsplitHoffset);
  }

  //programmable registers

   Pulse pulse1;  //$5000-5003
   Pulse pulse2;  //$5004-5007
     PCM pcm;     //$5010-5011

     Pin cl3;  //$5207-5208
     Pin sl3;  //$5207-5208

   uint2 programMode;    //$5100
   uint2 characterMode;  //$5101

   uint2 ramWriteProtect[2];  //$5102-$5103

   uint2 exramMode;         //$5104
   uint2 nametableMode[4];  //$5105
   uint8 fillmodeTile;      //$5106
   uint8 fillmodeColor;     //$5107

   uint1 ramSelect;                   //$5113
   uint2 ramBank;                     //$5113
   uint8 programBank[4];              //$5114-5117
  uint10 characterSpriteBank[8];      //$5120-5127
  uint10 characterBackgroundBank[4];  //$5128-512b
   uint2 characterBankHi;             //$5130

   uint1 vsplitEnable;  //$5200
   uint1 vsplitSide;    //$5200
   uint5 vsplitTile;    //$5200
   uint8 vsplitScroll;  //$5201
   uint8 vsplitBank;    //$5202

   uint8 irqCoincidence;  //$5203
   uint1 irqEnable;       //$5204

   uint8 multiplicand;  //$5205
   uint8 multiplier;    //$5206

  uint16 timerCounter;  //$5209-520a
   uint1 timerLine;

  //status registers

   uint8 cycleCounter;
   uint1 irqLine;
   uint1 inFrame;

  uint16 vcounter;
  uint16 hcounter;
  uint16 characterAccess[4];
   uint1 characterActive;
   uint1 sprite8x16;

   uint8 exbank;
   uint8 exattr;

   uint1 vsplitFetch;
   uint8 vsplitVoffset;
   uint8 vsplitHoffset;
};
