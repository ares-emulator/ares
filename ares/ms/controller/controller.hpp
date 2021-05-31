// Master System port pinout:
//  ---------------------
// \ (1) (2) (3) (4) (5) /
//  \  (6) (7) (8) (9)  /
//   ------------------
// pin  name  bit
//  1:  D0    0
//  2:  D1    1
//  3:  D2    2
//  4:  D3    3
//  5:  VCC   -
//  6:  TL    4
//  7:  TH    5
//  8:  GND   -
//  9:  TR    6

struct Controller {
  Node::Peripheral node;

  virtual ~Controller() = default;

  virtual auto read() -> n7 { return 0x7f; }
  virtual auto write(n8 data) -> void {}
};

#include "port.hpp"
#include "gamepad/gamepad.hpp"
#include "light-phaser/light-phaser.hpp"
