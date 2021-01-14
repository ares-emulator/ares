struct System {
  Node::System node;
  Node::Setting::String regionNode;

  struct Controls {
    Node::Object node;
    Node::Input::Button pause;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
  } controls;

  enum class Model : uint { SG1000, SC3000 };
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
    Model model = Model::SG1000;
    Region region = Region::NTSC;
    double colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::SG1000() -> bool { return system.model() == System::Model::SG1000; }
auto Model::SC3000() -> bool { return system.model() == System::Model::SC3000; }

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
