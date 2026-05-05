struct RIOT : Thread {
  Node::Object node;
  Memory::Writable<n8> ram;

  //riot.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  auto clockTimer() -> void;
  auto reloadTimer(n8 data, n16 interval, n1 interruptEnable) -> void;

  //io.cpp
  auto readRam(n8 address) -> n8;
  auto writeRam(n8 address, n8 data) -> void;
  auto readIo(n8 address) -> n8;
  auto writeIo(n8 address, n8 data) -> void;

  auto readPortA() -> n8;
  auto readPortB() -> n8;
  auto writePortA(n8 data) -> void;
  auto writePortB(n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct {
    n8  counter;
    n16 interval;
    n16 prescaler;
    n1  interruptEnable;
    n1  interruptFlag;
    n1  underflow;
    n1  holdZero;
    n1  justWrapped;
  } timer;

  struct Port {
    n8 data;
    n8 direction;
  } port[2];

  n1 leftDifficulty;
  n1 leftDifficultyLatch;
  n1 rightDifficulty;
  n1 rightDifficultyLatch;
  n1 tvType;
  n1 tvTypeLatch;
};

extern RIOT riot;
