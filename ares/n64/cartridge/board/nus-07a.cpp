struct NUS_07A : Interface {
  using Interface::Interface;
  Memory::Readable16 rom;

#include "flash.hpp"

#include "rtc.hpp"

  struct Debugger {
    NUS_07A& self;
    Debugger(NUS_07A& self) : self(self) {}

    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory flash;
    } memory;
  } debugger{*this};

  auto load(Node::Object parent) -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(flash, "save.flash");
    rtc.load();

    debugger.load(parent);
  }

  auto unload(Node::Object parent) -> void override {
    debugger.unload(parent);

    rom.reset();
    flash.reset();
  }

  auto save() -> void override {
    Interface::save(flash, "save.flash");
    rtc.save();
  }

  auto readBus(u32 address) -> u16 override {
    const u16 unmapped = address & 0xFFFF;

    if(address >= 0x0800'0000 && address <= 0x0fff'ffff) {
      return flash.read<Half>(address);
    }

    if(address >= 0x1000'0000 && address <= 0x1000'0000 + rom.size - 1) {
      return rom.read<Half>(address);
    }

    return unmapped;
  }

  auto writeBus(u32 address, u16 data) -> void override {
    if(address >= 0x0800'0000 && address <= 0x0fff'ffff) {
      return flash.write<Half>(address, data);
    }

    if(address >= 0x1000'0000 && address <= 0x1000'0000 + rom.size - 1) {
      return rom.write<Half>(address, data);
    }

    return;
  }

  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override {
    n1 valid = 0, over = 0;

    valid = rtc.joybusComm(send, recv, input, output);

    n2 status;
    status.bit(0) = valid;
    status.bit(1) = over;
    return status;
  }

  auto tickRTC() -> void override {
    rtc.tick();
  }

  auto power(bool reset) -> void override {
    flash.mode = Flash::Mode::Idle;
    flash.status = 0;
    flash.source = 0;
    flash.offset = 0;

    rtc.power(reset);
  }

  auto serialize(serializer& s) -> void override {
    s(flash);
    s(rtc);
  }
};

auto NUS_07A::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(self.rom.size);
  memory.rom->setRead([&](u32 address) -> u8 {
    return self.rom.read<Byte>(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return self.rom.write<Byte>(address, data);
  });

  memory.flash = parent->append<Node::Debugger::Memory>("Cartridge Flash");
  memory.flash->setSize(self.flash.size);
  memory.flash->setRead([&](u32 address) -> u8 {
    return self.flash.read<Byte>(address);
  });
  memory.flash->setWrite([&](u32 address, u8 data) -> void {
    return self.flash.write<Byte>(address, data);
  });
}

auto NUS_07A::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.rom);
  parent->remove(memory.flash);
  memory.rom.reset();
  memory.flash.reset();
}

#define BOARD NUS_07A

#include "flash.cpp"

#include "rtc.cpp"

#undef BOARD
