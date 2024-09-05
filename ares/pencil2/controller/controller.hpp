struct Controller : Thread {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto read() -> n8 { return 0xff; }
  virtual auto write(n8 data) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
