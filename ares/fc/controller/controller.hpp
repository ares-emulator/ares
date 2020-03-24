// NES controller port pinout:
//  ____
// |    \
// |(7)  \
// |(2)(1)|
// |(3)(5)|
// |(4)(6)|
// |______|
//
// pin  name   port1           port2
//  1:  +5v
//  2:  clock  $4016 read      $4016.d0 write
//  3:  latch  $4016.d0 write  $4016.d0 write
//  4:  data0  $4016.d0 read   $4017.d0 read
//  5:  data3  $4016.d3 read   $4017.d3 read
//  6:  data4  $4016.d4 read   $4017.d4 read
//  7:  gnd

// The Famicom contained two hardwired controllers.
// Controller Port 2 contained a microphone, but no Select or Start buttons.

struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto data() -> uint3 { return 0b000; }
  virtual auto latch(uint1 data) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
