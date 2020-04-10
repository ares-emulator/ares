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

  node = parent->append<Node::Component>("PPU");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1 << 9, {&PPU::color, this});
  screen->setSize(256, 240);
  screen->setScale(1.0, 1.0);
  screen->setAspect(8.0, 7.0);

  overscan = screen->append<Node::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(256, 224);
    if(value == 1) screen->setSize(256, 240);
  });
  overscan->setDynamic(true);

  colorEmulation = screen->append<Node::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  debugger.load(node);
}

auto PPU::unload() -> void {
  ciram.reset();
  cgram.reset();
  oam.reset();
  node = {};
  screen = {};
  overscan = {};
  colorEmulation = {};
  debugger = {};
}

auto PPU::main() -> void {
  renderScanline();
}

auto PPU::step(uint clocks) -> void {
  uint L = vlines();

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
  scheduler.exit(Event::Frame);
}

auto PPU::refresh() -> void {
  if(overscan->value() == 0) {
    screen->refresh(buffer + 8 * 256, 256 * sizeof(uint32), 256, 224);
  }

  if(overscan->value() == 1) {
    screen->refresh(buffer, 256 * sizeof(uint32), 256, 240);
  }
}

auto PPU::power(bool reset) -> void {
  Thread::create(system.frequency(), {&PPU::main, this});

  io = {};
  latch = {};

  if(!reset) {
    for(auto& data : ciram ) data = 0;
    for(auto& data : cgram ) data = 0;
    for(auto& data : oam   ) data = 0;
  }

  for(auto& data : buffer) data = 0;
}

}
