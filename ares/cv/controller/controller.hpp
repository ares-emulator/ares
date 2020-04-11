struct Controller : Thread {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto read() -> uint8 { return 0xff; }
  virtual auto write(uint8 data) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
