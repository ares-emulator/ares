#if defined(PROFILE_PERFORMANCE)
#include "../ppu-performance/ppu.cpp"
#else
#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

PPU ppu;
#include "main.cpp"
#include "io.cpp"
#include "mosaic.cpp"
#include "background.cpp"
#include "mode7.cpp"
#include "oam.cpp"
#include "object.cpp"
#include "window.cpp"
#include "dac.cpp"
#include "color.cpp"
#include "debugger.cpp"
#include "serialization.cpp"
#include "counter/serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PPU");

  screen = node->append<Node::Video::Screen>("Screen", 512, 480);
  screen->colors(1 << 19, {&PPU::color, this});
  screen->setSize(512, 480);
  screen->setScale(0.5, 0.5);
  screen->setAspect(8.0, 7.0);

  versionPPU1 = node->append<Node::Setting::Natural>("PPU1 Version", 1);
  versionPPU1->setAllowedValues({1});

  versionPPU2 = node->append<Node::Setting::Natural>("PPU2 Version", 3);
  versionPPU2->setAllowedValues({1, 2, 3});

  vramSize = node->append<Node::Setting::Natural>("VRAM", 64_KiB);
  vramSize->setAllowedValues({64_KiB, 128_KiB});

  overscanEnable = screen->append<Node::Setting::Boolean>("Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(512, 448);
    if(value == 1) screen->setSize(512, 480);
  });
  overscanEnable->setDynamic(true);

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  colorBleed = screen->append<Node::Setting::Boolean>("Color Bleed", true, [&](auto value) {
    screen->setColorBleed(value);
  });
  colorBleed->setDynamic(true);

  debugger.load(node);
}

auto PPU::unload() -> void {
  debugger.unload(node);
  versionPPU1.reset();
  versionPPU2.reset();
  vramSize.reset();
  overscanEnable.reset();
  colorEmulation.reset();
  colorBleed.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto PPU::map() -> void {
  function<n8   (n24, n8)> reader{&PPU::readIO, this};
  function<void (n24, n8)> writer{&PPU::writeIO, this};
  bus.map(reader, writer, "00-3f,80-bf:2100-213f");
}

inline auto PPU::step() -> void {
  tick(2);
  Thread::step(2);
  Thread::synchronize(cpu);
}

inline auto PPU::step(u32 clocks) -> void {
  clocks >>= 1;
  while(clocks--) {
    tick(2);
    Thread::step(2);
    Thread::synchronize(cpu);
  }
}

auto PPU::power(bool reset) -> void {
  Thread::create(system.cpuFrequency(), {&PPU::main, this});
  PPUcounter::reset();
  screen->power();

  if(!reset) random.array({vram.data, sizeof(vram.data)});

  ppu1.version = versionPPU1->value();
  ppu1.mdr = random.bias(0xff);

  ppu2.version = versionPPU2->value();
  ppu2.mdr = random.bias(0xff);

  vram.mask = vramSize->value() / sizeof(n16) - 1;
  if(vram.mask != 0xffff) vram.mask = 0x7fff;

  for(auto& object : oam.object) {
    object.x = 0;
    object.y = 0;
    object.character = 0;
    object.nameselect = 0;
    object.vflip = 0;
    object.hflip = 0;
    object.priority = 0;
    object.palette = 0;
    object.size = 0;
  }

  random.array({cgram, sizeof(cgram)});
  for(auto& word : cgram) word &= 0x7fff;

  latch.vram = random();
  latch.oam = random();
  latch.cgram = random();
  latch.bgofsPPU1 = random();
  latch.bgofsPPU2 = random();
  latch.mode7 = random();
  latch.counters = false;
  latch.hcounter = 0;
  latch.vcounter = 0;

  latch.oamAddress = 0x0000;
  latch.cgramAddress = 0x00;

  //$2100  INIDISP
  io.displayDisable = true;
  io.displayBrightness = 0;

  //$2102  OAMADDL
  //$2103  OAMADDH
  io.oamBaseAddress = random() & ~1;
  io.oamAddress = random();
  io.oamPriority = random();

  //$2105  BGMODE
  io.bgPriority = false;
  io.bgMode = 0;

  //$210d  BG1HOFS
  io.hoffsetMode7 = random();

  //$210e  BG1VOFS
  io.voffsetMode7 = random();

  //$2115  VMAIN
  io.vramIncrementMode = random.bias(1);
  io.vramMapping = random();
  io.vramIncrementSize = 1;

  //$2116  VMADDL
  //$2117  VMADDH
  io.vramAddress = random();

  //$211a  M7SEL
  io.repeatMode7 = random();
  io.vflipMode7 = random();
  io.hflipMode7 = random();

  //$211b  M7A
  io.m7a = random();

  //$211c  M7B
  io.m7b = random();

  //$211d  M7C
  io.m7c = random();

  //$211e  M7D
  io.m7d = random();

  //$211f  M7X
  io.m7x = random();

  //$2120  M7Y
  io.m7y = random();

  //$2121  CGADD
  io.cgramAddress = random();
  io.cgramAddressLatch = random();

  //$2133  SETINI
  io.extbg = random();
  io.pseudoHires = random();
  io.overscan = false;
  io.interlace = false;

  //$213c  OPHCT
  io.hcounter = 0;

  //$213d  OPVCT
  io.vcounter = 0;

  mosaic.power();
  bg1.power();
  bg2.power();
  bg3.power();
  bg4.power();
  obj.power();
  window.power();
  dac.power();

  updateVideoMode();
}

}
#endif
