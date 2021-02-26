struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;
};

#include "port.hpp"
#include "arcade-stick/arcade-stick.hpp"
