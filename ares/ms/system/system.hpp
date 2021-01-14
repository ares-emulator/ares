struct System {
  Node::System node;
  Node::Setting::String regionNode;

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

    bool yHold = 0;
    bool upLatch = 0;
    bool downLatch = 0;
    bool xHold = 0;
    bool leftLatch = 0;
    bool rightLatch = 0;
  } controls;

  enum class Model : uint { MasterSystem, GameGear };
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
    Model model = Model::MasterSystem;
    Region region = Region::NTSC;
    double colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::MasterSystem() -> bool { return system.model() == System::Model::MasterSystem; }
auto Model::GameGear() -> bool { return system.model() == System::Model::GameGear; }

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
