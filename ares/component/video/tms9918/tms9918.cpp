#include <ares/ares.hpp>
#include "tms9918.hpp"

namespace ares {

#include "io.cpp"
#include "background.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "serialization.cpp"

auto TMS9918::load(Node::Video::Screen screen) -> void {
  this->screen = screen;
}

auto TMS9918::unload() -> void {
  this->screen.reset();
}

auto TMS9918::main() -> void {
  if(io.vcounter < 192) {
    n8 y = io.vcounter;
    background.setup(y);
    sprite.setup(y);
    dac.setup(y);
    for(n8 x : range(256)) {
      background.run(x, y);
      sprite.run(x, y);
      dac.run(x, y);
      step(1);
    }
    step(200);
  } else {
    step(456);
  }

  io.vcounter++;
  if(io.vcounter == 262) io.vcounter = 0;
  if(io.vcounter == 192) irqFrame.pending = 1, poll(), frame();
}

auto TMS9918::poll() -> void {
  irq(irqFrame.pending && irqFrame.enable);
}

auto TMS9918::power() -> void {
  background.power();
  sprite.power();
  dac.power();
  irqFrame = {};
  io = {};
}

}
