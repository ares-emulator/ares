#include <ms/ms.hpp>

namespace ares::MasterSystem {

PPI ppi;

auto PPI::power() -> void {
  I8255::power();
  control = 0xff;
  x = y = startX = startY = latchX = latchY = 0;
  switches = 0;
}

auto PPI::readPortA() -> n8 {
  if(game == "hangonjr") {
    if(control.bit(0)) {
      return std::clamp((i32)system.arcadeControls.accelerator->value() >> 7, 0, 255);
    }
    auto angle = ((i32)system.arcadeControls.analogX->value() + 32768) >> 8;
    if(angle < 0x20) angle = 0x20;
    if(angle > 0xe0) angle = 0xe0;
    return angle;
  }

  if(game == "ridleofp" || game == "megrescu") {
    if(control.bit(4)) return 0xff;
    u16 value = control.bit(3) ? latchY : latchX;
    value |= (u16)switches << 12;
    return control.bit(2) ? value >> 8 : value & 0xff;
  }
  return 0xff;
}

auto PPI::readPortC() -> n8 {
  if(game == "hangonjr") return 0xef;
  return 0xff;
}

auto PPI::writePortC(n8 data) -> void {
  if((game == "ridleofp" || game == "megrescu") && control.bit(4) && !data.bit(4)) {
    latchX = (x - startX) & 0x0fff;
    latchY = (y - startY) & 0x0fff;
    switches = 0;
    if(system.arcadeControls.button1[0]->value()) switches |= 4;
    if(system.arcadeControls.button2[0]->value()) switches |= 1;
    if(switches) switches |= 8;
  }
  if(!control.bit(1) && data.bit(1)) startX = x;
  if(!control.bit(0) && data.bit(0)) startY = y;
  control = data;
}

auto PPI::serialize(serializer& s) -> void {
  I8255::serialize(s);
  s(control);
  s(x); s(y); s(startX); s(startY); s(latchX); s(latchY);
  s(switches);
}

}
