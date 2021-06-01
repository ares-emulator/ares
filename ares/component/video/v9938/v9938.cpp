#include <ares/ares.hpp>
#include "v9938.hpp"

namespace ares {

#include "io.cpp"
#include "background.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "commands.cpp"
#include "serialization.cpp"

auto V9938::load(Node::Video::Screen screen) -> void {
  this->screen = screen;
}

auto V9938::unload() -> void {
  screen.reset();
}

auto V9938::main() -> void {
  if(io.vcounter < vlines()) {
    n9 y = io.vcounter;
    background.setup(y);
    sprite.setup(y);
    dac.setup(y);

    while(io.hcounter < 256) {
      n9 x = io.hcounter;
      background.run(x, y);
      sprite.run(x, y);
      dac.run(x, y);
      tick(1);
    }

    tick(200);
  } else {
    tick(456);
  }

  io.hcounter = 0;
  io.vcounter++;

  if(io.vcounter >= vtotal()) {
    io.vcounter = 0;

    latch.timing    = io.timing;
    latch.interlace = io.interlace;
    latch.overscan  = io.overscan;
    latch.field     = !latch.field;
  }

  if(io.vcounter == vlines()) {
    virq.pending |= virq.enable;
    poll();
    frame();
  }

  if(io.vcounter == hirq.coincidence) {
    hirq.pending |= hirq.enable;
    poll();
  }
}

auto V9938::poll() -> void {
  irq(virq.pending | hirq.pending | lirq.pending);
}

auto V9938::tick(u32 clocks) -> void {
  io.hcounter += clocks;
  while(clocks--) {
    command();
    step(1);
  }
}

auto V9938::power() -> void {
  background.power();
  sprite.power();
  dac.power();
  op = {};
  virq = {};
  hirq = {};
  lirq = {};
  io = {};
  latch = {};
}

}
