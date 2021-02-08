struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto readData() -> n8 { return 0xff; }
  virtual auto writeData(n8 data) -> void {}
};

#include "port.hpp"
#include "control-pad/control-pad.hpp"
#include "fighting-pad/fighting-pad.hpp"
