//internal = Konami Q-Tai base cartridge (connects to Famicom system)
//external = Konami Q-Tai game cartridge (connects to base cartridge)
//PRG-ROM: 128 KiB internal ROM + 512 KiB external ROM (concatenated)
//CHR-ROM: 256 KiB internal ROM
//PRG-RAM: 8 KiB volatile internal + 8 KiB non-volatile external
//CHR-RAM: 8 KiB volatile internal
//QTRAM:   2 KiB volatile internal

struct KonamiVRC5 : Interface {
  static auto create(string id) -> Interface* {
    if(id == "KONAMI-VRC-5") return new KonamiVRC5;
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Writable<n8> programRAM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  Memory::Writable<n8> qtram;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(programRAM, "save.ram");
    Interface::load(characterROM, "character.rom");
    Interface::load(characterRAM, "character.ram");
    qtram.allocate(2_KiB);
  }

  auto save() -> void override {
    Interface::save(programRAM, "save.ram");
    Interface::save(characterRAM, "character.ram");
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
  auto jisLookup() const -> n16 {
    static const n8 table[4][8] = {
      {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
      {0x00, 0x00, 0x40, 0x10, 0x28, 0x00, 0x18, 0x30},
      {0x00, 0x00, 0x48, 0x18, 0x30, 0x08, 0x20, 0x38},
      {0x00, 0x00, 0x80, 0x20, 0x38, 0x10, 0x28, 0xb0},
    };
    n8 data = table[jisColumn >> 5][jisRow >> 4];
    n8 lo = jisPosition | (jisColumn & 31) << 2 | (jisRow & 1) << 7;
    n8 hi = jisRow >> 1 & 7 | data & 0x3f | 0x40 | jisAttribute << 7;
    if(data & 0x40) {
      hi &= 0xfb;
    } else if(data & 0x80) {
      hi |= 0x04;
    }
    return hi << 8 | lo << 0;
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
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
      n32 chip = !saveChip0 ? 0x2000 : 0x0000;
      data = programRAM.read(chip + (saveBank0 << 12) + (n12)address);
      return data;
    }

    if(address >= 0x7000 && address <= 0x7fff) {
      n32 chip = !saveChip1 ? 0x2000 : 0x0000;
      data = programRAM.read(chip + (saveBank1 << 12) + (n12)address);
      return data;
    }

    if(address >= 0x8000 && address <= 0x9fff) {
      n32 chip = !programChip0 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank0 << 13) + (n13)address);
      return data;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      n32 chip = !programChip1 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank1 << 13) + (n13)address);
      return data;
    }

    if(address >= 0xc000 && address <= 0xdfff) {
      n32 chip = !programChip2 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank2 << 13) + (n13)address);
      return data;
    }

    if(address >= 0xe000 && address <= 0xffff) {
      n32 chip = !programChip3 ? 0x00000 : 0x20000;
      data = programROM.read(chip + (programBank3 << 13) + (n13)address);
      return data;
    }

    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address >= 0x6000 && address <= 0x6fff) {
      n32 chip = !saveChip0 ? 0x2000 : 0x0000;
      programRAM.write(chip + (saveBank0 << 12) + (n12)address, data);
      return;
    }

    if(address >= 0x7000 && address <= 0x7fff) {
      n32 chip = !saveChip1 ? 0x2000 : 0x0000;
      programRAM.write(chip + (saveBank1 << 12) + (n12)address, data);
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

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x1fff && chrLatchCount) {
      //A12 is ignored here
      chrLatchCount--;
      data = chrLatchData;
      if(data.bit(7) && data.bit(6) && (address & 1 << 3)) {
        data = 0xff;
      } else if(data.bit(6)) {
        n6 bank = data.bit(0,5);
        data = characterROM.read((bank << 12) + (n12)address);
      } else {
        n6 bank = data.bit(0);
        data = characterRAM.read((bank << 12) + (n12)address);
      }
      return data;
    }

    if(address >= 0x0000 && address <= 0x0fff) {
      data = characterRAM.read((graphicBank0 << 12) + (n12)address);
      return data;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      data = characterRAM.read((graphicBank1 << 12) + (n12)address);
      return data;
    }

    if(address >= 0x2000 && address <= 0x2fff) {
      address = address >> mirror & 0x400 | address & 0x3ff;
      n8 ciram = ppu.readCIRAM(address);
      n8 qtram = this->qtram[address];

      //hack: how does the VRC5 determine nametable entries are being fetched?
      u32 x = ppu.io.lx;
      if((x >= 1 && x <= 256) || (x >= 321 && x <= 336)) {
        u32 step = x - 1 & 7;
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

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x0fff) {
      characterRAM.write((graphicBank0 << 12) + (n12)address, data);
      return;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      characterRAM.write((graphicBank1 << 12) + (n12)address, data);
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

  auto power() -> void override {
    programBank3 = 0x3f;
    programChip3 = 1;
    graphicBank1 = 1;
  }

  auto serialize(serializer& s) -> void override {
    s(programRAM);
    s(characterRAM);
    s(qtram);
    s(saveBank0);
    s(saveBank1);
    s(saveChip0);
    s(saveChip1);
    s(programBank0);
    s(programBank1);
    s(programBank2);
    s(programBank3);
    s(programChip0);
    s(programChip1);
    s(programChip2);
    s(programChip3);
    s(graphicBank0);
    s(graphicBank1);
    s(irqCounter);
    s(irqLatch);
    s(irqRepeat);
    s(irqEnable);
    s(qtramEnable);
    s(mirror);
    s(jisPosition);
    s(jisAttribute);
    s(jisColumn);
    s(jisRow);
    s(chrLatchData);
    s(chrLatchCount);
  }

  n1  saveBank0;
  n1  saveBank1;

  n1  saveChip0;  //0 = external, 1 = internal
  n1  saveChip1;

  n6  programBank0;
  n6  programBank1;
  n6  programBank2;
  n6  programBank3;  //fixed

  n1  programChip0;  //0 = internal, 1 = external
  n1  programChip1;
  n1  programChip2;
  n1  programChip3;  //fixed

  n1  graphicBank0;
  n1  graphicBank1;  //fixed

  n16 irqCounter;
  n16 irqLatch;
  n1  irqRepeat;
  n1  irqEnable;

  n1  qtramEnable;  //0 = CIRAM, 1 = QTRAM
  n1  mirror;       //0 = vertical, 1 = horizontal

  n2  jisPosition;   //0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
  n1  jisAttribute;  //0 = normal, 1 = alternate
  n7  jisColumn;
  n7  jisRow;

  n8  chrLatchData;
  n8  chrLatchCount;
};
