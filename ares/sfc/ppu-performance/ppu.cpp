#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

PPU ppu;
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
#include "../ppu/counter/serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 512, 480);
  screen->colors(1 << 19, {&PPU::color, this});
  screen->setSize(512, 480);
  screen->setScale(0.5, 0.5);
  screen->setAspect(8.0, 7.0);

  overscanEnable = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(512, 448);
    if(value == 1) screen->setSize(512, 480);
  });
  overscanEnable->setDynamic(true);

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  debugger.load(node);
}

auto PPU::unload() -> void {
  debugger = {};
  overscanEnable.reset();
  colorEmulation.reset();
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
    state.interlace = io.interlace;
    state.overscan = io.overscan;
    obj.io.rangeOver = 0;
    obj.io.timeOver = 0;
    width256 = 0;
    width512 = 0;
  }

  if(vcounter() && vcounter() < vdisp() && !runAhead()) {
    u32 width = hires() ? 512 : 256;
    if(width == 256) width256 = 1;
    if(width == 512) width512 = 1;
    widths[vcounter()] = width;

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
    normalize();
    if(state.interlace == 0) screen->setProgressive(1);
    if(state.interlace == 1) screen->setInterlace(field());
    if(overscanEnable->value() == 0) screen->setViewport(0, 18, 256 << width512, 448);
    if(overscanEnable->value() == 1) screen->setViewport(0,  0, 256 << width512, 480);
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

  for(auto& word : vram.data) word = 0;

  state = {};
  latch = {};
  io = {};
  mode7 = {};
  window.power();
  mosaic.power();
  bg1.power();
  bg2.power();
  bg3.power();
  bg4.power();
  obj.power();
  dac.power();

  updateVideoMode();

  string title;
  for(u32 index : range(21)) {
    char byte = bus.read(0xffc0 + index, 0x00);
    if(byte == 0x00) break;
    if(byte == 0xff) break;
    title.append(byte);
  }
  title.strip();

  renderingCycle = 512;
  if(title == "ADVENTURES OF FRANKEN") renderingCycle = 32;
  if(title == "AIR STRIKE PATROL" || title == "DESERT FIGHTER") renderingCycle = 32;
  if(title == "FIREPOWER 2000" || title == "SUPER SWIV") renderingCycle = 32;
  if(title == "NHL '94" || title == "NHL PROHOCKEY'94") renderingCycle = 32;
  if(title == "Suguro Quest++") renderingCycle = 128;
}

auto PPU::normalize() -> void {
  if(width256 && width512) {
    //this frame contains mixed resolutions: normalize every scanline to 512-width
    for(u32 y : range(1, 240)) {
      auto line = screen->pixels().data() + 1024 * y + (interlace() && field() ? 512 : 0);
      if(widths[y] == 256) {
        auto source = &line[256];
        auto target = &line[512];
        for(u32 x : range(256)) {
          auto color = *--source;
          *--target = color;
          *--target = color;
        }
      }
    }
  }
}

}
