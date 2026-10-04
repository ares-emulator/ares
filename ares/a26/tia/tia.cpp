#include <a26/a26.hpp>

namespace ares::Atari2600 {

TIA tia;

#include "analog.cpp"
#include "audio.cpp"
#include "collision.cpp"
#include "io.cpp"
#include "playfield.cpp"
#include "player.cpp"
#include "missile.cpp"
#include "ball.cpp"
#include "objects.cpp"
#include "priority.cpp"
#include "serialization.cpp"
#include "timing.cpp"
#include "trigger.cpp"
#include "write-queue.cpp"

auto TIA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("TIA");
  audio.load(node, system.frequency());
}

auto TIA::unload() -> void {
  audio.unload(node);
  node.reset();
}

auto TIA::main() -> void {
  scanline();
  timing.extendedHblank = 0;
}

auto TIA::scanline() -> void {
  cpu.io.scanlineCycles = 0;

  while(true) {
    bool lineEnded = clock();
    step();
    if(lineEnded) break;
  }
  video.endScanline();
}

auto TIA::clock() -> bool {
  clockWrites();
  auto position = timing.position();
  auto x = position - 68;
  auto hblank = timing.horizontalBlank();

  objects.runMovement(hblank, timing.hcounter);

  auto signals = objects.clock(hblank, timing.hcounter);

  auto pf = playfield.clock(x);
  auto pixel = priority.resolveColor(priority.resolveSource(pf, signals), x);

  collision.clock(playfield.pixel, signals, vblank);

  video.clock(x, pixel, hblank, vblank);

  audio.clock();
  analog.advance();
  controllerPort1.clock();
  controllerPort2.clock();

  if(!timing.advance()) return false;
  playfield.nextLine();
  cpu.rdyLine(1);
  return true;
}

auto TIA::objectResetCounter() const -> u8 {
  //Stella uses these phases; MAME confirms the visible +5/+4 offsets.
  if(!timing.horizontalBlank()) return 157;
  auto position = timing.position();
  return position >= 73 ? 158 : 159;
}

auto TIA::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto TIA::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&TIA::main, this));
  timing = {};
  vsync = 0;
  vblank = 0;
  triggers.power();
  analog.power(system.frequency());
  for(auto index : range(4)) updateAnalogInput(index);
  playfield = {};
  objects.power();
  priority = {};
  collision.power();
  for(auto& write : writes) write = {};
  audio.power();
}

}
