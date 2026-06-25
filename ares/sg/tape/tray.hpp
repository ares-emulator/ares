struct TapeDeckTray {
  Node::Port port;
  Tape tape;

  //tray.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
};
