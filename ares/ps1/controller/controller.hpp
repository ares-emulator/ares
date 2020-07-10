struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;
  virtual auto read() -> u64 { return 0; }
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
