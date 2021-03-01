//U1: TAMA7: Mask ROM (512KB)
//U2: TAMA5: Game Boy cartridge connector interface
//U3: TAMA6: Toshiba TMP47C243M (4-bit MCU)
//U4: RTC: Toshiba TC8521AM

//note: the TMP47C243M's 2048 x 8-bit program ROM is currently undumped
//as such, high level emulation is used as a necessary evil

struct TAMA : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  Memory::Writable<n8> rtc;

  auto toBCD  (n8 data) -> n8 { return (data / 10) * 16 + (data % 10); }
  auto fromBCD(n8 data) -> n8 { return (data / 16) * 10 + (data % 16); }

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
    Interface::load(rtc, "time.rtc");

    if(rtc.size() == 15) {
      io.rtc.year     = fromBCD(rtc[0]);
      io.rtc.month    = fromBCD(rtc[1]);
      io.rtc.day      = fromBCD(rtc[2]);
      io.rtc.hour     = fromBCD(rtc[3]);
      io.rtc.minute   = fromBCD(rtc[4]);
      io.rtc.second   = fromBCD(rtc[5]);
      io.rtc.meridian = rtc[6].bit(0);
      io.rtc.leapYear = rtc[6].bit(1,2);
      io.rtc.hourMode = rtc[6].bit(3);
      io.rtc.test     = rtc[6].bit(4,7);

      n64 timestamp = 0;
      for(u32 index : range(8)) {
        timestamp.byte(index) = rtc[7 + index];
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

    if(rtc.size() == 15) {
      rtc[0] = toBCD(io.rtc.year);
      rtc[1] = toBCD(io.rtc.month);
      rtc[2] = toBCD(io.rtc.day);
      rtc[3] = toBCD(io.rtc.hour);
      rtc[4] = toBCD(io.rtc.minute);
      rtc[5] = toBCD(io.rtc.second);
      rtc[6] = io.rtc.meridian << 0 | io.rtc.leapYear << 1 | io.rtc.hourMode << 3 | io.rtc.test << 4;

      n64 timestamp = chrono::timestamp();
      for(u32 index : range(8)) {
        rtc[7 + index] = timestamp.byte(index);
      }
    }
  }

  auto unload() -> void override {
  }

  auto main() -> void override {
    tickSecond();
    step(cartridge.frequency());
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((n14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return rom.read(io.rom.bank << 14 | (n14)address);
    }

    if(address >= 0xa000 && address <= 0xbfff && (address & 1) == 0) {
      if(io.select == 0x0a) {
        return 0xf0 | io.ready;
      }

      if(io.mode == 0 || io.mode == 1) {
        if(io.select == 0x0c) {
          return 0xf0 | io.output.bit(0,3);
        }

        if(io.select == 0x0d) {
          return 0xf0 | io.output.bit(4,7);
        }
      }

      if(io.mode == 2 || io.mode == 4) {
        if(io.select == 0x0c || io.select == 0x0d) {
          n4 data;
          if(io.rtc.index == 0) data = io.rtc.minute % 10;
          if(io.rtc.index == 1) data = io.rtc.minute / 10;
          if(io.rtc.index == 2) data = io.rtc.hour % 10;
          if(io.rtc.index == 3) data = io.rtc.hour / 10;
          if(io.rtc.index == 4) data = io.rtc.day / 10;
          if(io.rtc.index == 5) data = io.rtc.day % 10;
          if(io.rtc.index == 6) data = io.rtc.month / 10;
          if(io.rtc.index == 7) data = io.rtc.month % 10;
          io.rtc.index++;
          return 0xf0 | data;
        }
      }

      return 0xff;
    }

    if(address >= 0xa000 && address <= 0xbfff && (address & 1) == 1) {
      return 0xff;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0xa000 && address <= 0xbfff && (address & 1) == 0) {
      if(io.select == 0x00) {
        io.rom.bank.bit(0,3) = data.bit(0,3);
      }

      if(io.select == 0x01) {
        io.rom.bank.bit(4) = data.bit(0);
      }

      if(io.select == 0x04) {
        io.input.bit(0,3) = data.bit(0,3);
      }

      if(io.select == 0x05) {
        io.input.bit(4,7) = data.bit(0,3);
      }

      if(io.select == 0x06) {
        io.index.bit(4) = data.bit(0);
        io.mode = data.bit(1,3);
      }

      if(io.select == 0x07) {
        io.index.bit(0,3) = data.bit(0,3);

        if(io.mode == 0) {
          if(ram) ram.write(io.index, io.input);
        }

        if(io.mode == 1) {
          if(ram) io.output = ram.read(io.index);
        }

        if(io.mode == 2 && io.index == 0x04) {
          io.rtc.minute = fromBCD(io.input);
        }

        if(io.mode == 2 && io.index == 0x05) {
          io.rtc.hour = fromBCD(io.input);
          io.rtc.meridian = io.rtc.hour >= 12;
        }

        if(io.mode == 4 && io.index == 0x00 && io.input.bit(0,3) == 0x7) {
          n8 day = toBCD(io.rtc.day);
          day.bit(0,3) = io.input.bit(4,7);
          io.rtc.day = fromBCD(day);
        }

        if(io.mode == 4 && io.index == 0x00 && io.input.bit(0,3) == 0x8) {
          n8 day = toBCD(io.rtc.day);
          day.bit(4,7) = io.input.bit(4,7);
          io.rtc.day = fromBCD(day);
        }

        if(io.mode == 4 && io.index == 0x00 && io.input.bit(0,3) == 0x9) {
          n8 month = toBCD(io.rtc.month);
          month.bit(0,3) = io.input.bit(4,7);
          io.rtc.month = fromBCD(month);
        }

        if(io.mode == 4 && io.index == 0x00 && io.input.bit(0,3) == 0xa) {
          n8 month = toBCD(io.rtc.month);
          month.bit(4,7) = io.input.bit(4,7);
          io.rtc.month = fromBCD(month);
        }

        if(io.mode == 4 && io.index == 0x00 && io.input.bit(0,3) == 0xb) {
          n8 year = toBCD(io.rtc.year);
          year.bit(0,3) = io.input.bit(4,7);
          io.rtc.year = fromBCD(year);
        }

        if(io.mode == 4 && io.index == 0x00 && io.input.bit(0,3) == 0xc) {
          n8 year = toBCD(io.rtc.year);
          year.bit(4,7) = io.input.bit(4,7);
          io.rtc.year = fromBCD(year);
        }

        if(io.mode == 4 && io.index == 0x02 && io.input.bit(0,3) == 0xa) {
          io.rtc.hourMode = io.input.bit(4);
          io.rtc.second = 0;  //hack: unclear where this is really being set (if it is at all)
        }

        if(io.mode == 4 && io.index == 0x02 && io.input.bit(0,3) == 0xb) {
          io.rtc.leapYear = data.bit(4,5);
        }

        if(io.mode == 4 && io.index == 0x02 && io.input.bit(0,3) == 0xe) {
          io.rtc.test = io.input.bit(4,7);
        }

        if(io.mode == 2 && io.index == 0x06) {
          io.rtc.index = 0;
        }
      }

      return;
    }

    if(address >= 0xa000 && address <= 0xbfff && (address & 1) == 1) {
      io.select = data.bit(0,3);

      if(io.select == 0x0a) {
        io.ready = true;
      }

      return;
    }
  }

  auto power() -> void override {
    io.ready = 0;
    io.select = 0;
    io.mode = 0;
    io.index = 0;
    io.input = 0;
    io.output = 0;
    io.rom.bank = 0;
  //io.rtc registers are initialized by load()
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(rtc);
    s(io.ready);
    s(io.select);
    s(io.mode);
    s(io.index);
    s(io.input);
    s(io.output);
    s(io.rom.bank);
    s(io.rtc.year);
    s(io.rtc.month);
    s(io.rtc.day);
    s(io.rtc.hour);
    s(io.rtc.minute);
    s(io.rtc.second);
    s(io.rtc.meridian);
    s(io.rtc.leapYear);
    s(io.rtc.hourMode);
    s(io.rtc.test);
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
      tickDay();
    }
  }

  auto tickHour() -> void {
    if(io.rtc.hourMode == 0 && ++io.rtc.hour >= 12) {
      io.rtc.hour = 0;
      io.rtc.meridian++;
    }

    if(io.rtc.hourMode == 1 && ++io.rtc.hour >= 24) {
      io.rtc.hour = 0;
      io.rtc.meridian = io.rtc.hour >= 12;
    }

    if((io.rtc.hourMode == 0 && io.rtc.hour == 0 && io.rtc.meridian == 0)
    || (io.rtc.hourMode == 1 && io.rtc.hour == 0)
    ) {
      tickDay();
    }
  }

  auto tickDay() -> void {
    u32 days[12] = {31, 28, 31, 30, 31, 30, 30, 31, 30, 31, 30, 31};
    if(io.rtc.leapYear == 0) days[1] = 29;  //extra day in February for leap years

    if(++io.rtc.day > days[(io.rtc.month - 1) % 12]) {
      io.rtc.day = 1;
      tickMonth();
    }
  }

  auto tickMonth() -> void {
    if(++io.rtc.month > 12) {
      io.rtc.month = 1;
      io.rtc.leapYear++;
      tickYear();
    }
  }

  auto tickYear() -> void {
    if(++io.rtc.year >= 100) {
      io.rtc.year = 0;
    }
  }

  struct IO {
    n1 ready;
    n4 select;
    n3 mode;
    n5 index;
    n8 input;
    n8 output;
    struct ROM {
      n5 bank;
    } rom;
    struct RTC {
      n8 year;      //0 - 99
      n8 month;     //1 - 12
      n8 day;       //1 - 31
      n8 hour;      //0 - 23
      n8 minute;    //0 - 59
      n8 second;    //0 - 59
      n1 meridian;  //0 = AM; 1 = PM
      n2 leapYear;  //0 = leap year; 1-3 = non-leap year
      n1 hourMode;  //0 = 12-hour; 1 = 24-hour
      n4 test;
      n8 index;
    } rtc;
  } io;
};
