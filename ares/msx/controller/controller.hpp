struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto read() -> n6 { return 0x3f; }
  virtual auto write(n8 data) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
