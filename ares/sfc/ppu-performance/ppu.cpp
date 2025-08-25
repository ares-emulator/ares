#include <sfc/sfc.hpp>

#undef ppu
#define PPU PPUPerformance
#define ppu ppuPerformanceImpl

namespace ares::SuperFamicom {

PPU ppuPerformanceImpl;
#include "io.cpp"
#include "window.cpp"
#include "mosaic.cpp"
#include "background.cpp"
#include "mode7.cpp"
#include "oam.cpp"
#include "object.cpp"
#include "dac.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 564, height() * 2);
  deepBlackBoost = screen->append<Node::Setting::Boolean>("Deep Black Boost", true, [&](auto value) {
    screen->resetPalette();
  });
  deepBlackBoost->setDynamic(true);
  screen->colors(1 << 19, {&PPU::color, this});
  screen->setSize(564, height() * 2);
  screen->setScale(0.5, 0.5);
  Region::PAL() ? screen->setAspect(55.0, 43.0) :screen->setAspect(8.0, 7.0);
  screen->refreshRateHint(system.cpuFrequency(), 1364, Region::PAL() ? 312 : 262);

  vramSize = node->append<Node::Setting::Natural>("VRAM", 64_KiB);
  vramSize->setAllowedValues({64_KiB, 128_KiB});

  debugger.load(node);
}

auto PPU::unload() -> void {
  debugger.unload(node);
  vramSize.reset();
  deepBlackBoost.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto PPU::step(u32 clocks) -> void {
  tick(clocks);
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PPU::main() -> void {
  if(vcounter() == 0) {
    state.interlace  = io.interlace;
    state.overscan   = io.overscan;
    obj.io.rangeOver = 0;
    obj.io.timeOver  = 0;
  }

  if(vcounter() && vcounter() < vdisp() && !runAhead()) {
    step(renderingCycle);
    mosaic.scanline();
    dac.prepare();
    if(!io.displayDisable) {
      bg1.render();
      bg2.render();
      bg3.render();
      bg4.render();
      obj.render();
    }
    dac.render();
  }

  if(vcounter() == vdisp()) {
    if(!io.displayDisable) obj.addressReset();
  }

  if(vcounter() == 240) {
    if(state.interlace == 0) screen->setProgressive(0);
    if(state.interlace == 1) screen->setInterlace(field());
    auto yScale = state.interlace ? 2 : 1;
    screen->setScale(0.5, 1.0 / yScale);

    if(screen->overscan()) {
      screen->setSize(564, height() * yScale);
      screen->setViewport(0, 0, 564, height() * yScale);
    } else {
      int x = 26;
      int y = 9 * yScale;
      int w = 564 - 52;
      int h = height() - 18;

      if(Region::PAL()) {
        x -= 4;
        y += 12 * yScale;
        h -= 31;

        if(!io.overscan) {
          y += 8 * yScale;
          h -= 15;
        }
      }

      screen->setSize(w, h * yScale);
      screen->setViewport(x, y, w, h * yScale);
    }

    screen->frame();
    scheduler.exit(Event::Frame);
  }

  step(hperiod() - hcounter());
}

auto PPU::map() -> void {
  function<n8   (n24, n8)> reader{&PPU::readIO, this};
  function<void (n24, n8)> writer{&PPU::writeIO, this};
  bus.map(reader, writer, "00-3f,80-bf:2100-213f");
}

auto PPU::power(bool reset) -> void {
  Thread::create(system.cpuFrequency(), {&PPU::main, this});
  PPUcounter::reset();
  screen->power();

  ppu1.version = 1, ppu1.mdr = 0x00;
  ppu2.version = 3, ppu2.mdr = 0x00;

  if(!reset) for(auto& word : vram.data) word = 0;
  vram.mask = vramSize->value() / sizeof(n16) - 1;
  if(vram.mask != 0xffff) vram.mask = 0x7fff;

  state = {};
  latch = {};
  io = {};
  mode7 = {};
  
  if(!reset) {
    window.power();
    mosaic.power();
    bg1.power();
    bg2.power();
    bg3.power();
    bg4.power();
    obj.power();
    dac.power();
  }
  
  updateVideoMode();

  string title;
  for(u32 index : range(21)) {
    auto byte = bus.read(0xffc0 + index, 0x00);
    if(byte == 0x00) break;
    if(byte == 0xff) break;
    title.append((char)byte);
  }
  title.strip();

  renderingCycle = 512;
  if(title == "ADVENTURES OF FRANKEN") renderingCycle = 32;
  if(title == "AIR STRIKE PATROL" || title == "DESERT FIGHTER") renderingCycle = 32;
  if(title == "FIREPOWER 2000" || title == "SUPER SWIV") renderingCycle = 32;
  if(title == "NHL '94" || title == "NHL PROHOCKEY'94") renderingCycle = 32;
  if(title == "Sugoro Quest++") renderingCycle = 128;
}

}
