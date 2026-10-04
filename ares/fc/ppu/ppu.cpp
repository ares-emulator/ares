#include <fc/fc.hpp>

namespace ares::Famicom {

PPU ppu;
#include "model.cpp"
#include "memory.cpp"
#include "render.cpp"
#include "debugger.cpp"
#include "sprite.cpp"
#include "scroll.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  model = createBaseModel();

  ciram.allocate(2048);
  cgram.allocate(32);
  oam.allocate(256);
  soam.allocate(32);

  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 283, displayHeight());
  screen->colors(1 << 9, std::bind_front(&PPU::color, this));
  screen->setSize(283, displayHeight());
  screen->setScale(1.0, 1.0);
  screen->setAspect(model->raster.aspectX, model->raster.aspectY);
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
  oam.reset();
  soam.reset();
  model.reset();
}

auto PPU::main() -> void {
  renderScanline();
}

auto PPU::step(u32 clocks) -> void {
  u32 L = vlines();

  while(clocks--) {
    if (var.blockingRead) --var.blockingRead;
    scrollTransferDelay();

    if (enable() && io.ly == L - 1)
      cyclePrepareSpriteEvaluation();

    if(enable() && model->raster.evaluatesSprites(io.ly))
      cycleSpriteEvaluation();

    if (enable() && (io.ly < 240 || io.ly == L - 1))
      cycleScroll();

    u32 vbs = vblankScanline();
    if(io.ly == vbs - 1 && io.lx ==   1) io.busAddress = var.address, cartridge.ppuAddressBus(io.busAddress);
    if(io.ly == vbs - 1 && io.lx == 340) io.nmiHold = 1;
    if(io.ly == vbs     && io.lx ==   0) io.nmiFlag = io.nmiHold;
    if(io.ly == vbs     && io.lx ==   2) cpu.nmiLine(io.nmiEnable && io.nmiFlag);

    if(io.ly == L-2 && io.lx == 340) io.spriteZeroHit = 0, sprite.spriteOverflow = 0;

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
    screen->setSize(283, displayHeight());
    screen->setViewport(0, 0, 283, displayHeight());
  } else {
    auto& raster = model->raster;
    screen->setSize(raster.viewportWidth, raster.viewportHeight);
    screen->setViewport(raster.viewportX, raster.viewportY, raster.viewportWidth, raster.viewportHeight);
  }

  screen->frame();
  scheduler.exit(Event::Frame);
}

auto PPU::power(bool reset) -> void {
  if(!reset && system.model() == System::Model::VsUniSystem) {
    auto candidate = createVsModel();
    if(!candidate) return;
    model = std::move(candidate);
    screen->resetPalette();
  }

  Thread::create(system.frequency(), std::bind_front(&PPU::main, this));
  screen->power();

  if(!reset) {
    ciram.fill();
    oam.fill();
    soam.fill();
    memory::copy(cgram.data(), cgramBootValue, 32);
  }

  scroll = {};
  var = {};
  io = {};
  latch = {};
  sprite = {};
}

}
