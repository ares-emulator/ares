struct ROM {
  Memory::Readable<uint8> bios;
  Memory::Readable<uint8> sub;
};

struct System {
  Node::System node;
  Node::Setting::String regionNode;

  enum class Model : uint { MSX, MSX2 };
  enum class Region : uint { NTSC, PAL };

  auto name() const -> string { return node->name(); }
  auto model() const -> Model { return information.model; }
  auto region() const -> Region { return information.region; }
  auto colorburst() const -> double { return information.colorburst; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;

  auto power(bool reset = false) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Model model = Model::MSX;
    Region region = Region::NTSC;
    double colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern ROM rom;
extern System system;

auto Model::MSX() -> bool { return system.model() == System::Model::MSX; }
auto Model::MSX2() -> bool { return system.model() == System::Model::MSX2; }

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
