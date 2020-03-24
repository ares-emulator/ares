struct Controller : Thread {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto read() -> uint4 { return 0x0f; }
  virtual auto write(uint2) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
#include "avenuepad/avenuepad.hpp"
