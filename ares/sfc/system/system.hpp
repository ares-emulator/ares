struct System {
  Node::System node;
  VFS::Pak pak;

  struct Controls {
    Node::Object node;
    Node::Input::Button reset;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
  } controls;

  enum class Region : u32 { NTSC, PAL };

  auto name() const -> string { return information.name; }
  auto region() const -> Region { return information.region; }
  auto cpuFrequency() const -> double { return information.cpuFrequency; }
  auto apuFrequency() const -> double { return information.apuFrequency; }

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
    string name = "Super Famicom";
    Region region = Region::NTSC;
    f64 cpuFrequency = Constants::Colorburst::NTSC * 6.0;
    f64 apuFrequency = 32040.0 * 768.0;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;

  friend class Cartridge;
};

extern Random random;
extern System system;

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
