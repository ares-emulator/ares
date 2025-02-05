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

  enum class Model : u32 { Pencil2 };
  enum class Region : u32 { PAL };

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto region() const -> Region { return information.region; }
  auto crystalFrequency() const -> f64 { return information.crystalFrequency; }

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

  u8 bios[0x2000];

private:
  struct Information {
    string name = "Pencil 2";
    Model model = Model::Pencil2;
    Region region = Region::PAL;
    f64 crystalFrequency = 10'738'635;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::Pencil2() -> bool { return system.model() == System::Model::Pencil2; }

auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
