//internal = Konami Q-Tai base cartridge (connects to Famicom system)
//external = Konami Q-Tai game cartridge (connects to base cartridge)
//PRG-ROM: 128 KiB internal ROM + 512 KiB external ROM (concatenated)
//CHR-ROM: 256 KiB internal ROM
//PRG-RAM: 8 KiB volatile internal + 8 KiB non-volatile external
//CHR-RAM: 8 KiB volatile internal
//QTRAM:   2 KiB volatile internal

struct KonamiVRC5 : Board {
  KonamiVRC5(Markup::Node& document) : Board(document) {
  }

  auto main() -> void {
    if(io.irqEnable) {
      if(++io.irqCounter == 0) {
        io.irqCounter = io.irqLatch;
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
    uint8 data = table[io.jisColumn >> 5][io.jisRow >> 4];
    uint8 lo = io.jisPosition | (io.jisColumn & 31) << 2 | (io.jisRow & 1) << 7;
    uint8 hi = io.jisRow >> 1 & 7 | data & 0x3f | 0x40 | io.jisAttribute << 7;
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
      uint chip = !io.saveChip0 ? 0x2000 : 0x0000;
      data = prgram.read(chip + (io.saveBank0 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x7000 && address <= 0x7fff) {
      uint chip = !io.saveChip1 ? 0x2000 : 0x0000;
      data = prgram.read(chip + (io.saveBank1 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x8000 && address <= 0x9fff) {
      uint chip = !io.programChip0 ? 0x00000 : 0x20000;
      data = prgrom.read(chip + (io.programBank0 << 13) + (uint13)address);
      return data;
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      uint chip = !io.programChip1 ? 0x00000 : 0x20000;
      data = prgrom.read(chip + (io.programBank1 << 13) + (uint13)address);
      return data;
    }

    if(address >= 0xc000 && address <= 0xdfff) {
      uint chip = !io.programChip2 ? 0x00000 : 0x20000;
      data = prgrom.read(chip + (io.programBank2 << 13) + (uint13)address);
      return data;
    }

    if(address >= 0xe000 && address <= 0xffff) {
      uint chip = !io.programChip3 ? 0x00000 : 0x20000;
      data = prgrom.read(chip + (io.programBank3 << 13) + (uint13)address);
      return data;
    }

    return data;
  }

  auto writePRG(uint address, uint8 data) -> void {
    if(address >= 0x6000 && address <= 0x6fff) {
      uint chip = !io.saveChip0 ? 0x2000 : 0x0000;
      prgram.write(chip + (io.saveBank0 << 12) + (uint12)address, data);
      return;
    }

    if(address >= 0x7000 && address <= 0x7fff) {
      uint chip = !io.saveChip1 ? 0x2000 : 0x0000;
      prgram.write(chip + (io.saveBank1 << 12) + (uint12)address, data);
      return;
    }

    address &= 0xff00;

    if(address == 0xd000) {
      io.saveBank0 = data.bit(0);
      io.saveChip0 = data.bit(3);
      return;
    }

    if(address == 0xd100) {
      io.saveBank1 = data.bit(0);
      io.saveChip1 = data.bit(3);
      return;
    }

    if(address == 0xd200) {
      io.programBank0 = data.bit(0,5);
      io.programChip0 = data.bit(6);
      if(!io.programChip0) io.programBank0.bit(4,5) = 0;
      return;
    }

    if(address == 0xd300) {
      io.programBank1 = data.bit(0,5);
      io.programChip1 = data.bit(6);
      if(!io.programChip1) io.programBank1.bit(4,5) = 0;
      return;
    }

    if(address == 0xd400) {
      io.programBank2 = data.bit(0,5);
      io.programChip2 = data.bit(6);
      if(!io.programChip2) io.programBank2.bit(4,5) = 0;
      return;
    }

    if(address == 0xd500) {
      io.graphicBank0 = data.bit(0);
      return;
    }

    if(address == 0xd600) {
      io.irqLatch.byte(0) = data;
      return;
    }

    if(address == 0xd700) {
      io.irqLatch.byte(1) = data;
      return;
    }

    if(address == 0xd800) {
      io.irqEnable = io.irqRepeat;
      cpu.irqLine(0);
      return;
    }

    if(address == 0xd900) {
      io.irqRepeat  = data.bit(0);
      io.irqEnable  = data.bit(1);
      io.irqCounter = io.irqLatch;
      cpu.irqLine(0);
      return;
    }

    if(address == 0xda00) {
      io.qtramEnable = data.bit(0);
      io.mirroring   = data.bit(1);
      return;
    }

    if(address == 0xdb00) {
      io.jisPosition  = data.bit(0,1);
      io.jisAttribute = data.bit(2);
      return;
    }

    if(address == 0xdc00) {
      io.jisColumn = data.bit(0,6);
      return;
    }

    if(address == 0xdd00) {
      io.jisRow = data.bit(0,6);
      return;
    }
  }

  auto readCHR(uint address) -> uint8 {
    uint8 data;

    if(address >= 0x0000 && address <= 0x1fff && io.chrLatchCount) {
      //A12 is ignored here
      io.chrLatchCount--;
      data = io.chrLatchData;
      if(data.bit(7) && data.bit(6) && (address & 1 << 3)) {
        data = 0xff;
      } else if(data.bit(6)) {
        uint6 bank = data.bit(0,5);
        data = chrrom.read((bank << 12) + (uint12)address);
      } else {
        uint6 bank = data.bit(0);
        data = chrram.read((bank << 12) + (uint12)address);
      }
      return data;
    }

    if(address >= 0x0000 && address <= 0x0fff) {
      data = chrram.read((io.graphicBank0 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      data = chrram.read((io.graphicBank1 << 12) + (uint12)address);
      return data;
    }

    if(address >= 0x2000 && address <= 0x2fff) {
      if(io.mirroring == 0) address = address >> 0 & 0x400 | address & 0x3ff;
      if(io.mirroring == 1) address = address >> 1 & 0x400 | address & 0x3ff;
      uint8 ciram = ppu.readCIRAM(address);
      uint8 qtram = this->qtram[address];

      //hack: how does the VRC5 determine nametable entries are being fetched?
      uint x = ppu.io.lx;
      if((x >= 1 && x <= 256) || (x >= 321 && x <= 336)) {
        uint step = x - 1 & 7;
        if(step == 0) {
          //nametable fetch: specialize the next two tiledata CHR fetches
          io.chrLatchData  = qtram;
          io.chrLatchCount = 2;
        }
      }

      data = !io.qtramEnable ? ciram : qtram;
      return data;
    }

    return data;  //should never occur
  }

  auto writeCHR(uint address, uint8 data) -> void {
    if(address >= 0x0000 && address <= 0x0fff) {
      chrram.write((io.graphicBank0 << 12) + (uint12)address, data);
      return;
    }

    if(address >= 0x1000 && address <= 0x1fff) {
      chrram.write((io.graphicBank1 << 12) + (uint12)address, data);
      return;
    }

    if(address >= 0x2000 && address <= 0x2fff) {
      if(io.mirroring == 0) address = address >> 0 & 0x400 | address & 0x3ff;
      if(io.mirroring == 1) address = address >> 1 & 0x400 | address & 0x3ff;
      if(!io.qtramEnable) {
        ppu.writeCIRAM(address, data);
      } else {
        qtram[address] = data;
      }
      return;
    }
  }

  auto power() -> void {
    for(auto& byte : qtram) byte = 0x00;
    io = {};
  }

  auto serialize(serializer& s) -> void {
    Board::serialize(s);
    s.array(qtram);
    s.integer(io.saveBank0);
    s.integer(io.saveBank1);
    s.integer(io.saveChip0);
    s.integer(io.saveChip1);
    s.integer(io.programBank0);
    s.integer(io.programBank1);
    s.integer(io.programBank2);
    s.integer(io.programBank3);
    s.integer(io.programChip0);
    s.integer(io.programChip1);
    s.integer(io.programChip2);
    s.integer(io.programChip3);
    s.integer(io.graphicBank0);
    s.integer(io.graphicBank1);
    s.integer(io.irqCounter);
    s.integer(io.irqLatch);
    s.integer(io.irqRepeat);
    s.integer(io.irqEnable);
    s.integer(io.qtramEnable);
    s.integer(io.mirroring);
    s.integer(io.jisPosition);
    s.integer(io.jisAttribute);
    s.integer(io.jisColumn);
    s.integer(io.jisRow);
    s.integer(io.chrLatchData);
    s.integer(io.chrLatchCount);
  }

protected:
  uint8 qtram[0x800];

  struct IO {
    uint1 saveBank0;
    uint1 saveBank1;

    uint1 saveChip0;  //0 = external, 1 = internal
    uint1 saveChip1;

    uint6 programBank0;
    uint6 programBank1;
    uint6 programBank2;
    uint6 programBank3 = 0x3f;  //fixed

    uint1 programChip0;  //0 = internal, 1 = external
    uint1 programChip1;
    uint1 programChip2;
    uint1 programChip3 = 1;  //fixed

    uint1 graphicBank0;
    uint1 graphicBank1 = 1;  //fixed

    uint16 irqCounter = 0;
    uint16 irqLatch = 0;
     uint1 irqRepeat = 0;
     uint1 irqEnable = 0;

     uint1 qtramEnable = 0;  //0 = CIRAM, 1 = QTRAM
     uint1 mirroring = 0;    //0 = vertical, 1 = horizontal

     uint2 jisPosition;   //0 = top-left, 1 = top-right, 2 = bottom-left, 3 = bottom-right
     uint1 jisAttribute;  //0 = normal, 1 = alternate
     uint7 jisColumn;
     uint7 jisRow;

     uint8 chrLatchData;
     uint8 chrLatchCount;
  } io;
};
