extern Random random;

struct System {
  Node::System node;

  struct Controls {
    Node::Object node;
    Node::Input::Button reset;
    Node::Input::Button select;
    Node::Input::Button leftDifficulty;
    Node::Input::Button rightDifficulty;
    Node::Input::Button tvType;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
  } controls;

  enum class Region : u32 { NTSC, PAL, SECAM };

  auto name() const -> string { return information.name; }
  auto region() const -> Region { return information.region; }
  auto frequency() const -> u32 { return information.frequency; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    string name = "Atari 2600";
    Region region = Region::NTSC;
    u32 frequency = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Region::NTSC()  -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL()   -> bool { return system.region() == System::Region::PAL; }
auto Region::SECAM() -> bool { return system.region() == System::Region::SECAM; }
