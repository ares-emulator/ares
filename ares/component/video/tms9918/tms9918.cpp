#include <ares/ares.hpp>
#include "tms9918.hpp"

namespace ares {

#include "io.cpp"
#include "background.cpp"
#include "sprites.cpp"
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
    sprite(y);
    auto line = screen->pixels().data() + y * 256;
    for(n8 x : range(256)) {
      background(x, y);
      sprite(x, y);
      line[x] = output.color;
      step(1);
    }
    step(200);
  } else {
    step(456);
  }

  io.vcounter++;
  if(io.vcounter == 262) io.vcounter = 0;
  if(io.vcounter ==   0) io.irqLine = 0;
  if(io.vcounter == 192) io.irqLine = 1, irq(io.irqEnable), frame();
}

auto TMS9918::power() -> void {
  io = {};
}

}
