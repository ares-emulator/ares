namespace Board {

struct Interface {
  virtual auto load(Markup::Node) -> void {}
  virtual auto save(Markup::Node) -> void {}
  virtual auto read(uint8 bank, uint13 address) -> uint8 { return 0x00; }
  virtual auto write(uint8 bank, uint13 address, uint8 data) -> void {}
  virtual auto power() -> void {}
  virtual auto serialize(serializer&) -> void {}
};

#include "linear.hpp"
#include "split.hpp"
#include "banked.hpp"
#include "ram.hpp"
#include "system-card.hpp"

}
