struct System {
  Node::System node;

  struct Controls {
    Node::Object node;

    //Master System
    Node::Input::Button pause;
    Node::Input::Button reset;

    //Game Gear
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button one;
    Node::Input::Button two;
    Node::Input::Button start;

    auto load(Node::Object) -> void;
    auto poll() -> void;

    n1 yHold;
    n1 upLatch;
    n1 downLatch;
    n1 xHold;
    n1 leftLatch;
    n1 rightLatch;
  } controls;

  enum class Model : u32 { MarkIII, MasterSystemI, MasterSystemII, GameGear };
  enum class Region : u32 { NTSCJ, NTSCU, PAL };

  auto name() const -> string { return information.name; }
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
    string name = "Master System";
    Model model = Model::MasterSystemI;
    Region region = Region::NTSCJ;
    f64 colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::MarkIII() -> bool { return system.model() == System::Model::MarkIII; }
auto Model::MasterSystemI() -> bool { return system.model() == System::Model::MasterSystemI; }
auto Model::MasterSystemII() -> bool { return system.model() == System::Model::MasterSystemII; }
auto Model::MasterSystem() -> bool { return MarkIII() || MasterSystemI() || MasterSystemII(); }
auto Model::GameGear() -> bool { return system.model() == System::Model::GameGear; }

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
