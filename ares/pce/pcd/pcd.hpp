//PC Engine CD-ROM

struct PCD : Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral disc;
  Shared::File fd;
  Memory::Writable<uint8> wram;  //work RAM (64KB)
  Memory::Writable<uint8> bram;  //backup RAM (2KB)

  static auto Present() -> bool { return true; }

  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }

  //pcd.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto read(uint4 address) -> uint8;
  auto write(uint4 address, uint8 data) -> void;

  auto readWRAM(uint16 address) -> uint8;
  auto writeWRAM(uint16 address, uint8 data) -> void;

  auto readBRAM(uint11 address) -> uint8;
  auto writeBRAM(uint11 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
  } information;
};

extern PCD pcd;
