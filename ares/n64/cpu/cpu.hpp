//NEC VR4300

struct CPU : Thread {
  Node::Component node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto exception(string_view) -> void;
    auto interrupt(string_view) -> void;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification exception;
      Node::Notification interrupt;
    } tracer;
  } debugger;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  #include "core/core.hpp"
};

extern CPU cpu;
