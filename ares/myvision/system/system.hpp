struct System {
  Node::System node;
  VFS::Pak pak;

  struct Controls {
    Node::Object node;
    Node::Input::Button B;
    Node::Input::Button C;
    Node::Input::Button A;
    Node::Input::Button D;
    Node::Input::Button E;
    Node::Input::Button _1;
    Node::Input::Button _2;
    Node::Input::Button _3;
    Node::Input::Button _4;
    Node::Input::Button _5;
    Node::Input::Button _6;
    Node::Input::Button _7;
    Node::Input::Button _8;
    Node::Input::Button _9;
    Node::Input::Button _10;
    Node::Input::Button _11;
    Node::Input::Button _12;
    Node::Input::Button _13;
    Node::Input::Button _14;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
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
    string name = "MyVision";
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;
