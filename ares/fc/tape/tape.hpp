struct Tape {
  Node::Peripheral node;

  virtual ~Tape() = default;

  virtual auto read() -> n1 = 0;
  virtual auto write(n3 data) -> void = 0;

  virtual auto serialize(serializer &) -> void {}
};

#include "port.hpp"
#include "family-basic-data-recorder/family-basic-data-recorder.hpp"