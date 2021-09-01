struct Controller : Thread {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto read() -> n4 { return 0x0f; }
  virtual auto write(n2) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
#include "avenuepad/avenuepad.hpp"
#include "multitap/multitap.hpp"
