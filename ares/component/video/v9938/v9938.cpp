#include <ares/ares.hpp>
#include "v9938.hpp"

namespace ares {

#include "io.cpp"
#include "commands.cpp"
#include "graphic1.cpp"
#include "graphic2.cpp"
#include "graphic3.cpp"
#include "graphic4.cpp"
#include "sprite1.cpp"
#include "sprite2.cpp"
#include "serialization.cpp"

auto V9938::main() -> void {
  if(io.vcounter < vlines()) {
    uint9 y = io.vcounter;
    uint9 ycenter = y + (!overscan() ? 10 : 0);  //centers output within buffer
    uint32* lines[2] = {buffer + ycenter * 1024, buffer + ycenter * 1024 + 512};

    if(s1()) sprite1(y);
    if(s2()) sprite2(y);

    while(io.hcounter < 256) {
      uint9 x = io.hcounter;
      uint4 color;
      if(screen.enable) {
        color = io.colorBackground;
        if(g1()) graphic1(color, x, y);
        if(g2()) graphic2(color, x, y);
        if(g3()) graphic3(color, x, y);
        if(g4()) graphic4(color, x, y + screen.vscroll);
        if(s1()) sprite1(color, x, y);
        if(s2()) sprite2(color, x, y);
      }

      if(!interlace()) {
        *lines[0]++ = paletteRAM[color]; *lines[0]++ = paletteRAM[color];
        *lines[1]++ = paletteRAM[color]; *lines[1]++ = paletteRAM[color];
      } else {
        *lines[field()]++ = paletteRAM[color]; *lines[field()]++ = paletteRAM[color];
      }

      tick(1);
    }

    tick(200);
  } else {
    tick(456);
  }

  io.hcounter = 0;
  io.vcounter++;
  if(io.vcounter == vtotal()) {
    io.vcounter = 0;
    latch.timing = screen.timing;
    latch.overscan = screen.overscan;
    latch.interlace = screen.interlace;
    latch.field = !latch.field;
    if(!overscan()) {
      //clear border regions (in case the previous frame drew in the overscan area)
      for(uint y : range(212)) {
        if(y >= 10 && y < 202) continue;
        auto line = buffer + y * 1024;
        for(uint x : range(1024)) *line++ = 0;
      }
    }
  }
  if(io.vcounter == vlines()) virq.pending |= virq.enable, poll(), frame();
  if(io.vcounter == hirq.coincidence) hirq.pending |= hirq.enable, poll();
}

auto V9938::poll() -> void {
  irq(virq.pending || hirq.pending || lirq.pending);
}

auto V9938::tick(uint clocks) -> void {
  io.hcounter += clocks;
  while(clocks--) {
    command();
    step(1);
  }
}

auto V9938::power() -> void {
  //format: ggg-rrr-bbb (octal encoding)
  //the default palette is an approximation of the TMS9918 palette
  paletteRAM[ 0] = 0'000;
  paletteRAM[ 1] = 0'000;
  paletteRAM[ 2] = 0'611;
  paletteRAM[ 3] = 0'733;
  paletteRAM[ 4] = 0'117;
  paletteRAM[ 5] = 0'327;
  paletteRAM[ 6] = 0'151;
  paletteRAM[ 7] = 0'627;
  paletteRAM[ 8] = 0'171;
  paletteRAM[ 9] = 0'373;
  paletteRAM[10] = 0'661;
  paletteRAM[11] = 0'664;
  paletteRAM[12] = 0'411;
  paletteRAM[13] = 0'265;
  paletteRAM[14] = 0'555;
  paletteRAM[15] = 0'777;

  virq = {};
  hirq = {};
  lirq = {};
  latch = {};
  screen = {};
  table = {};
  sprite = {};
  io = {};
  op = {};
}

}
