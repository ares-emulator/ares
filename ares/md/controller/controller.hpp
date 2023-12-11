// Mega Drive port pinout:
// ----------------------
// \ (1) (2) (3) (4) (5) /
//  \  (6) (7) (8) (9)  /
//   ------------------
// pin  name
//  1:  D0
//  2:  D1
//  3:  D2
//  4:  D3
//  5:  VCC
//  6:  TL
//  7:  TH
//  8:  GND
//  9:  TR

struct Controller {
  Node::Peripheral node;

  struct Data {
    n8 value;
    n8 mask;
  };

  virtual ~Controller() = default;

  virtual auto poll() -> void {}
  virtual auto readData() -> Data { return {0x7f, 0x7f}; }
  virtual auto writeData(n8 data) -> void {}
};

#include "port.hpp"
#include "control-pad/control-pad.hpp"
#include "fighting-pad/fighting-pad.hpp"
#include "mega-mouse/mega-mouse.hpp"
