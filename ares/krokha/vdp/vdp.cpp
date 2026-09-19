#include <krokha/krokha.hpp>

namespace ares::Krokha {

VDP vdp;
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", width, height);
  screen->colors(2, std::bind_front(&VDP::color, this));
  screen->setSize(width, height);
  screen->setViewport(0, 0, width, height);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
  screen->refreshRateHint(system.colorburst(), dotsPerLine, linesPerFrame);
}

auto VDP::unload() -> void {
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::main() -> void {
  if(vcounter < height) scanline();

  //vertical blank: the flip-flop that raises INT is set here and cleared by
  //the CPU's interrupt acknowledge cycle. see cpu.cpp
  if(vcounter == height) cpu.setIRQ(1);

  step(dotsPerLine);

  if(++vcounter >= linesPerFrame) {
    vcounter = 0;
    frame();
  }
}

auto VDP::scanline() -> void {
  auto output = screen->pixels().data() + vcounter * width;
  u32 row = vcounter >> 3;
  u32 ra  = vcounter & 7;

  //columns 0-15 are scanned but never displayed, so start at 16
  for(u32 x = 16; x < 64; x++) {
    n8 gfx = cartridge.character(cpu.ram.read(x << 5 | row) << 3 | ra);
    for(u32 i : range(8)) *output++ = gfx.bit(i ^ 7);
  }
}

auto VDP::frame() -> void {
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::color(n32 color) -> n64 {
  if(color == 0) return 0x0000'0000'0000ull;
  return 0xffff'ffff'ffffull;
}

auto VDP::power() -> void {
  Thread::create(system.colorburst(), std::bind_front(&VDP::main, this));
  screen->power();

  vcounter = 0;
}

auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

}
