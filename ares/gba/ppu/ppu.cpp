#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

PPU ppu;
#include "background.cpp"
#include "object.cpp"
#include "window.cpp"
#include "dac.cpp"
#include "io.cpp"
#include "memory.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PPU::setAccurate(bool value) -> void {
  accurate = value;
}

auto PPU::load(Node::Object parent) -> void {
  vram.allocate(96_KiB);
  pram.allocate(512);

  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 240, 160);
  screen->colors(1 << 15, {&PPU::color, this});
  screen->setSize(240, 160);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
  screen->setViewport(0, 0, 240, 160);
  screen->refreshRateHint(system.frequency() / 4, 308, 228);

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
    screen->setInterframeBlending(value);
  });
  interframeBlending->setDynamic(true);

  rotation = screen->append<Node::Setting::String>("Orientation", "0°", [&](auto value) {
    if(value ==   "0°") screen->setRotation(  0);
    if(value ==  "90°") screen->setRotation( 90);
    if(value == "180°") screen->setRotation(180);
    if(value == "270°") screen->setRotation(270);
  });
  rotation->setDynamic(true);
  rotation->setAllowedValues({"0°", "90°", "180°", "270°"});

  debugger.load(node);
}

auto PPU::unload() -> void {
  debugger.unload(node);
  colorEmulation.reset();
  interframeBlending.reset();
  rotation.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
  vram.reset();
  pram.reset();
}

inline auto PPU::blank() -> bool {
  return io.forceBlank[0] || cpu.stopped();
}

auto PPU::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, display);
}

template<u32 Cycle>
auto PPU::cycleLinear(u32 x, u32 y) -> void {
  n3 mode = PPU::Background::IO::mode;
  if constexpr(Cycle == 0) if(mode <= 1) bg0.linear(x, y);
  if constexpr(Cycle == 1) if(mode <= 1) bg1.linear(x, y);
  if constexpr(Cycle == 2) if(mode == 0) bg2.linear(x, y);
  if constexpr(Cycle == 3) if(mode == 0) bg3.linear(x, y);
}

template<u32 Cycle>
auto PPU::cycleAffine(u32 x, u32 y) -> void {
  n3 mode = PPU::Background::IO::mode;
  if constexpr(Cycle == 0) if(             mode == 2) bg3.affineFetchTileMap(x, y);
  if constexpr(Cycle == 1) if(             mode == 2) bg3.affineFetchTileData(x, y);
  if constexpr(Cycle == 2) if(mode == 1 || mode == 2) bg2.affineFetchTileMap(x, y);
  if constexpr(Cycle == 3) if(mode == 1 || mode == 2) bg2.affineFetchTileData(x, y);
}

auto PPU::cycleBitmap(u32 x, u32 y) -> void {
  n3 mode = PPU::Background::IO::mode;
  if(mode >= 3 && mode <= 5) bg2.bitmap(x, y);
}

auto PPU::cycleUpperLayer(u32 x, u32 y) -> void {
  ppu.bg0.outputPixel(x, y);
  ppu.bg1.outputPixel(x, y);
  ppu.bg2.outputPixel(x, y);
  ppu.bg3.outputPixel(x, y);
  ppu.objects.outputPixel(x, y);
  window0.run(x, y);
  window1.run(x, y);
  window2.output = objects.output.window;
  window3.output = true;
  dac.upperLayer(x, y);
}

template<u32 Cycle>
auto PPU::cycle(u32 y) -> void {
  if constexpr(Cycle >= 31 && Cycle <= 1005                         ) cycleLinear<(Cycle - 31) & 3>((Cycle - 31) / 4, y);
  if constexpr(Cycle >= 31 && Cycle <= 1005                         ) cycleAffine<(Cycle - 31) & 3>((Cycle - 31) / 4, y);
  if constexpr(Cycle >= 31 && Cycle <= 1005 && (Cycle - 31) % 4 == 3) cycleBitmap((Cycle - 31) / 4, y);
  if constexpr(Cycle >= 46 && Cycle <= 1005 && (Cycle - 46) % 4 == 0) cycleUpperLayer((Cycle - 46) / 4, y);
  if constexpr(Cycle >= 46 && Cycle <= 1005 && (Cycle - 46) % 4 == 2) dac.lowerLayer((Cycle - 46) / 4, y);
  step(1);
  releaseBus();
}

