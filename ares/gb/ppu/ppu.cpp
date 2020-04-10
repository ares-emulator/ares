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

  node = parent->append<Node::Component>("PPU");

  if(Model::GameBoy() || Model::GameBoyColor()) {
    screen = node->append<Node::Screen>("Screen");

    if(Model::GameBoy()) {
      screen->colors(1 << 2, {&PPU::colorGameBoy, this});
      screen->setSize(160, 144);
      screen->setScale(1.0, 1.0);
      screen->setAspect(1.0, 1.0);

      colorEmulationDMG = screen->append<Node::String>("Color Emulation", "Game Boy", [&](auto value) {
        screen->resetPalette();
      });
      colorEmulationDMG->setAllowedValues({"Game Boy", "Game Boy Pocket", "RGB"});
      colorEmulationDMG->setDynamic(true);

      interframeBlending = screen->append<Node::Boolean>("Interframe Blending", true, [&](auto value) {
        screen->setInterframeBlending(value);
      });
      interframeBlending->setDynamic(true);
    }

    if(Model::GameBoyColor()) {
      screen->colors(1 << 15, {&PPU::colorGameBoyColor, this});
      screen->setSize(160, 144);
      screen->setScale(1.0, 1.0);
      screen->setAspect(1.0, 1.0);

      colorEmulationCGB = screen->append<Node::Boolean>("Color Emulation", true, [&](auto value) {
        screen->resetPalette();
      });
      colorEmulationCGB->setDynamic(true);

      interframeBlending = screen->append<Node::Boolean>("Interframe Blending", true, [&](auto value) {
        screen->setInterframeBlending(value);
      });
      interframeBlending->setDynamic(true);
    }
  }

  debugger.load(node);
}

auto PPU::unload() -> void {
  vram.reset();
  oam.reset();
  node = {};
  screen = {};
  colorEmulationDMG = {};
  colorEmulationCGB = {};
  interframeBlending = {};
  debugger = {};
}

auto PPU::main() -> void {
  if(!status.displayEnable) {
    for(uint n : range(160 * 144)) output[n] = Model::GameBoyColor() ? 0x7fff : 0;
    step(154 * 456);
    scheduler.exit(Event::Frame);
    return;
  }

  status.lx = 0;

  latch.windowDisplayEnable = status.windowDisplayEnable;
  latch.wx = status.wx;

  if(status.ly == 0) {
    latch.wy = 0;
  }

  if(latch.displayEnable) {
    latch.displayEnable = 0;
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
    scheduler.exit(Event::Frame);
  }

  if(status.ly == 154) {
    status.ly = 0;
  }
}

auto PPU::mode(uint2 mode) -> void {
  if(status.mode == 0 && mode != 0) {
    if(Model::SuperGameBoy()) superGameBoy->ppuHreset();
  }

  if(status.mode == 1 && mode != 1) {
    if(Model::SuperGameBoy()) superGameBoy->ppuVreset();
  }

  status.mode = mode;
}

auto PPU::stat() -> void {
  bool irq = status.irq;

  status.irq  = status.interruptHblank && status.mode == 0;
  status.irq |= status.interruptVblank && status.mode == 1;
  status.irq |= status.interruptOAM    && status.mode == 2;
  status.irq |= status.interruptLYC    && coincidence();

  if(!irq && status.irq) cpu.raise(CPU::Interrupt::Stat);
}

auto PPU::coincidence() -> bool {
  uint ly = status.ly;
  if(ly == 153 && status.lx >= 92) ly = 0;  //LYC=0 triggers early during LY=153
  return ly == status.lyc;
}

auto PPU::refresh() -> void {
  if(!Model::SuperGameBoy()) {
    screen->refresh(output, 160 * sizeof(uint32), 160, 144);
  }
}

auto PPU::step(uint clocks) -> void {
  while(clocks--) {
    history.mode = history.mode << 2 | status.mode;
    stat();
    if(status.dmaActive) {
      uint hi = status.dmaClock++;
      uint lo = hi & (cpu.status.speedDouble ? 1 : 3);
      hi >>= cpu.status.speedDouble ? 1 : 2;
      if(lo == 0) {
        if(hi == 0) {
          //warm-up
        } else if(hi == 161) {
          //cool-down; disable
          status.dmaActive = 0;
        } else {
          uint8 bank = status.dmaBank;
          if(bank == 0xfe) bank = 0xde;  //OAM DMA cannot reference OAM, I/O, or HRAM:
          if(bank == 0xff) bank = 0xdf;  //it accesses HRAM instead.
          uint8 data = bus.read(bank << 8 | hi - 1, 0xff);
          oam[hi - 1] = data;
        }
      }
    }

    status.lx++;
    Thread::step(1);
    Thread::synchronize(cpu);
  }
}

auto PPU::hflip(uint data) const -> uint {
  return (data & 0x8080) >> 7 | (data & 0x4040) >> 5
       | (data & 0x2020) >> 3 | (data & 0x1010) >> 1
       | (data & 0x0808) << 1 | (data & 0x0404) << 3
       | (data & 0x0202) << 5 | (data & 0x0101) << 7;
}

auto PPU::power() -> void {
  Thread::create(4 * 1024 * 1024, {&PPU::main, this});

  if(Model::GameBoyColor()) {
    scanline = {&PPU::scanlineCGB, this};
    run = {&PPU::runCGB, this};
  } else {
    scanline = {&PPU::scanlineDMG, this};
    run = {&PPU::runDMG, this};
  }

  for(auto& n : vram) n = 0x00;
  for(auto& n : oam) n = 0x00;
  for(auto& n : bgp) n = 0x00;
  for(auto& n : obp[0]) n = 3;
  for(auto& n : obp[1]) n = 3;
  for(auto& n : bgpd) n = 0x0000;
  for(auto& n : obpd) n = 0x0000;

  status = {};
  latch = {};
  history = {};

  for(auto& n : output) n = 0;

  bg.color = 0;
  bg.palette = 0;
  bg.priority = 0;

  ob.color = 0;
  ob.palette = 0;
  ob.priority = 0;

  for(auto& s : sprite) {
    s.x = 0;
    s.y = 0;
    s.tile = 0;
    s.attr = 0;
    s.data = 0;
  }
  sprites = 0;

  background.attr = 0;
  background.data = 0;

  window.attr = 0;
  window.data = 0;
}

}
