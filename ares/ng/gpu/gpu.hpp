//custom chipset

struct GPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Memory::Writable<n16> vram;
  Memory::Writable<n16> pram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
      Node::Debugger::Memory pram;
    } memory;
  } debugger;

  //gpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void;
  auto main() -> void;
  auto frame() -> void;
  auto power(bool reset) -> void;

  //render.cpp
  auto render(n9 y) -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n9  hcounter;
    n9  vcounter;
    n1  shadow;
    n16 vramAddress;
    n16 vramIncrement;
    n1  pramBank;
    n1  autoAnimationDisable;
    n8  autoAnimationSpeed;
    n3  autoAnimationCounter;
    n1  timerInterruptEnable;
    n1  timerReloadOnChange;
    n1  timerReloadOnVblank;
    n1  timerReloadOnZero;
    n32 timerReload;
    n32 timerCounter;
    n1  timerStopPAL;
  } io;

  struct IRQ {
    n1 powerAcknowledge;
    n1 timerAcknowledge;
    n1 vblankAcknowledge;
  } irq;
};

extern GPU gpu;
