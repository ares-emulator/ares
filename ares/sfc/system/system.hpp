extern Random random;

struct System {
  Node::System node;
  Node::Setting::String regionNode;

  struct Controls {
    Node::Object node;
    Node::Input::Button reset;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
  } controls;

  enum class Region : uint { NTSC, PAL };

  auto name() const -> string { return node->name(); }
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
    Region region = Region::NTSC;
    double cpuFrequency = Constants::Colorburst::NTSC * 6.0;
    double apuFrequency = 32040.0 * 768.0;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;

  friend class Cartridge;
};

extern System system;

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
