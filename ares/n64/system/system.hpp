struct System {
  Node::System node;
  Node::Setting::String regionNode;

  enum class Region : uint { NTSC, PAL };

  auto name() const -> string { return node->name(); }
  auto region() const -> Region { return information.region; }
  auto frequency() const -> uint { return information.frequency; }

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
    Region region = Region::NTSC;
    uint frequency = 93'750'000;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
