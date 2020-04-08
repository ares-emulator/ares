//internal = Konami Q-Tai base cartridge (connects to Famicom system)
//external = Konami Q-Tai game cartridge (connects to base cartridge)
//PRG-ROM: 128 KiB internal ROM + 512 KiB external ROM (concatenated)
//CHR-ROM: 256 KiB internal ROM
//PRG-RAM: 8 KiB volatile internal + 8 KiB non-volatile external
//CHR-RAM: 8 KiB volatile internal
//QTRAM:   2 KiB volatile internal

struct KonamiVRC5 : Interface {
  Memory::Readable<uint8> programROM;
  Memory::Writable<uint8> programRAM;
  Memory::Readable<uint8> characterROM;
  Memory::Writable<uint8> characterRAM;
  Memory::Writable<uint8> qtram;

  using Interface::Interface;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(programROM, board["memory(type=ROM,content=Program)"]);
    Interface::load(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::load(characterROM, board["memory(type=ROM,content=Character)"]);
    Interface::load(characterRAM, board["memory(type=RAM,content=Character)"]);
    qtram.allocate(2_KiB);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(programRAM, board["memory(type=RAM,content=Save)"]);
    Interface::save(characterRAM, board["memory(type=RAM,content=Character)"]);
  }

  auto main() -> void override {
    if(irqEnable) {
      if(++irqCounter == 0) {
        irqCounter = irqLatch;
        cpu.irqLine(1);
      }
    }
    tick();
  }

