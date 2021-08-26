#include <gb/gb.hpp>

namespace ares::GameBoy {

PPU ppu;
#include "timing.cpp"
#include "io.cpp"
#include "dmg.cpp"
#include "cgb.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  vram.allocate(!Model::GameBoyColor() ? 8_KiB : 16_KiB);
  oam.allocate(160);
  bgp.allocate(4);
  obp.allocate(8);
  bgpd.allocate(32);
  obpd.allocate(32);

  node = parent->append<Node::Object>("PPU");

  if(Model::GameBoy() || Model::GameBoyColor()) {
    screen = node->append<Node::Video::Screen>("Screen", 160, 144);
    screen->setViewport(0, 0, 160, 144);
    screen->setSize(160, 144);
    screen->setScale(1.0, 1.0);
    screen->setAspect(1.0, 1.0);

    if(Model::GameBoy()) {
      screen->colors(1 << 2, {&PPU::colorGameBoy, this});
      screen->setFillColor(0);

      colorEmulationDMG = screen->append<Node::Setting::String>("Color Emulation", "Game Boy", [&](auto value) {
        screen->resetPalette();
      });
      colorEmulationDMG->setAllowedValues({"Game Boy", "Game Boy Pocket", "RGB"});
      colorEmulationDMG->setDynamic(true);

      interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
        screen->setInterframeBlending(value);
      });
      interframeBlending->setDynamic(true);
    }

    if(Model::GameBoyColor()) {
      screen->colors(1 << 15, {&PPU::colorGameBoyColor, this});
      screen->setFillColor(0x7fff);

      colorEmulationCGB = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
        screen->resetPalette();
      });
      colorEmulationCGB->setDynamic(true);

      interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
        screen->setInterframeBlending(value);
      });
      interframeBlending->setDynamic(true);
    }
  }

  debugger.load(node);
}

auto PPU::unload() -> void {
  debugger = {};
  colorEmulationDMG.reset();
  colorEmulationCGB.reset();
  interframeBlending.reset();
  if(screen) screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
  vram.reset();
  oam.reset();
  bgp.reset();
  obp.reset();
  bgpd.reset();
  obpd.reset();
}

auto PPU::main() -> void {
  if(!status.displayEnable) {
    step(456 * 154);
    if(screen) screen->frame();
    scheduler.exit(Event::Frame);
    return;
  }

  status.lx = 0;

  latch.windowDisplayEnable = status.windowDisplayEnable;
  latch.wx = status.wx;

  if(status.ly == 0) {
    latch.wy = 0;
  }

  if(latch.displayEnable && status.ly == 0) {
    mode(0);
    step(72);

    mode(3);
    step(172);

    mode(0);
    cpu.hblank();
    step(456 - 8 - status.lx);
  } else if(status.ly <= 143) {
    mode(2);
    scanline();
    step(80);

    if(status.ly >= status.wy && status.wx < 7) latch.wy++;

    mode(3);
    for(auto n : range(160)) {
      run();
      step(1);
    }
    step(12);

    mode(0);
    cpu.hblank();
    step(456 - status.lx);
  } else {
    mode(1);
    step(456);
  }

  status.ly++;

  if(status.ly == 144) {
    cpu.raise(CPU::Interrupt::VerticalBlank);
    if(screen) screen->frame();
    scheduler.exit(Event::Frame);

    latch.displayEnable = 0;
  }

  if(status.ly == 154) {
    status.ly = 0;
  }
}

auto PPU::mode(n2 mode) -> void {
  if(status.mode == 0 && mode != 0) {
    if(Model::SuperGameBoy()) superGameBoy->ppuHreset();
  }

  if(status.mode == 1 && mode != 1) {
    if(Model::SuperGameBoy()) superGameBoy->ppuVreset();
  }

  status.mode = mode;
}

auto PPU::stat() -> void {
  if(!status.displayEnable) return;

  bool irq = status.irq;

  status.irq  = status.interruptHblank && status.mode == 0;
  status.irq |= status.interruptVblank && status.mode == 1;
  status.irq |= status.interruptOAM    && triggerOAM();
  status.irq |= status.interruptLYC    && compareLYC();

  if(!irq && status.irq) cpu.raise(CPU::Interrupt::Stat);
}

auto PPU::step(u32 clocks) -> void {
  while(clocks--) {
    history.mode = history.mode << 2 | status.mode;
    stat();
    if(status.dmaActive) {
      u32 hi = status.dmaClock++;
      u32 lo = hi & (cpu.status.speedDouble ? 1 : 3);
      hi >>= cpu.status.speedDouble ? 1 : 2;
      if(lo == 0) {
        if(hi == 0) {
          //warm-up
        } else if(hi == 161) {
          //cool-down; disable
          status.dmaActive = 0;
        } else {
          n8 bank = status.dmaBank;
          if(bank == 0xfe) bank = 0xde;  //OAM DMA cannot reference OAM, I/O, or HRAM:
          if(bank == 0xff) bank = 0xdf;  //it accesses HRAM instead.
          n8 data = bus.read(bank << 8 | hi - 1, 0xff);
          oam[hi - 1] = data;
        }
      }
    }

    status.lx++;
    Thread::step(1);
    Thread::synchronize(cpu);
  }
}

//flips 2bpp tiledata line horizontally
auto PPU::hflip(n16 tiledata) const -> n16 {
  return tiledata >> 7 & 0x0101
       | tiledata >> 5 & 0x0202
       | tiledata >> 3 & 0x0404
       | tiledata >> 1 & 0x0808
       | tiledata << 1 & 0x1010
       | tiledata << 3 & 0x2020
       | tiledata << 5 & 0x4040
       | tiledata << 7 & 0x8080;
}

auto PPU::power() -> void {
  Thread::create(4 * 1024 * 1024, {&PPU::main, this});
  if(screen) screen->power();

  if(Model::GameBoyColor()) {
    scanline = {&PPU::scanlineCGB, this};
    run = {&PPU::runCGB, this};
  } else {
    scanline = {&PPU::scanlineDMG, this};
    run = {&PPU::runDMG, this};
  }

  for(auto& n : vram) n = 0x00;
  for(auto& n : oam ) n = 0x00;
  for(auto& n : bgp ) n = 0;
  for(auto& n : obp ) n = 3;
  for(auto& n : bgpd) n = 0x0000;
  for(auto& n : obpd) n = 0x0000;

  status = {};
  latch = {};
  history = {};

  bg.color = 0;
  bg.palette = 0;
  bg.priority = 0;

  ob.color = 0;
  ob.palette = 0;
  ob.priority = 0;

  for(auto& s : sprite) s = {};
  sprites = 0;

  background = {};
  window = {};
}

}
