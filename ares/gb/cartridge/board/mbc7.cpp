struct MBC7 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Node::Input::Axis x;
  Node::Input::Axis y;
  static constexpr s32 Center = 0x81d0;  //not 0x8000

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    x = cartridge.node->append<Node::Input::Axis>("X");
    y = cartridge.node->append<Node::Input::Axis>("Y");
    eeprom.load();
  }

  auto save() -> void override {
    cartridge.node->remove(x);
    cartridge.node->remove(y);
    eeprom.save();
  }

  auto unload() -> void override {
  }

  auto main() -> void override {
    eeprom.main();
    step(cartridge.frequency() / 1000);  //step by approximately one millisecond
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address >= 0x0000 && address <= 0x3fff) {
      return rom.read((n14)address);
    }

    if(address >= 0x4000 && address <= 0x7fff) {
      return rom.read(io.rom.bank << 14 | (n14)address);
    }

    if(address >= 0xa000 && address <= 0xafff) {
      if(!io.ram.enable[0] || !io.ram.enable[1]) return 0xff;

      switch(address.bit(4,7)) {
      case 2: return io.accelerometer.x.byte(0);
      case 3: return io.accelerometer.x.byte(1);
      case 4: return io.accelerometer.y.byte(0);
      case 5: return io.accelerometer.y.byte(1);
      case 6: return 0x00;  //z?
      case 7: return 0xff;  //z?
      case 8: return eeprom.readIO();
      }

      return 0xff;
    }

    return data;
  }

  auto write(n16 address, n8 data) -> void override {
    if(address >= 0x0000 && address <= 0x1fff) {
      io.ram.enable[0] = data.bit(0,3) == 0xa;
      if(!io.ram.enable[0]) io.ram.enable[1] = false;
      return;
    }

    if(address >= 0x2000 && address <= 0x3fff) {
      io.rom.bank = data;
      if(!io.rom.bank) io.rom.bank = 1;
      return;
    }

    if(address >= 0x4000 && address <= 0x5fff) {
      if(!io.ram.enable[0]) return;
      io.ram.enable[1] = data == 0x40;
    }

    if(address >= 0xa000 && address <= 0xafff) {
      if(!io.ram.enable[0] || !io.ram.enable[1]) return;

      switch(address.bit(4,7)) {
      case 0:
        if(data != 0x55) break;
        io.accelerometer.x = Center;
        io.accelerometer.y = Center;
        break;
      case 1:
        if(data != 0xaa) break;
        platform->input(x);
        platform->input(y);
        io.accelerometer.x = max(0x0000, min(0xffff, Center - (x->value() >> 8)));
        io.accelerometer.y = max(0x0000, min(0xffff, Center - (y->value() >> 8)));
        break;
      case 8:
        eeprom.writeIO(data);
        break;
      }

      return;
    }
  }

  auto power() -> void override {
    eeprom.power();
    io = {};
  }

  auto serialize(serializer& s) -> void override {
    s(eeprom);
    s(io.rom.bank);
    s(io.ram.enable);
    s(io.accelerometer.x);
    s(io.accelerometer.y);
  }

  //MBC7 interface to the M93LCx6 chip
  struct EEPROM : M93LCx6 {
    MBC7& self;
    EEPROM(MBC7& self) : self(self) {}

    auto load() -> void {
      if(auto fp = self.pak->read("save.eeprom")) {
        u32 size  = fp->size();
        u32 width = fp->attribute("width").natural();
        allocate(size, width, 0, 0xff);
        fp->read({data, min(fp->size(), sizeof(data))});
      }
    }

    auto save() -> void {
      if(auto fp = self.pak->write("save.eeprom")) {
        fp->write({data, size});
      }
    }

    auto power() -> void {
      M93LCx6::power();
      select = 0;
      clock = 0;
    }

    auto main() -> void {
      M93LCx6::clock();  //clocked at ~1000hz
    }

    auto readIO() -> n8 {
      n8 data;
      if(!select) {
        data.bit(0) = 1;  //high-z when the chip is idle (not selected)
      } else if(busy) {
        data.bit(0) = 0;  //low when a programming command is in progress
      } else if(output.count) {
        data.bit(0) = output.edge();  //shift register data during read commands
      } else {
        data.bit(0) = 1;  //high-z during all other commands
      }
      data.bit(1) = input.edge();
      data.bit(2) = 1;
      data.bit(3) = 1;
      data.bit(4) = 1;
      data.bit(5) = 1;
      data.bit(6) = clock;
      data.bit(7) = select;
      return data;
    }

    auto writeIO(n8 data) -> void {
      //chip enters idle state on falling CS edge
      if(select && !data.bit(7)) return power();

      //chip leaves idle state on rising CS edge
      if(!(select = data.bit(7))) return;

      //input shift register clocks on rising edge
      if(!clock.raise(data.bit(6))) return;

      //read mode
      if(output.count && !data.bit(1)) {
        if(input.start() && *input.start() == 1) {
          if(input.opcode() && *input.opcode() == 0b10) {
            output.read();
            if(output.count == 0) {
              //sequential read mode
              input.increment();
              read();
            }
          }
        }
        return;
      }
      output.flush();

      input.write(data.bit(1));
      edge();
    }

    auto serialize(serializer& s) -> void {
      M93LCx6::serialize(s);
      s(select);
      s(clock);
    }

    boolean select;  //CS
    boolean clock;   //CLK
  } eeprom{*this};

  struct IO {
    struct ROM {
      n8 bank = 0x01;
    } rom;
    struct RAM {
      n1 enable[2];
    } ram;
    struct Accelerometer {
      n16 x = Center;
      n16 y = Center;
    } accelerometer;
  } io;
};
