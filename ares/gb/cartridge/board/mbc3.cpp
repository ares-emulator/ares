struct MBC3 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  Memory::Writable<n8> rtc;
  bool MBC30 = false;  //0 = MBC3, 1 = MBC30
  //MBC3  supports 128 ROM banks (2MB) and 4 RAM banks (32KB)
  //MBC30 supports 256 ROM banks (4MB) and 8 RAM banks (64KB)

  auto load() -> void override {
    MBC30 = pak->attribute("board") == "MBC30";
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
    Interface::load(rtc, "time.rtc");

    if(rtc.size() == 13) {
      io.rtc.second       = rtc[0];
      io.rtc.minute       = rtc[1];
      io.rtc.hour         = rtc[2];
      io.rtc.day.bit(0,7) = rtc[3];
      io.rtc.day.bit(8)   = rtc[4].bit(0);
      io.rtc.halt         = rtc[4].bit(6);
      io.rtc.dayCarry     = rtc[4].bit(7);

      n64 timestamp = 0;
      for(u32 index : range(8)) {
        timestamp.byte(index) = rtc[5 + index];
      }
      n64 diff = chrono::timestamp() - timestamp;
      if(diff < 32 * 365 * 24 * 60 * 60) {
        while(diff >= 24 * 60 * 60) { tickDay(); diff -= 24 * 60 * 60; }
        while(diff >= 60 * 60) { tickHour(); diff -= 60 * 60; }
        while(diff >= 60) { tickMinute(); diff -= 60; }
        while(diff) { tickSecond(); diff -= 1; }
      }
    }
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
    Interface::save(rtc, "time.rtc");

    if(rtc.size() == 13) {
      rtc[0] = io.rtc.second;
      rtc[1] = io.rtc.minute;
      rtc[2] = io.rtc.hour;
      rtc[3] = io.rtc.day.bit(0,7);
      rtc[4] = io.rtc.day.bit(8) << 0 | io.rtc.halt << 6 | io.rtc.dayCarry << 7;

      n64 timestamp = chrono::timestamp();
      for(u32 index : range(8)) {
        rtc[5 + index] = timestamp.byte(index);
      }
    }
  }

  auto unload() -> void override {
  }

  auto main() -> void override {
    step(cartridge.frequency());
    if(!io.rtc.halt) tickSecond();
  }

  auto tickSecond() -> void {
    if(++io.rtc.second >= 60) {
      io.rtc.second = 0;
      tickMinute();
    }
  }

  auto tickMinute() -> void {
    if(++io.rtc.minute >= 60) {
      io.rtc.minute = 0;
      tickHour();
    }
  }

  auto tickHour() -> void {
    if(++io.rtc.hour >= 24) {
      io.rtc.hour = 0;
      tickDay();
    }
  }

  auto tickDay() -> void {
    if(++io.rtc.day == 0) {
      io.rtc.dayCarry = true;
    }
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((n14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return rom.read(io.rom.bank << 14 | (n14)address);
    }

    if(address >= 0xa000 && address <= 0xbfff) {
      //RAM disable affects RTC registers as well
      if(!io.ram.enable) return 0xff;
      if(io.ram.bank <= (!MBC30 ? 0x03 : 0x07)) {
        if(!ram) return 0xff;
        return ram.read(io.ram.bank << 13 | (n13)address);
      }
      if(io.ram.bank == 0x08) return io.rtc.latchSecond;
      if(io.ram.bank == 0x09) return io.rtc.latchMinute;
      if(io.ram.bank == 0x0a) return io.rtc.latchHour;
      if(io.ram.bank == 0x0b) return io.rtc.latchDay.bit(0,7);
      if(io.ram.bank == 0x0c) return io.rtc.latchDay.bit(8) << 0 | io.rtc.latchHalt << 6 | io.rtc.latchDayCarry << 7;
      return 0xff;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      io.ram.enable = data.bit(0,3) == 0x0a;
      return;
    }

    if(address >= 0x2000 && address <= 0x3fff) {
      if(MBC30 == 0) io.rom.bank = data.bit(0,6);
      if(MBC30 == 1) io.rom.bank = data.bit(0,7);
      if(!io.rom.bank) io.rom.bank = 0x01;
      return;
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      io.ram.bank = data;
      return;
    }

    if(address >= 0x6000 && address <= 0x7fff) {
      if(io.rtc.latch == 0 && data == 1) {
        io.rtc.latchSecond = io.rtc.second;
        io.rtc.latchMinute = io.rtc.minute;
        io.rtc.latchHour = io.rtc.hour;
        io.rtc.latchDay = io.rtc.day;
        io.rtc.latchHalt = io.rtc.latch;
        io.rtc.latchDayCarry = io.rtc.dayCarry;
      }
      io.rtc.latch = data;
      return;
    }

    if((address & 0xe000) == 0xa000) {  //$a000-bfff
      //RAM disable affects RTC registers as well
      if(!io.ram.enable) return;
      if(io.ram.bank <= (!MBC30 ? 0x03 : 0x07)) {
        if(!ram) return;
        ram.write(io.ram.bank << 13 | (n13)address, data);
      } else if(io.ram.bank == 0x08) {
        if(data >= 60) data = 0;  //unverified
        io.rtc.second = data;
      } else if(io.ram.bank == 0x09) {
        if(data >= 60) data = 0;  //unverified
        io.rtc.minute = data;
      } else if(io.ram.bank == 0x0a) {
        if(data >= 24) data = 0;  //unverified
        io.rtc.hour = data;
      } else if(io.ram.bank == 0x0b) {
        io.rtc.day.bit(0,7) = data.bit(0,7);
      } else if(io.ram.bank == 0x0c) {
        io.rtc.day.bit(8) = data.bit(0);
        io.rtc.halt = data.bit(6);
        io.rtc.dayCarry = data.bit(7);
      }
      return;
    }
  }

  auto power() -> void override {
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(io.rom.bank);
    s(io.ram.enable);
    s(io.ram.bank);
    s(io.rtc.second);
    s(io.rtc.minute);
    s(io.rtc.hour);
    s(io.rtc.day);
    s(io.rtc.halt);
    s(io.rtc.dayCarry);
    s(io.rtc.latch);
    s(io.rtc.latchSecond);
    s(io.rtc.latchMinute);
    s(io.rtc.latchHour);
    s(io.rtc.latchDay);
    s(io.rtc.latchHalt);
    s(io.rtc.latchDayCarry);
  }

  struct IO {
    struct ROM {
      n8 bank = 0x01;
    } rom;
    struct RAM {
      n1 enable;
      n8 bank;
    } ram;
    struct RTC {
      n8 second;
      n8 minute;
      n8 hour;
      n9 day;
      n1 halt;
      n1 dayCarry;

      n1 latch;
      n8 latchSecond;
      n8 latchMinute;
      n8 latchHour;
      n9 latchDay;
      n1 latchHalt;
      n1 latchDayCarry;
    } rtc;
  } io;
};
