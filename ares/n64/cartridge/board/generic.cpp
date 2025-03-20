struct Generic : Interface {
  using Interface::Interface;
  Memory::Readable16 rom;
  Memory::Writable16 ram;
  Memory::Writable16 eeprom;

#include "flash.hpp"

#include "isviewer.hpp"

#include "rtc.hpp"

  struct Has {
    boolean RTC;
    boolean SRAM;
    boolean EEPROM;
    boolean Flash;
  } has;

  struct Debugger {
    Generic& self;
    Debugger(Generic& self) : self(self) {}
  
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory ram;
      Node::Debugger::Memory eeprom;
      Node::Debugger::Memory flash;
    } memory;
  } debugger{*this};

  auto load(Node::Object parent) -> void override {
    has = {};

    if(!Interface::load(rom, "program.rom")) {
      rom.allocate(16);
    }

    if(Interface::load(ram, "save.ram")) {
      has.SRAM = true;
    }

    if(Interface::load(eeprom, "save.eeprom")) {
      has.EEPROM = true;
    }

    if(Interface::load(flash, "save.flash")) {
      has.Flash = true;
    }

    if(rtc.load()) {
      has.RTC = true;
    }

    if(rom.size <= 0x03ff'0000) {
      isviewer.ram.allocate(64_KiB);
      isviewer.tracer = parent->append<Node::Debugger::Tracer::Notification>("ISViewer", "Cartridge");
      isviewer.tracer->setAutoLineBreak(false);
      isviewer.tracer->setTerminal(true);
    }

    debugger.load(parent);
  }

  auto unload(Node::Object parent) -> void override {
    debugger.unload(parent);

    rom.reset();
    if(has.SRAM) {
      ram.reset();
    }
    if(has.EEPROM) {
      eeprom.reset();
    }
    if(has.Flash) {
      flash.reset();
    }
    isviewer.ram.reset();
  }

  auto save() -> void override {
    if(has.SRAM) {
      Interface::save(ram, "save.ram");
    }
    if(has.EEPROM) {
      Interface::save(eeprom, "save.eeprom");
    }
    if(has.Flash) {
      Interface::save(flash, "save.flash");
    }
    if(has.RTC) {
      rtc.save();
    }
  }

  auto readBus(u32 address) -> u16 override {
    const u16 unmapped = address & 0xFFFF;

    if(address >= 0x0800'0000 && address <= 0x0fff'ffff) {
      if(has.SRAM ) return ram.read<Half>(address);
      if(has.Flash) return flash.read<Half>(address);
      return unmapped;
    }
    
    if(isviewer.enabled() && address >= 0x13ff'0000 && address <= 0x13ff'ffff) {
      return isviewer.read<Half>(address);
    }

    if(address >= 0x1000'0000 && address <= 0x1000'0000 + rom.size - 1) {
      return rom.read<Half>(address);
    }

    return unmapped;
  }

  auto writeBus(u32 address, u16 data) -> void override {
    if(address >= 0x0800'0000 && address <= 0x0fff'ffff) {
      if(has.SRAM ) return ram.write<Half>(address, data);
      if(has.Flash) return flash.write<Half>(address, data);
    }

    if(address >= 0x13ff'0000 && address <= 0x13ff'ffff) {
      if(isviewer.enabled()) {
        pi.writeForceFinish(); //Debugging channel for homebrew, be gentle
        return isviewer.write<Half>(address, data);      
      } else {
        debug(unhandled, "[PI::busWrite] attempt to write to ISViewer: ROM is too big so ISViewer is disabled");
      }
    }

    if(address >= 0x1000'0000 && address <= 0x1000'0000 + rom.size - 1) {
      return rom.write<Half>(address, data);
    }

    return;
  }

  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override {
    n1 valid = 0, over = 0;

    valid = Interface::joybusEeprom(eeprom, send, recv, input, output);

    if(has.RTC) {
      valid |= rtc.joybusComm(send, recv, input, output);
    }
  
    n2 status;
    status.bit(0) = valid;
    status.bit(1) = over;
    return status;
  }

  auto tickRTC() -> void override {
    rtc.tick();
  }

  auto power(bool reset) -> void override {
    if(has.Flash) {
      flash.mode = Flash::Mode::Idle;
      flash.status = 0;
      flash.source = 0;
      flash.offset = 0;
    }

    isviewer.ram.fill(0);

    if(has.RTC) {
      rtc.power(reset);
    }
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(eeprom);
    s(flash);
    s(rtc);
  }
};

auto Generic::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(self.rom.size);
  memory.rom->setRead([&](u32 address) -> u8 {
    return self.rom.read<Byte>(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return self.rom.write<Byte>(address, data);
  });

  if(self.has.SRAM) {
    memory.ram = parent->append<Node::Debugger::Memory>("Cartridge SRAM");
    memory.ram->setSize(self.ram.size);
    memory.ram->setRead([&](u32 address) -> u8 {
      return self.ram.read<Byte>(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return self.ram.write<Byte>(address, data);
    });
  }

  if(self.has.EEPROM) {
    memory.eeprom = parent->append<Node::Debugger::Memory>("Cartridge EEPROM");
    memory.eeprom->setSize(self.eeprom.size);
    memory.eeprom->setRead([&](u32 address) -> u8 {
      return self.eeprom.read<Byte>(address);
    });
    memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
      return self.eeprom.write<Byte>(address, data);
    });
  }

  if(self.has.Flash) {
    memory.flash = parent->append<Node::Debugger::Memory>("Cartridge Flash");
    memory.flash->setSize(self.flash.size);
    memory.flash->setRead([&](u32 address) -> u8 {
      return self.flash.read<Byte>(address);
    });
    memory.flash->setWrite([&](u32 address, u8 data) -> void {
      return self.flash.write<Byte>(address, data);
    });
  }
}

auto Generic::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.rom);
  parent->remove(memory.ram);
  parent->remove(memory.eeprom);
  parent->remove(memory.flash);
  memory.rom.reset();
  memory.ram.reset();
  memory.eeprom.reset();
  memory.flash.reset();
}

#define BOARD Generic

#include "flash.cpp"

#include "isviewer.cpp"

#include "rtc.cpp"

#undef BOARD