  //converts JIS X 0208 codepoint to CIRAM tile# + QTRAM bank#
  auto jisLookup() const -> uint16 {
    static const uint8 table[4][8] = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x40, 0x10, 0x28, 0x00, 0x18, 0x30},
      {0x00, 0x00, 0x48, 0x18, 0x30, 0x08, 0x20, 0x38},
      {0x00, 0x00, 0x80, 0x20, 0x38, 0x10, 0x28, 0xb0},
    };
    uint8 data = table[jisColumn >> 5][jisRow >> 4];
    uint8 lo = jisPosition | (jisColumn & 31) << 2 | (jisRow & 1) << 7;
    uint8 hi = jisRow >> 1 & 7 | data & 0x3f | 0x40 | jisAttribute << 7;
    if(data & 0x40) {
      hi &= 0xfb;
    } else if(data & 0x80) {
      hi |= 0x04;
    }
    return hi << 8 | lo << 0;
  }

  auto readPRG(uint address) -> uint8 {
    uint8 data;

    //replaces program bank 2 region
    if(address >= 0xdc00 && address <= 0xdcff) {
      data = jisLookup() >> 0;
      return data;
    }

    //replaces program bank 2 region
    if(address >= 0xdd00 && address <= 0xddff) {
      data = jisLookup() >> 8;
      return data;
    }

    if(address >= 0x6000 && address <= 0x6fff) {
      uint chip = !saveChip0 ? 0x2000 : 0x0000;
      data = programRAM.read(chip + (saveBank0 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x7000 && address <= 0x7fff) {
      uint chip = !saveChip1 ? 0x2000 : 0x0000;
      data = programRAM.read(chip + (saveBank1 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x8000 && address <= 0x9fff) {
      uint chip = !programChip0 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank0 << 13) + (uint13)address);
      return data;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      uint chip = !programChip1 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank1 << 13) + (uint13)address);
      return data;
    }

    if(address >= 0xc000 && address <= 0xdfff) {
      uint chip = !programChip2 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank2 << 13) + (uint13)address);
      return data;
    }

    if(address >= 0xe000 && address <= 0xffff) {
      uint chip = !programChip3 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank3 << 13) + (uint13)address);
      return data;
    }

    return data;
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address >= 0x6000 && address <= 0x6fff) {
      uint chip = !saveChip0 ? 0x2000 : 0x0000;
      programRAM.write(chip + (saveBank0 << 12) + (uint12)address, data);
      return;
    }

    if(address >= 0x7000 && address <= 0x7fff) {
      uint chip = !saveChip1 ? 0x2000 : 0x0000;
      programRAM.write(chip + (saveBank1 << 12) + (uint12)address, data);
      return;
    }

    address &= 0xff00;

    if(address == 0xd000) {
      saveBank0 = data.bit(0);
      saveChip0 = data.bit(3);
      return;
    }

    if(address == 0xd100) {
      saveBank1 = data.bit(0);
      saveChip1 = data.bit(3);
      return;
    }

    if(address == 0xd200) {
      programBank0 = data.bit(0,5);
      programChip0 = data.bit(6);
      if(!programChip0) programBank0.bit(4,5) = 0;
      return;
    }

    if(address == 0xd300) {
      programBank1 = data.bit(0,5);
      programChip1 = data.bit(6);
      if(!programChip1) programBank1.bit(4,5) = 0;
      return;
    }

    if(address == 0xd400) {
      programBank2 = data.bit(0,5);
      programChip2 = data.bit(6);
      if(!programChip2) programBank2.bit(4,5) = 0;
      return;
    }

    if(address == 0xd500) {
      graphicBank0 = data.bit(0);
      return;
    }

    if(address == 0xd600) {
      irqLatch.byte(0) = data;
      return;
    }

    if(address == 0xd700) {
      irqLatch.byte(1) = data;
      return;
    }

    if(address == 0xd800) {
      irqEnable = irqRepeat;
      cpu.irqLine(0);
      return;
    }

    if(address == 0xd900) {
      irqRepeat  = data.bit(0);
      irqEnable  = data.bit(1);
      irqCounter = irqLatch;
      cpu.irqLine(0);
      return;
    }

    if(address == 0xda00) {
      qtramEnable = data.bit(0);
      mirror      = data.bit(1);
      return;
    }

    if(address == 0xdb00) {
      jisPosition  = data.bit(0,1);
      jisAttribute = data.bit(2);
      return;
    }

    if(address == 0xdc00) {
      jisColumn = data.bit(0,6);
      return;
    }

    if(address == 0xdd00) {
      jisRow = data.bit(0,6);
      return;
    }
  }

  auto readCHR(uint address) -> uint8 {
    uint8 data;

    if(address >= 0x0000 && address <= 0x1fff && chrLatchCount) {
      //A12 is ignored here
      chrLatchCount--;
      data = chrLatchData;
      if(data.bit(7) && data.bit(6) && (address & 1 << 3)) {
        data = 0xff;
      } else if(data.bit(6)) {
        uint6 bank = data.bit(0,5);
        data = characterROM.read((bank << 12) + (uint12)address);
      } else {
        uint6 bank = data.bit(0);
        data = characterRAM.read((bank << 12) + (uint12)address);
      }
      return data;
    }

    if(address >= 0x0000 && address <= 0x0fff) {
      data = characterRAM.read((graphicBank0 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      data = characterRAM.read((graphicBank1 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x2000 && address <= 0x2fff) {
      address = address >> mirror & 0x400 | address & 0x3ff;
      uint8 ciram = ppu.readCIRAM(address);
      uint8 qtram = this->qtram[address];

      //hack: how does the VRC5 determine nametable entries are being fetched?
      uint x = ppu.io.lx;
      if((x >= 1 && x <= 256) || (x >= 321 && x <= 336)) {
        uint step = x - 1 & 7;
        if(step == 0) {
          //nametable fetch: specialize the next two tiledata CHR fetches
          chrLatchData  = qtram;
          chrLatchCount = 2;
        }
      }

      data = !qtramEnable ? ciram : qtram;
      return data;
    }

    return data;  //should never occur
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address >= 0x0000 && address <= 0x0fff) {
      characterRAM.write((graphicBank0 << 12) + (uint12)address, data);
      return;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      characterRAM.write((graphicBank1 << 12) + (uint12)address, data);
      return;
    }

    if(address >= 0x2000 && address <= 0x2fff) {
      address = address >> mirror & 0x400 | address & 0x3ff;
      if(!qtramEnable) {
        ppu.writeCIRAM(address, data);
      } else {
        qtram[address] = data;
      }
      return;
    }
  }

  auto power() -> void {
    for(auto& byte : qtram) byte = 0x00;
    saveBank0 = 0;
    saveBank1 = 0;
    saveChip0 = 0;
    saveChip1 = 0;
    programBank0 = 0;
    programBank1 = 0;
    programBank2 = 0;
    programBank3 = 0x3f;
    programChip0 = 0;
    programChip1 = 0;
    programChip2 = 0;
    programChip3 = 1;
    graphicBank0 = 0;
    graphicBank1 = 1;
    irqCounter = 0;
    irqLatch = 0;
    irqRepeat = 0;
    irqEnable = 0;
    qtramEnable = 0;
    mirror = 0;
    jisPosition = 0;
    jisAttribute = 0;
    jisColumn = 0;
    jisRow = 0;
    chrLatchData = 0;
    chrLatchCount = 0;
  }

  auto serialize(serializer& s) -> void {
    programRAM.serialize(s);
    characterRAM.serialize(s);
    qtram.serialize(s);
    s.integer(saveBank0);
    s.integer(saveBank1);
    s.integer(saveChip0);
    s.integer(saveChip1);
    s.integer(programBank0);
    s.integer(programBank1);
    s.integer(programBank2);
    s.integer(programBank3);
    s.integer(programChip0);
    s.integer(programChip1);
    s.integer(programChip2);
    s.integer(programChip3);
    s.integer(graphicBank0);
    s.integer(graphicBank1);
    s.integer(irqCounter);
    s.integer(irqLatch);
    s.integer(irqRepeat);
    s.integer(irqEnable);
    s.integer(qtramEnable);
    s.integer(mirror);
    s.integer(jisPosition);
    s.integer(jisAttribute);
    s.integer(jisColumn);
    s.integer(jisRow);
    s.integer(chrLatchData);
    s.integer(chrLatchCount);
  }

  uint1 saveBank0;
  uint1 saveBank1;

  uint1 saveChip0;  //0 = external, 1 = internal
  uint1 saveChip1;

  uint6 programBank0;
  uint6 programBank1;
  uint6 programBank2;
  uint6 programBank3;  //fixed

  uint1 programChip0;  //0 = internal, 1 = external
  uint1 programChip1;
  uint1 programChip2;
  uint1 programChip3;  //fixed

  uint1 graphicBank0;
  uint1 graphicBank1;  //fixed

  uint16 irqCounter;
  uint16 irqLatch;
   uint1 irqRepeat;
   uint1 irqEnable;

   uint1 qtramEnable;  //0 = CIRAM, 1 = QTRAM
   uint1 mirror;       //0 = vertical, 1 = horizontal

   uint2 jisPosition;   //0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
   uint1 jisAttribute;  //0 = normal, 1 = alternate
   uint7 jisColumn;
   uint7 jisRow;

   uint8 chrLatchData;
   uint8 chrLatchCount;
};
