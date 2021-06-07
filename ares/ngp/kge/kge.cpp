#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

KGE kge;
#include "memory.cpp"
#include "window.cpp"
#include "plane.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "color.cpp"
#include "serialization.cpp"

auto KGE::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("KGE");

  screen = node->append<Node::Video::Screen>("Screen", 160, 152);
  if(Model::NeoGeoPocket()) screen->colors(1 << 3, {&KGE::colorNeoGeoPocket, this});
  if(Model::NeoGeoPocketColor()) screen->colors(1 << 12, {&KGE::colorNeoGeoPocketColor, this});
  screen->setSize(160, 152);
  screen->setViewport(0, 0, 160, 152);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);

  interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
    screen->setInterframeBlending(value);
  });
  interframeBlending->setDynamic(true);
}

auto KGE::unload() -> void {
  interframeBlending.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto KGE::main() -> void {
  if(io.vcounter < 152) {
    n8 y = io.vcounter;
    dac.begin(y);
    sprite.begin(y);
    for(n8 x : range(160)) {
      dac.run(x, y);
      io.hcounter += 3;
      step(3);
    }
    if(io.vcounter <= 150) {
      io.hblankActive = 1;
      cpu.ti0 = !io.hblankEnableIRQ;
    }
  }

  step(515 - io.hcounter);
  io.hcounter = 0;
  io.hblankActive = 0;
  cpu.ti0 = 1;

  io.vcounter++;
  if(io.vcounter == 152) {
    io.vblankActive = 1;
    cpu.int4.set(!io.vblankEnableIRQ);
    screen->frame();
    scheduler.exit(Event::Frame);
  }
  if(io.vcounter == io.vlines) {
    io.hblankActive = 1;
    cpu.ti0 = !io.hblankEnableIRQ;
  }
  if(io.vcounter > io.vlines) {
    io.vcounter = 0;
    io.vblankActive = 0;
    io.characterOver = 0;
    cpu.int4.set(1);
  }

  //note: this is not the most intuitive place to call this,
  //but calling after every CPU instruction is too demanding
  cpu.pollPowerButton();
}

auto KGE::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

auto KGE::power() -> void {
  Thread::create(system.frequency(), {&KGE::main, this});
  screen->power();

  window.power();
  plane1.power();
  plane2.power();
  sprite.power();
  dac.power();
  for(auto& attribute : attributes) attribute = {};
  for(auto& character : characters) for(auto& y : character) for(auto& x : y) x = 0;
  background = {};
  led = {};
  io = {};
}

}
