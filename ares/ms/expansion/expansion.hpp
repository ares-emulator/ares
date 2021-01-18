struct Expansion {
  Node::Peripheral node;

  Expansion();
  virtual ~Expansion();

  virtual auto serialize(serializer& s) -> void {}
};

#include "port.hpp"
#include "fmsound/fmsound.hpp"
