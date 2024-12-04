#include <n64/n64.hpp>

namespace ares::Nintendo64 {

PI pi;
#include "dma.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PI");

  bb_nand.buffer0.allocate(0x200);
  bb_nand.buffer1.allocate(0x200);
  bb_nand.spare0.allocate(0x10);
  bb_nand.spare1.allocate(0x10);

  bb_nand.nand[0] = &nand0;
  bb_nand.nand[1] = &nand1;
  bb_nand.nand[2] = &nand2;
  bb_nand.nand[3] = &nand3;

  bb_aes.ekey.allocate(0xB0);
  bb_aes.iv.allocate(0x10);

  debugger.load(node);
}

auto PI::unload() -> void {
  debugger = {};
  node.reset();

  bb_nand.buffer0.reset();
  bb_nand.buffer1.reset();
  bb_nand.spare0.reset();
  bb_nand.spare1.reset();
  bb_aes.ekey.reset();
  bb_aes.iv.reset();
}

auto PI::power(bool reset) -> void {
  io = {};
  bsd1 = {};
  bsd2 = {};
  if(system._BB()) {
    bb_gpio = {};
    bb_gpio.power.mask = 1;
    bb_gpio.led.mask = 1;
    bb_allowed = {};
    bb_ide[0] = {};
    bb_ide[1] = {};
    bb_ide[2] = {};
    bb_ide[3] = {};
    bb_nand.io = {};
    bb_nand.buffer0.fill();
    bb_nand.buffer1.fill();
    bb_nand.spare0.fill();
    bb_nand.spare1.fill();
    bb_aes.ekey.fill();
    bb_aes.iv.fill();
  }
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
