#include <a52/a52.hpp>

namespace ares::Atari5200 {

GTIA gtia;

#include "io.cpp"
#include "timing.cpp"
#include "console.cpp"
#include "player-missile.cpp"
#include "playfield.cpp"
#include "priority.cpp"
#include "collision.cpp"
#include "video.cpp"
#include "color.cpp"

auto GTIA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("GTIA");

  screen = node->append<Node::Video::Screen>(
    "Screen", Timing::SamplesPerScanline, Timing::ScanlinesPerFrame
  );
  screen->colors(1 << 8, std::bind_front(&GTIA::color, this));
  screen->setSize(Timing::SamplesPerScanline, Timing::ScanlinesPerFrame);
  screen->setScale(1.0, 1.0);
  screen->setAspect(20.0, 21.0);
  screen->refreshRateHint(system.frequency(), Timing::ColorClocksPerScanline, Timing::ScanlinesPerFrame);
}

auto GTIA::unload() -> void {
  if(screen) screen->quit();
  if(node && screen) node->remove(screen);
  screen.reset();
  node = {};
}

auto GTIA::power() -> void {
  if(screen) screen->power();
  playerMissile = {};
  priority = {};
  console.power();
  colors.power();
  collision.clear();
  playfield.power();
  counter.power();
  graphicsControl = 0;
}

}
