struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
