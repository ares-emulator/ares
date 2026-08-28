#include <a26/a26.hpp>

namespace ares::Atari2600 {

Video video;

#include "color.cpp"
#include "receiver.cpp"
#include "serialization.cpp"

auto Video::load(Node::Object parent) -> void {
  node = parent;
  screen = node->append<Node::Video::Screen>("Screen", 180, displayHeight());
  screen->colors(1 << 7, std::bind_front(&Video::color, this));
  screen->setSize(180, displayHeight());
  screen->setScale(2.0, 1.0);
  Region::NTSC() ? screen->setAspect(4.0, 5.0) : screen->setAspect(38.0, 45.0);
  screen->setViewport(0, 0, 180, displayHeight());
}

auto Video::unload() -> void {
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto Video::power() -> void {
  screen->power();
  sync = Sync::Waiting;
  lineCounter = 0;
  linesSinceReturn = 0;
  pulseLines = 0;
}

}
