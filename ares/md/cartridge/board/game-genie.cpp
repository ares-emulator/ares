struct GameGenie : Interface {
  using Interface::Interface;
  Memory::Readable<uint16> rom;
  CartridgeSlot slot{"Cartridge Slot"};

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);

    slot.load(cartridge->node);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
  }

  auto read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 override {
    if(enable) {
      for(auto& code : codes) {
        if(code.enable && code.address == address) return data = code.data;
      }
      if(slot.connected()) return slot.cartridge.read(upper, lower, address, data);
    }
    return data = rom[address >> 1];
  }

  auto write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void override {
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

  auto readIO(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 override {
    if(slot.connected()) slot.cartridge.readIO(upper, lower, address, data);
    return data;
  }

  auto writeIO(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void override {
    if(slot.connected()) slot.cartridge.writeIO(upper, lower, address, data);
  }

  auto power() -> void override {
    if(slot.connected()) slot.cartridge.power();
    enable = 0;
    for(auto& code : codes) code = {};
  }

  auto serialize(serializer& s) -> void {
    if(slot.connected()) slot.cartridge.serialize(s);
    s.integer(enable);
    for(auto& code : codes) {
      s.integer(code.enable);
      s.integer(code.address);
      s.integer(code.data);
    }
  }

  uint1 enable;
  struct Code {
     uint1 enable;
    uint24 address;
    uint16 data;
  } codes[5];
};
