// Ricoh RP-5C01A
// Alarm and test functionality omitted as it is not used on MSX
struct RTC : Thread {
  Node::Object node;
  Memory::Writable<n4> sram; // 26 bytes

  //rtc.cpp
  auto load(Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto select(n4 reg) -> void { io.registerIndex = reg; }
  auto read() -> n4;
  auto write(n4 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct {
    n2 registerBank;
    n4 registerIndex;
    n1 timerEnable;
  } io;
};

extern RTC rtc;
