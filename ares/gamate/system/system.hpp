struct System {
  Node::System node;
  VFS::Pak pak;

  //the 4KB boot ROM inside the system ASIC; it draws the splash and hands over
  Memory::Readable<n8> bios;

  struct Controls {
    Node::Object node;
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button a;
    Node::Input::Button b;
    Node::Input::Button start;
    Node::Input::Button select;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
    auto read() -> n8;
  } controls;

  auto name() const -> string { return information.name; }

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
    string name = "Gamate";
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;
