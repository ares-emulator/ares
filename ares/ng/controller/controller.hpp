struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto readButtons() -> n8 { return 0; }
  virtual auto readControls() -> n2 { return 0; }
  virtual auto writeOutputs(n3 data) -> void {}
};

#include "port.hpp"
#include "arcade-stick/arcade-stick.hpp"
