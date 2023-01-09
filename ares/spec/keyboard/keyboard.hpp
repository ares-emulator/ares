struct Keyboard {
  Node::Port port;
  Node::Peripheral layout;

  //keyboard.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port, string) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto power() -> void;
  auto read(u8 row) -> n5 ;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  Node::Input::Button matrix[8][5];
};

extern Keyboard keyboard;
