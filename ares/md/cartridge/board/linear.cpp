struct Linear : Interface {
  using Interface::Interface;
  Memory::Readable<uint16> rom;
  Memory::Writable<uint16> wram;
  Memory::Writable< uint8> bram;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
    Interface::load(wram, board["memory(type=RAM,content=Save)"]);
    Interface::load(bram, board["memory(type=RAM,content=Save)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::save(wram, board["memory(type=RAM,content=Save)"]);
    Interface::save(bram, board["memory(type=RAM,content=Save)"]);
  }

  auto read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 override {
    if(address >= 0x200000 && ramEnable) {
      if(wram) return data = wram[address >> 1];
      if(bram) return data = bram[address >> 1] * 0x0101;  //todo: unconfirmed
    }
    return data = rom[address >> 1];
  }

  auto write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void override {
    //emulating ramWritable will break commercial software:
    //it does not appear that many (any?) games actually connect $a130f1.d1 to /WE;
    //hence RAM ends up always being writable, and many games fail to set d1=1
    if(address >= 0x200000 && ramEnable) {
      if(wram) {
        if(upper) wram[address >> 1].byte(1) = data.byte(1);
        if(lower) wram[address >> 1].byte(0) = data.byte(0);
        return;
      }
      if(bram) {
        bram[address >> 1] = data;  //todo: unconfirmed
        return;
      }
    }
  }

  auto readIO(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 override {
    return data;
  }

  auto writeIO(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void override {
    if(!lower) return;  //todo: unconfirmed
    if(address == 0xa130f0) {
      ramEnable   = data.bit(0);
      ramWritable = data.bit(1);
    }
  }

  auto power() -> void override {
    ramEnable = 1;
    ramWritable = 1;
  }

  auto serialize(serializer& s) -> void override {
    wram.serialize(s);
    bram.serialize(s);
    s.integer(ramEnable);
    s.integer(ramWritable);
  }

  uint1 ramEnable = 1;
  uint1 ramWritable = 1;
};
