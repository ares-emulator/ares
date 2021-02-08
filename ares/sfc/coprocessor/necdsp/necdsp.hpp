struct NECDSP : uPD96050, Thread {
  Node::Object node;
  n32 Frequency = 0;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
    } tracer;
  } debugger;

  //necdsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //memory.cpp
  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  auto readRAM(n24 address, n8 data) -> n8;
  auto writeRAM(n24 address, n8 data) -> void;

  auto firmware() const -> vector<n8>;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern NECDSP necdsp;