auto PPU::main() -> void {
  if(display.io.vcounter == 0) {
    frame();

    bg2.io.lx = bg2.io.x;
    bg2.io.ly = bg2.io.y;

    bg3.io.lx = bg3.io.x;
    bg3.io.ly = bg3.io.y;
  }

  step(31);

  u32 y = display.io.vcounter;
  memory::move(io.forceBlank, io.forceBlank + 1, sizeof(io.forceBlank) - 1);
  memory::move(bg0.io.enable, bg0.io.enable + 1, sizeof(bg0.io.enable) - 1);
  memory::move(bg1.io.enable, bg1.io.enable + 1, sizeof(bg1.io.enable) - 1);
  memory::move(bg2.io.enable, bg2.io.enable + 1, sizeof(bg2.io.enable) - 1);
  memory::move(bg3.io.enable, bg3.io.enable + 1, sizeof(bg3.io.enable) - 1);
  memory::move(objects.io.enable, objects.io.enable + 1, sizeof(objects.io.enable) - 1);
  bg0.scanline(y);
  bg1.scanline(y);
  bg2.scanline(y);
  bg3.scanline(y);
  objects.scanline((y + 1) % 228);
  dac.scanline(y);

  if(y < 160) {
    if(accurate) {
      #define cycles01(index) cycle<index>(y)
      #define cycles02(index) cycles01(index); cycles01(index +  1)
      #define cycles04(index) cycles02(index); cycles02(index +  2)
      #define cycles08(index) cycles04(index); cycles04(index +  4)
      #define cycles16(index) cycles08(index); cycles08(index +  8)
      #define cycles32(index) cycles16(index); cycles16(index + 16)
      #define cycles64(index) cycles32(index); cycles32(index + 32)

      //cycle 31 - start rendering backgrounds
      cycles01( 31);
      cycles02( 32);
      cycles04( 34);
      cycles08( 38);

      //cycle 46 - start pixel output
      cycles64( 46);
      cycles64(110);
      cycles64(174);
      cycles64(238);
      cycles64(302);
      cycles64(366);
      cycles64(430);
      cycles64(494);
      cycles64(558);
      cycles64(622);
      cycles64(686);
      cycles64(750);
      cycles64(814);
      cycles64(878);
      cycles64(942);

      #undef cycles02
      #undef cycles04
      #undef cycles08
      #undef cycles16
      #undef cycles32
      #undef cycles64
    } else {
      for(u32 x : range(240)) {
        bg0.run(x, y);
        bg1.run(x, y);
        bg2.run(x, y);
        bg3.run(x, y);
        cycleUpperLayer(x, y);
        dac.lowerLayer(x, y);
      }
      releaseBus();
      step(975);
    }
  } else {
    step(975);
  }

  step(226);
}

auto PPU::frame() -> void {
  system.controls.poll();
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto PPU::power() -> void {
  Thread::create(system.frequency(), {&PPU::main, this});
  screen->power();

  for(u32 n = 0x000; n <= 0x055; n++) bus.io[n] = this;

  for(u32 n = 0; n < 96 * 1024; n++) vram[n] = 0x00;
  for(u32 n = 0; n < 1024; n += 2) writePRAM(Half, n, 0x0000);
  for(u32 n = 0; n < 1024; n += 2) writeOAM(Half, n, 0x0000);

  io = {};
  for(auto& object : this->object) object = {};
  for(auto& param : this->objectParam) param = {};

  bg0.power(BG0);
  bg1.power(BG1);
  bg2.power(BG2);
  bg3.power(BG3);
  objects.power();
  window0.power(IN0);
  window1.power(IN1);
  window2.power(IN2);
  window3.power(OUT);
  dac.power();
}

}
