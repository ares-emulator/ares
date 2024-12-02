#include <n64/n64.hpp>

namespace ares::Nintendo64 {

PI pi;
#include "dma.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PI");

  debugger.load(node);
}

auto PI::unload() -> void {
  debugger = {};
  node.reset();
}

auto PI::power(bool reset) -> void {
  io = {};
  bsd1 = {};
  bsd2 = {};
  bb_gpio = {};
  bb_gpio.power.mask = 1;
  bb_gpio.led.mask = 1;
  bb_allowed = {};
  bb_ide[0] = 0;
  bb_ide[1] = 0;
  bb_ide[2] = 0;
  bb_ide[3] = 0;
}

auto PI::access() -> BBAccess {
  if(mi.secure()) return (BBAccess){
      .buf = 1,
      .flash = 1,
      .atb = 1,
      .aes = 1,
      .dma = 1,
      .gpio = 1,
      .ide = 1,
      .err = 1,
    };
  else return bb_allowed;
}

}
