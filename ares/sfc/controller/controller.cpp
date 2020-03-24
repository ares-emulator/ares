#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

#include "port.cpp"
#include "gamepad/gamepad.cpp"
#include "justifier/justifier.cpp"
#include "justifiers/justifiers.cpp"
#include "mouse/mouse.cpp"
#include "ntt-data-keypad/ntt-data-keypad.cpp"
#include "super-multitap/super-multitap.cpp"
#include "super-scope/super-scope.cpp"
#include "twin-tap/twin-tap.cpp"

auto Controller::iobit() -> bool {
  if(this == controllerPort1.device.data()) return cpu.pio() & 0x40;
  if(this == controllerPort2.device.data()) return cpu.pio() & 0x80;
  return 1;
}

auto Controller::iobit(bool data) -> void {
  if(this == controllerPort1.device.data()) bus.write(0x4201, cpu.pio() & ~0x40 | data << 6);
  if(this == controllerPort2.device.data()) bus.write(0x4201, cpu.pio() & ~0x80 | data << 7);
}

}
