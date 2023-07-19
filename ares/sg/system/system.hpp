struct System {
  Node::System node;

  struct ArcadeControls {
    Node::Object node;
    Node::Input::Button p1up;
    Node::Input::Button p1down;
    Node::Input::Button p1left;
    Node::Input::Button p1right;
    Node::Input::Button p1b1;
    Node::Input::Button p1b2;
    Node::Input::Button p1start;
    Node::Input::Button coin;
    Node::Input::Button service;
    Node::Input::Button p2up;
    Node::Input::Button p2down;
    Node::Input::Button p2left;
    Node::Input::Button p2right;
    Node::Input::Button p2b1;
    Node::Input::Button p2b2;
    Node::Input::Button p2start;

    //arcade-controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
  } arcadeControls;

  enum class Model : u32 { SG1000, SG1000A, SC3000 };
  enum class Region : u32 { NTSC, PAL };

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
    string name = "SG-1000";
    Model model = Model::SG1000;
    Region region = Region::NTSC;
    f64 colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::SG1000() -> bool { return system.model() == System::Model::SG1000; }
auto Model::SC3000() -> bool { return system.model() == System::Model::SC3000; }
auto Model::SG1000A() -> bool { return system.model() == System::Model::SG1000A; }

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
