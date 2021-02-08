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
  auto read() -> n8;
  auto write(n4 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  Node::Input::Button matrix[12][8];

  struct IO {
    n4 select;
  } io;
};

extern Keyboard keyboard;
