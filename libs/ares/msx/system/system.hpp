struct ROM {
  Memory::Readable<n8> bios;
  Memory::Readable<n8> sub;
};

struct System {
  Node::System node;
  VFS::Pak pak;

  enum class Model : u32 { MSX, MSX2 };
  enum class Region : u32 { NTSC, PAL };

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto region() const -> Region { return information.region; }
  auto colorburst() const -> f64 { return information.colorburst; }

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
    string name = "MSX";
    Model model = Model::MSX;
    Region region = Region::NTSC;
    f64 colorburst = Constants::Colorburst::NTSC;
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
