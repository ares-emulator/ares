struct ArcadeCardDuo : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> dram;

  struct Debugger {
    maybe<ArcadeCardDuo&> super;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory dram;
    } memory;
  } debugger;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(dram, "dynamic.ram");
    debugger.super = *this;
    debugger.load(cartridge.node);
  }

  auto save() -> void override {
  }

  auto unload() -> void override {
    debugger.unload(cartridge.node);
  }

  auto read(n8 bank, n13 address, n8 data) -> n8 override {
    if(bank >= 0x00 && bank <= 0x3f) {
      return rom.read(bank << 13 | address);
    }

    if(bank >= 0x40 && bank <= 0x43) {
      auto& page = pages[bank - 0x40];
      return dram.read(page.address());
    }

    if(bank == 0xff) {
      switch(address) {
      case 0x1ae0: return alu.value.byte(0);
      case 0x1ae1: return alu.value.byte(1);
      case 0x1ae2: return alu.value.byte(2);
      case 0x1ae3: return alu.value.byte(3);
      case 0x1ae4: return alu.shift;
      case 0x1ae5: return alu.rotate;
      case 0x1afd: return 0x00;  //version# (low)
      case 0x1afe: return 0x10;  //version# (high)
      case 0x1aff: return 0x51;  //identifier
      }

      auto& page = pages[address.bit(4,5)];
      switch(address & 0x1f8f) {
      case 0x1a00: return dram.read(page.address());
      case 0x1a01: return dram.read(page.address());
      case 0x1a02: return page.base.byte(0);
      case 0x1a03: return page.base.byte(1);
      case 0x1a04: return page.base.byte(2);
      case 0x1a05: return page.offset.byte(0);
      case 0x1a06: return page.offset.byte(1);
      case 0x1a07: return page.adjust.byte(0);
      case 0x1a08: return page.adjust.byte(1);
      case 0x1a09: return page.control;
      }
    }

    return data;
  }

  auto write(n8 bank, n13 address, n8 data) -> void override {
    if(bank >= 0x40 && bank <= 0x43) {
      auto& page = pages[bank - 0x40];
      return dram.write(page.address(), data);
    }

    if(bank == 0xff) {
      switch(address) {
      case 0x1ae0: alu.value.byte(0) = data; return;
      case 0x1ae1: alu.value.byte(1) = data; return;
      case 0x1ae2: alu.value.byte(2) = data; return;
      case 0x1ae3: alu.value.byte(3) = data; return;
      case 0x1ae4: alu.shift = data.bit(0,3);
        if(alu.shift.bit(3) == 0) alu.value = alu.value << alu.shift;
        if(alu.shift.bit(3) == 1) alu.value = alu.value >> 16 - alu.shift;
        return;
      case 0x1ae5: alu.rotate = data.bit(0,3);
        if(alu.rotate.bit(3) == 0) alu.value = alu.value << alu.rotate | alu.value >> 32 - alu.rotate;
        if(alu.rotate.bit(3) == 1) alu.value = alu.value >> 32 - (16 - alu.rotate) | alu.value << 16 - alu.rotate;
        return;
      }

      auto& page = pages[address.bit(4,5)];
      switch(address & 0x1f8f) {
      case 0x1a00: return dram.write(page.address(), data);
      case 0x1a01: return dram.write(page.address(), data);
      case 0x1a02: page.base.byte(0) = data; return;
      case 0x1a03: page.base.byte(1) = data; return;
      case 0x1a04: page.base.byte(2) = data; return;
      case 0x1a05: page.offset.byte(0) = data;
        if(page.control.bit(5,6) != 1) return;
        page.base += page.offset + 0xff0000 * page.control.bit(3);
        return;
      case 0x1a06: page.offset.byte(1) = data;
        if(page.control.bit(5,6) != 2) return;
        page.base += page.offset + 0xff0000 * page.control.bit(3);
        return;
      case 0x1a07: page.adjust.byte(0) = data; return;
      case 0x1a08: page.adjust.byte(1) = data; return;
      case 0x1a09: page.control = data.bit(0,6); return;
      case 0x1a0a:
        if(page.control.bit(5,6) != 3) return;
        page.base += page.offset + 0xff0000 + page.control.bit(3);
        return;
      }
    }
  }

  auto power() -> void override {
    for(auto& page : pages) page = {};
    alu = {};
  }

  auto serialize(serializer& s) -> void override {
    s(dram);

    for(auto& page : pages) {
      s(page.control);
      s(page.base);
      s(page.offset);
      s(page.adjust);
    }

    s(alu.value);
    s(alu.shift);
    s(alu.rotate);
  }

  struct Page {
    auto address() -> n21 {
      n21 address = base;
      if(control.bit(1) == 1) address += offset + 0xff0000 * control.bit(3);
      if(control.bit(0) == 1) increment();
      return address;
    }

    auto increment() -> void {
      if(control.bit(4) == 0) offset += adjust;
      if(control.bit(4) == 1) base += adjust;
    }

    n7  control;
    n24 base;
    n16 offset;
    n16 adjust;
  } pages[4];

  struct ALU {
    n32 value;
    n4  shift;
    n4  rotate;
  } alu;
};
