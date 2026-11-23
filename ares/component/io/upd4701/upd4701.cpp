#include <ares/ares.hpp>
#include "upd4701.hpp"

namespace ares {

#include "serialization.cpp"

auto uPD4701A::power() -> void {
  cs = 1;
  xy = 0;
  ul = 0;
  resetX = 0;
  resetY = 0;
  x = y = 0;
  startX = startY = 0;
  latchX = latchY = 0;
  switches = 0;
  latchSwitches = 0;
  cf = 1;
}

auto uPD4701A::addX(s32 delta) -> void {
  if(resetX || delta == 0) return;
  x += delta;
  if(cs) cf = 0;
}

auto uPD4701A::addY(s32 delta) -> void {
  if(resetY || delta == 0) return;
  y += delta;
  if(cs) cf = 0;
}

auto uPD4701A::setSwitch(u32 mask, bool pressed) -> void {
  if(pressed) switches |= mask;
  else switches &= ~mask;
}

auto uPD4701A::latch() -> void {
  latchX = x - startX;
  latchY = y - startY;
  latchSwitches = switches;
  if(switches) latchSwitches |= 8;
  cf = 1;
}

auto uPD4701A::setChipSelect(bool select) -> void {
  if(cs == select) return;
  cs = select;
  if(!cs) latch();
}

auto uPD4701A::setAxis(bool y) -> void {
  xy = y;
}

auto uPD4701A::setByte(bool upper) -> void {
  ul = upper;
}

auto uPD4701A::setResetX(bool reset) -> void {
  if(resetX == reset) return;
  resetX = reset;
  if(resetX) startX = x;
}

auto uPD4701A::setResetY(bool reset) -> void {
  if(resetY == reset) return;
  resetY = reset;
  if(resetY) startY = y;
}

auto uPD4701A::resetPulse(bool x, bool y) -> void {
  if(x) { setResetX(1); setResetX(0); }
  if(y) { setResetY(1); setResetY(0); }
}

auto uPD4701A::readData() -> n8 {
  if(cs) return 0;
  n16 data = xy ? latchY : latchX;
  data |= latchSwitches << 12;
  if(ul) return data >> 8;
  return data;
}

auto uPD4701A::readSelected(n2 selection) -> n8 {
  n1 previous = cs;
  setChipSelect(0);
  setAxis(selection.bit(1));
  setByte(selection.bit(0));
  n8 data = readData();
  setChipSelect(previous);
  return data;
}

}
