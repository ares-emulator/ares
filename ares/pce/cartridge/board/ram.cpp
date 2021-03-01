struct RAM : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;

  struct Debugger {
    maybe<RAM&> super;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;
  } debugger;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(ram, "save.ram");
    debugger.super = *this;
    debugger.load(cartridge.node);
  }

  auto save() -> void override {
    Interface::save(ram, "save.ram");
  }

  auto unload() -> void override {
    debugger.unload(cartridge.node);
  }

  auto read(n8 bank, n13 address, n8 data) -> n8 override {
    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(bank << 13 | address);
    }

    if(bank >= 0x40 && bank <= 0x43) {
      return ram.read(bank << 13 | address);
    }

    return data;
  }

  auto write(n8 bank, n13 address, n8 data) -> void override {
    if(bank >= 0x40 && bank <= 0x43) {
      return ram.write(bank << 13 | address, data);
    }
  }

  auto power() -> void override {
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
  }
};
