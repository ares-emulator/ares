//RDRAM Interface

struct RI {
  Node::Component node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification io;
    } tracer;
  } debugger;

  //ri.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern RI ri;
