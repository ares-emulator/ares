struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;
  virtual auto read() -> uint32 { return 0; }
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
