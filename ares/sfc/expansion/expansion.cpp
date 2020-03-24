#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

#include <sfc/expansion/port.cpp>
#include <sfc/expansion/21fx/21fx.cpp>
#include <sfc/expansion/satellaview/satellaview.cpp>

Expansion::Expansion() {
  if(!handle()) Thread::create(1, {&Expansion::main, this});
  cpu.peripherals.append(this);
}

Expansion::~Expansion() {
  Thread::destroy();
}

auto Expansion::main() -> void {
  step(1);
  synchronize(cpu);
}

}
