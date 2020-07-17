struct System {
  Node::Object node;
  Node::String regionNode;
  Node::Boolean fastBoot;

  enum class Region : uint { NTSCJ, NTSCU, PAL };

  auto region() const -> Region { return information.region; }

  //system.cpp
  auto run() -> void;
  auto load(Node::Object&) -> void;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize = true) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Region region = Region::NTSCJ;
    uint32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern System system;

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
auto Region::PAL()   -> bool { return system.region() == System::Region::PAL;   }
