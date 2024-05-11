#include <fc/fc.hpp>

namespace ares::Famicom {

PPU ppu;
#include "memory.cpp"
#include "render.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "sprite.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  ciram.allocate(2048);
  cgram.allocate(32);
  spriteEvaluation.oam.allocate(256);

  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 282, displayHeight());
  screen->colors(1 << 9, {&PPU::color, this});
  screen->setSize(282, displayHeight());
  screen->setScale(1.0, 1.0);
  Region::PAL() ? screen->setAspect(55.0, 43.0) :screen->setAspect(8.0, 7.0);
  screen->refreshRateHint(system.frequency() / rate(), 341, vlines());

  debugger.load(node);
}

auto PPU::unload() -> void {
  screen->quit();
  node->remove(screen);
  debugger.unload();
  screen.reset();
  node.reset();
  ciram.reset();
  cgram.reset();
  spriteEvaluation.oam.reset();
}

auto PPU::main() -> void {
  renderScanline();
}

auto PPU::step(u32 clocks) -> void {
  u32 L = vlines();

  while(clocks--) {
    spriteEvaluation.main();

    if(io.ly == 240 && io.lx == 340) io.nmiHold = 1;
    if(io.ly == 241 && io.lx ==   0) io.nmiFlag = io.nmiHold;
    if(io.ly == 241 && io.lx ==   2) cpu.nmiLine(io.nmiEnable && io.nmiFlag);

    if(io.ly == L-2 && io.lx == 340) io.spriteZeroHit = 0, spriteEvaluation.io.spriteOverflow = 0;

    if(io.ly == L-2 && io.lx == 340) io.nmiHold = 0;
    if(io.ly == L-1 && io.lx ==   0) io.nmiFlag = io.nmiHold;
    if(io.ly == L-1 && io.lx ==   2) cpu.nmiLine(io.nmiEnable && io.nmiFlag);

    Thread::step(rate());
    Thread::synchronize(cpu);

    io.lx++;
  }
}

auto PPU::scanline() -> void {
  io.lx = 0;
  if(++io.ly == vlines()) {
    io.ly = 0;
    frame();
    io.mdr = 0; // io.mdr decays after approx 1 frame
  }
  cartridge.scanline(io.ly);
}

auto PPU::frame() -> void {
  io.field++;

  if(screen->overscan()) {
    screen->setSize(282, displayHeight());
    screen->setViewport(0, 0, 282, displayHeight());
  } else {
    int x = 16;
    int y = 8;
    int width = 282 - 32;
    int height = displayHeight() - 16;

    if(Region::PAL()) height -= 48;

    screen->setSize(width, height);
    screen->setViewport(x, y, width, height);
  }

  screen->frame();
  scheduler.exit(Event::Frame);
}

auto PPU::power(bool reset) -> void {
  Thread::create(system.frequency(), {&PPU::main, this});
  screen->power();
  spriteEvaluation.power(reset);

  if(!reset) {
    ciram.fill();
    cgram.fill();
  }

  io = {};
}

}
