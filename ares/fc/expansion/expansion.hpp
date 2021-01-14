// Famicom expansion port pinout:
//
//  __________________________________
// /                                  \
// \ (01)(02)(03)(04)(05)(06)(07)(08) /
//  \ (09)(10)(11)(12)(13)(14)(15)   /
//   \______________________________/
//
// pin  name     register
// 01:  gnd
// 02:  audio
// 03:  irq
// 04:  read2.4  $4017.d4 read
// 05:  read2.3  $4017.d3 read
// 06:  read2.2  $4017.d2 read
// 07:  read2.1  $4017.d1 read
// 08:  read2.0  $4017.d0 read
// 09:  clock2   $4017 read
// 10:  write2   $4016.d2 write
// 11:  write1   $4016.d1 write
// 12:  latch    $4016.d0 write
// 13:  read1    $4016.d1 read
// 14:  clock1   $4016 read
// 15:  +5v

struct Expansion {
  Node::Peripheral node;

  virtual ~Expansion() = default;

  virtual auto read1() -> n1 { return 0; }
  virtual auto read2() -> n5 { return 0; }
  virtual auto write(n3 data) -> void {}
};

#include "port.hpp"
#include "family-keyboard/family-keyboard.hpp"
