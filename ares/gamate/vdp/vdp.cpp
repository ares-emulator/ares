#include <gamate/gamate.hpp>

namespace ares::Gamate {

VDP vdp;
#include "color.cpp"
#include "serialization.cpp"

auto VDP::load(Node::Object parent) -> void {
  vram.allocate(16_KiB);

  node = parent->append<Node::Object>("VDP");

  screen = node->append<Node::Video::Screen>("Screen", width, height);
  screen->colors(1 << 2, std::bind_front(&VDP::color, this));
  screen->setSize(width, height);
  screen->setViewport(0, 0, width, height);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
  screen->refreshRateHint(CpuClock, cyclesPerLine, height);
}

auto VDP::unload() -> void {
  vram.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto VDP::main() -> void {
  scanline();
  step(cyclesPerLine);

  if(++vcounter >= height) {
    vcounter = 0;
    frame();
  }
}

auto VDP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

//the port is a single 13-bit word pointer shared by both planes; bit 7 of the
//x register picks which plane the next access lands on
auto VDP::vramAddress() const -> n16 {
  return io.address << 1 | io.plane;
}

auto VDP::increment() -> void {
  io.address = io.address + (io.incrementRow ? 0x20 : 0x01);
}

auto VDP::read(n3 address) -> n8 {
  if(address == 6) {
    auto data = vram.read(vramAddress());
    increment();
    return data;
  }
  return 0xff;  //every other register is write only
}

auto VDP::write(n3 address, n8 data) -> void {
  switch(address) {

  case 1:  //LCD control
    io.blank        = data.bit(7);
    io.incrementRow = data.bit(6);
    io.window       = data.bit(5);
    io.swapPlanes   = data.bit(4);
    //bit 0 stops the panel being refreshed at all, which damages real hardware
    return;

  case 2: io.scrollX = data; return;
  case 3: io.scrollY = data; return;

  case 4:  //plane select and the low five bits of the address
    io.plane = data.bit(7);
    io.address = io.address & 0x1fe0 | data.bit(0, 4);
    return;

  case 5:  //the upper eight bits of the address
    io.address = io.address & 0x001f | data << 5;
    return;

  case 7:
    vram.write(vramAddress(), data);
    increment();
    return;

  }
}

//two separate mechanisms hold the top of the screen still for a status bar: an
//explicit window bit, and a second one that engages on large Y scroll values
auto VDP::origin(u32 line, u32& x, u32& y) -> void {
  if(io.scrollY < 0xc8) {
    y = line + io.scrollY;
    if(y >= 0xc8) y -= 0xc8;
    x = io.scrollX;

    if(io.window && line < 0x10) {
      x = 0;
      y = 0xd0 + line;
    }
    return;
  }

  //scroll values with bit 3 set behave as though no scrolling were set at all
  if(io.scrollY.bit(3)) {
    x = io.scrollX;
    y = 0;
    return;
  }

  //otherwise the top one to eight lines come from VRAM rows f8-ff and do not
  //scroll horizontally. no released game is known to use this
  u32 fixed = io.scrollY & 0x07;
  if(line <= fixed) {
    x = 0;
    y = 0xf8 + line + (7 - fixed);
  } else {
    x = io.scrollX;
    y = line;
  }
}

auto VDP::pixel(u32 x, u32 y) -> n2 {
  x &= 0xff;
  y &= 0xff;

  u32 address = (y * 0x20 + (x >> 3)) << 1;
  n1 plane0 = vram.read(address + 0) >> (7 - (x & 7)) & 1;
  n1 plane1 = vram.read(address + 1) >> (7 - (x & 7)) & 1;

  if(!io.swapPlanes) return plane0 | plane1 << 1;
  return plane1 | plane0 << 1;
}

auto VDP::scanline() -> void {
  auto output = screen->pixels().data() + vcounter * width;

  u32 x, y;
  origin(vcounter, x, y);

  for(u32 n : range(width)) {
    *output++ = io.blank ? 0 : (u32)pixel(n + x, y);
  }
}

auto VDP::frame() -> void {
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto VDP::power() -> void {
  Thread::create(CpuClock, std::bind_front(&VDP::main, this));
  screen->power();

  io = {};
  io.scrollY = 10;  //the controller comes out of reset with a small Y scroll
  vcounter = 0;
  vram.fill(0x00);
}

}
