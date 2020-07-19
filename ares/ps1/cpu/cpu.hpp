//LSI CoreWare CW33300 (MIPS R3000A core)

struct CPU : Thread {
  Node::Component node;
  Memory::Writable ram;
  Memory::Writable cache;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto exception(string_view) -> void;
    auto interrupt(string_view) -> void;
    auto message(char) -> void;
    auto function() -> void;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification exception;
      Node::Notification interrupt;
      Node::Notification message;
      Node::Notification function;
    } tracer;

    struct Memory {
      Node::Memory ram;
      Node::Memory cache;
    } memory;
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
