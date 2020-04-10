struct NECDSP : uPD96050, Thread {
  Node::Component node;
  uint Frequency = 0;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Instruction instruction;
    } tracer;
  } debugger;

  //necdsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //memory.cpp
  auto read(uint24 address, uint8 data) -> uint8;
  auto write(uint24 address, uint8 data) -> void;

  auto readRAM(uint24 address, uint8 data) -> uint8;
  auto writeRAM(uint24 address, uint8 data) -> void;

  auto firmware() const -> vector<uint8>;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern NECDSP necdsp;
