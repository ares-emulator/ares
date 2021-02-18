#include <fc/fc.hpp>

namespace ares::Famicom {

PPU ppu;
#include "memory.cpp"
#include "render.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  ciram.allocate(2048);
  cgram.allocate(32);
  oam.allocate(256);

  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 256, 240);
  screen->colors(1 << 9, {&PPU::color, this});
  screen->setSize(256, 240);
  screen->setScale(1.0, 1.0);
  screen->setAspect(8.0, 7.0);

  overscan = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(256, 224);
    if(value == 1) screen->setSize(256, 240);
  });
  overscan->setDynamic(true);

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  debugger.load(node);
}

auto PPU::unload() -> void {
  screen->quit();
  node->remove(screen);
  debugger.unload();
  colorEmulation.reset();
  overscan.reset();
  screen.reset();
  node.reset();
  ciram.reset();
  cgram.reset();
  oam.reset();
}

auto PPU::main() -> void {
  renderScanline();
}

auto PPU::step(u32 clocks) -> void {
  u32 L = vlines();

  while(clocks--) {
    if(io.ly == 240 && io.lx == 340) io.nmiHold = 1;
    if(io.ly == 241 && io.lx ==   0) io.nmiFlag = io.nmiHold;
    if(io.ly == 241 && io.lx ==   2) cpu.nmiLine(io.nmiEnable && io.nmiFlag);

    if(io.ly == L-2 && io.lx == 340) io.spriteZeroHit = 0, io.spriteOverflow = 0;

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
  }
  cartridge.scanline(io.ly);
}

auto PPU::frame() -> void {
  io.field++;
  if(overscan->value() == 0) screen->setViewport(0, 8, 256, 224);
  if(overscan->value() == 1) screen->setViewport(0, 0, 256, 240);
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto PPU::power(bool reset) -> void {
  Thread::create(system.frequency(), {&PPU::main, this});
  screen->power();

  if(!reset) {
    ciram.fill();
    cgram.fill();
    oam.fill();
  }

  io = {};
}

}
