struct Keyboard {
  Node::Object node;

  //keyboard.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto read(n3 row) -> n12;

  Node::Input::Button matrix[7][12];
};

extern Keyboard keyboard;
