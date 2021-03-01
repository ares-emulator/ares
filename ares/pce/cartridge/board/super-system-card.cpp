struct SuperSystemCard : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  struct Debugger {
    maybe<SuperSystemCard&> super;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;
  } debugger;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "work.ram");
    debugger.super = *this;
    debugger.load(cartridge.node);
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
    debugger.load(cartridge.node);
  }

  auto read(n8 bank, n13 address, n8 data) -> n8 override {
    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(bank << 13 | address);
    }

    if(bank >= 0x68 && bank <= 0x7f) {
      return ram.read(bank - 0x68 << 13 | address);
    }

    if(bank == 0xff) {
      switch(address) {
      case 0x18c4: return 0x00;
      case 0x18c5: return Region::NTSCJ() ? 0xaa : 0x55;
      case 0x18c6: return Region::NTSCJ() ? 0x55 : 0xaa;
      case 0x18c7: return ram.size() / 64_KiB;
      }
    }

    return data;
  }

  auto write(n8 bank, n13 address, n8 data) -> void override {
    if(bank >= 0x68 && bank <= 0x7f) {
      return ram.write(bank - 0x68 << 13 | address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
