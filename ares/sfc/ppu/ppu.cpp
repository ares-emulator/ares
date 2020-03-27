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
#include "serialization.cpp"
#include "counter/serialization.cpp"

auto PPU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "PPU");
  from = Node::scan(parent = node, from);

  versionPPU1 = Node::append<Node::Natural>(parent, from, "PPU1 Version", 1);
  versionPPU1->setAllowedValues({1});

  versionPPU2 = Node::append<Node::Natural>(parent, from, "PPU2 Version", 3);
  versionPPU2->setAllowedValues({1, 2, 3});

  vramSize = Node::append<Node::Natural>(parent, from, "VRAM", 64_KiB);
  vramSize->setAllowedValues({64_KiB, 128_KiB});

  screen = Node::append<Node::Screen>(parent, from, "Screen");
  screen->colors(1 << 19, {&PPU::color, this});
  screen->setSize(512, 480);
  screen->setScale(0.5, 0.5);
  screen->setAspect(8.0, 7.0);
  from = Node::scan(parent = screen, from);

  overscanEnable = Node::append<Node::Boolean>(parent, from, "Overscan", true, [&](auto value) {
    if(value == 0) screen->setSize(512, 448);
    if(value == 1) screen->setSize(512, 480);
  });
  overscanEnable->setDynamic(true);

  colorEmulation = Node::append<Node::Boolean>(parent, from, "Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  colorBleed = Node::append<Node::Boolean>(parent, from, "Color Bleed", true, [&](auto value) {
    screen->setColorBleed(value);
  });
  colorBleed->setDynamic(true);

  debugVRAM = Node::append<Node::Memory>(parent, from, "PPU VRAM");
  debugVRAM->setSize(vram.mask + 1 << 1);
  debugVRAM->setRead([&](uint32 address) -> uint8 {
    return vram.data[address >> 1 & vram.mask].byte(address & 1);
  });

  debugOAM = Node::append<Node::Memory>(parent, from, "PPU OAM");
  debugOAM->setSize(512 + 32);
  debugOAM->setRead([&](uint32 address) -> uint8 {
    return obj.oam.read(address);
  });
  debugOAM->setWrite([&](uint32 address, uint8 data) -> void {
    return obj.oam.write(address, data);
  });

  debugCGRAM = Node::append<Node::Memory>(parent, from, "PPU CGRAM");
  debugCGRAM->setSize(256 << 1);
  debugCGRAM->setRead([&](uint32 address) -> uint8 {
    return dac.cgram[address >> 1 & 255].byte(address & 1);
  });
  debugCGRAM->setWrite([&](uint32 address, uint8 data) -> void {
    dac.cgram[address >> 1 & 255].byte(address & 1) = data;
  });

  output = new uint32[512 * 512];
  output += 16 * 512;  //overscan offset
}

auto PPU::unload() -> void {
  node = {};
  versionPPU1 = {};
  versionPPU2 = {};
  vramSize = {};
  screen = {};
  overscanEnable = {};
  colorEmulation = {};
  colorBleed = {};
  debugVRAM = {};
  debugOAM = {};
  debugCGRAM = {};

  output -= 16 * 512;
  delete[] output;
}

auto PPU::map() -> void {
  function<uint8 (uint24, uint8)> reader{&PPU::readIO, this};
  function<void (uint24, uint8)> writer{&PPU::writeIO, this};
  bus.map(reader, writer, "00-3f,80-bf:2100-213f");
}

auto PPU::step() -> void {
  tick(2);
  Thread::step(2);
  Thread::synchronize(cpu);
}

auto PPU::step(uint clocks) -> void {
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
  memory::fill<uint32>(output, 512 * 480);

  if(!reset) random.array((uint8*)vram.data, sizeof(vram.data));

  ppu1.version = versionPPU1->value();
  ppu1.mdr = random.bias(0xff);

  ppu2.version = versionPPU2->value();
  ppu2.mdr = random.bias(0xff);

  vram.mask = vramSize->value() / sizeof(uint16) - 1;
  if(vram.mask != 0xffff) vram.mask = 0x7fff;

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

auto PPU::refresh() -> void {
  auto data = output;

  if(overscanEnable->value() == 0) {
    data += 2 * 512;
    if(overscan()) data += 16 * 512;
    screen->refresh(data, 512 * sizeof(uint32), 512, 448);
  }

  if(overscanEnable->value() == 1) {
    if(!overscan()) data -= 14 * 512;
    screen->refresh(data, 512 * sizeof(uint32), 512, 480);
  }
}

}
#endif
