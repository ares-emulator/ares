struct Keyboard {
  Node::Port port;
  Node::Peripheral layout;

  //keyboard.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto power() -> void;
  auto read() -> uint8;
  auto write(uint4 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  Node::Button matrix[12][8];

  struct IO {
    uint4 select;
  } io;
};

extern Keyboard keyboard;
