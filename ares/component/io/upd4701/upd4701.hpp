#pragma once

namespace ares {

struct uPD4701A {
  enum : u32 { Middle = 1, Right = 2, Left = 4 };

  auto power() -> void;
  auto serialize(serializer&) -> void;

  auto addX(s32 delta) -> void;
  auto addY(s32 delta) -> void;
  auto setSwitch(u32 mask, bool pressed) -> void;

  auto setChipSelect(bool select) -> void;
  auto setAxis(bool y) -> void;
  auto setByte(bool upper) -> void;
  auto setResetX(bool reset) -> void;
  auto setResetY(bool reset) -> void;
  auto resetPulse(bool x, bool y) -> void;

  auto readData() -> n8;
  auto readSelected(n2 selection) -> n8;

  auto switchFlag() const -> bool { return switches == 0; }
  auto countFlag() const -> bool { return cf; }

private:
  auto latch() -> void;

  n1 cs = 1;
  n1 xy;
  n1 ul;
  n1 resetX;
  n1 resetY;
  n12 x;
  n12 y;
  n12 startX;
  n12 startY;
  n12 latchX;
  n12 latchY;
  n3 switches;
  n4 latchSwitches;
  n1 cf = 1;
};

}
