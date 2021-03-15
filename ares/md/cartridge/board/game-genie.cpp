struct GameGenie : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;
  CartridgeSlot slot{"Cartridge Slot"};

  auto load() -> void override {
    Interface::load(rom, "program.rom");
    slot.load(cartridge->node);
  }

  auto save() -> void override {
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    if(enable) {
      for(auto& code : codes) {
        if(code.enable && code.address == address) return data = code.data;
      }
      if(slot.connected()) return slot.cartridge.read(upper, lower, address, data);
    }
    return data = rom[address >> 1];
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    if(enable) {
      if(slot.connected()) return slot.cartridge.write(upper, lower, address, data);
    }
    if(address == 0x02 && data == 0x0001) {
      enable = 1;
    }
    if(address >= 0x04 && address <= 0x20 && upper && lower) {  //todo: what about 8-bit writes?
      address = address - 0x04 >> 1;
      auto& code = codes[address / 3];
      if(address % 3 == 0) code.address.bit(16,23) = data.bit(0, 7);
      if(address % 3 == 1) code.address.bit( 0,15) = data.bit(0,15);
      if(address % 3 == 2) code.data = data, code.enable = 1;
    }
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    if(slot.connected()) slot.cartridge.readIO(upper, lower, address, data);
    return data;
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(slot.connected()) slot.cartridge.writeIO(upper, lower, address, data);
  }

  auto power(bool reset) -> void override {
    if(slot.connected()) slot.cartridge.power(reset);
    enable = 0;
    for(auto& code : codes) code = {};
  }

  auto serialize(serializer& s) -> void override {
    if(slot.connected()) s(slot.cartridge);
    s(enable);
    for(auto& code : codes) {
      s(code.enable);
      s(code.address);
      s(code.data);
    }
  }

  n1 enable;
  struct Code {
    n1  enable;
    n24 address;
    n16 data;
  } codes[5];
};
