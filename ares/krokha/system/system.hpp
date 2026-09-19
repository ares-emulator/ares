struct System {
  Node::System node;
  VFS::Pak pak;

  struct Controls {
    Node::Object node;
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button fire;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
    auto read() -> n8;
  } controls;

  auto name() const -> string { return information.name; }

  //К580ВМ80А clocked from an 8MHz crystal divided by four
  auto frequency() const -> u32 { return 8'000'000 / 4; }

  //the video counter chain runs from the undivided crystal
  auto colorburst() const -> u32 { return 8'000'000; }

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
    string name = "Krokha";
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;
