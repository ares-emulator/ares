#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

#include "port.cpp"
#include "21fx/21fx.cpp"
#include "satellaview/satellaview.cpp"

Expansion::Expansion() {
  if(!handle()) Thread::create(1, std::bind_front(&Expansion::main, this));
  cpu.peripherals.push_back(this);
}

Expansion::~Expansion() {
  Thread::destroy();
}

auto Expansion::main() -> void {
  step(1);
  synchronize(cpu);
}

}
