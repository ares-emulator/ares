struct Expansion {
  Node::Peripheral node;
  virtual ~Expansion() = default;

  virtual auto romcs() -> bool = 0;
  virtual auto mapped(n16 address, bool io) -> bool = 0;
  virtual auto read(n16 address) -> n8 = 0;
  virtual auto write(n16 address, n8 data) -> void = 0;
  virtual auto in(n16 address) -> n8 = 0;
  virtual auto out(n16 address, n8 data) -> void = 0;
};

#include "kempston/kempston.hpp"
#include "port.hpp"
