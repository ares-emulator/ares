struct Banked : Interface {
  using Interface::Interface;
  Memory::Readable<n16> rom;

  auto load(Markup::Node document) -> void override {
    auto board = document["game/board"];
    Interface::load(rom, board["memory(type=ROM,content=Program)"]);
  }

  auto save(Markup::Node document) -> void override {
    auto board = document["game/board"];
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    n25 offset = romBank[address >> 19] << 19 | (n19)address;
    return data = rom[offset >> 1];
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    return data;
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(!lower) return;  //todo: unconfirmed
    if(address == 0xa130f2) romBank[1] = data.bit(0,5);
    if(address == 0xa130f4) romBank[2] = data.bit(0,5);
    if(address == 0xa130f6) romBank[3] = data.bit(0,5);
    if(address == 0xa130f8) romBank[4] = data.bit(0,5);
    if(address == 0xa130fa) romBank[5] = data.bit(0,5);
    if(address == 0xa130fc) romBank[6] = data.bit(0,5);
    if(address == 0xa130fe) romBank[7] = data.bit(0,5);
  }

  auto power() -> void override {
    for(u32 index : range(8)) romBank[index] = index;
  }

  auto serialize(serializer& s) -> void override {
    s(romBank);
  }

  n6 romBank[8];
};
