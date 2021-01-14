struct System {
  Node::System node;
  Node::Setting::String regionNode;
  Node::Setting::Boolean fastBoot;

  enum class Region : uint { NTSCJ, NTSCU, PAL };

  auto name() const -> string { return node->name(); }
  auto region() const -> Region { return information.region; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;
  auto load(Node::System& node, string name) -> bool;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize = true) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Region region = Region::NTSCJ;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
auto Region::PAL()   -> bool { return system.region() == System::Region::PAL;   }
