#include <a26/a26.hpp>

namespace ares::Atari2600 {

TIA tia;

#include "audio.cpp"
#include "color.cpp"
#include "io.cpp"
#include "playfield.cpp"
#include "player.cpp"
#include "missile.cpp"
#include "ball.cpp"
#include "serialization.cpp"
#include "write-queue.cpp"

auto TIA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("TIA");

  screen = node->append<Node::Video::Screen>("Screen", 180, displayHeight());
  screen->colors(1 << 7, {&TIA::color, this});
  screen->setSize(180, displayHeight());
  screen->setScale(2.0, 1.0);
  Region::PAL() ? screen->setAspect(38.0, 45.0) : screen->setAspect(4.0, 5.0);
  screen->setViewport(0, 0, 180, displayHeight());

  stream = node->append<Node::Audio::Stream>("Audio");
  stream->setChannels(1);
  stream->setFrequency(31'403);
  stream->addHighPassFilter(  20.0, 1);
  stream->addLowPassFilter (2840.0, 1);
}

auto TIA::unload() -> void {
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto TIA::main() -> void {
  scanline();
  io.vcounter++;
  io.hmoveTriggered = 0;
}

auto TIA::scanline() -> void {
  cpu.io.scanlineCycles = 0;

  for(io.hcounter = 0; io.hcounter < 228; io.hcounter++) {
    writeQueue.step();
    auto x = io.hcounter - 68;
    auto y = io.vcounter - voffset();
    bool hblank = (io.hmoveTriggered && io.hcounter < 76) || io.hcounter < 68;

    // Playfield ignores hmove and always begins at cycle 68
    //if(io.hcounter >= 68) playfield.step();

    // Objects are stepped only during active display
    if(!hblank) {
      ball.step();
      for(auto& player : this->player) player.step();
      for(auto& missile : this->missile) missile.step();
    }

    n8 pixel = io.bgColor;
    auto pf = runPlayfield(x);
    auto bl = ball.output;
    auto p0 = player[0].output;
    auto p1 = player[1].output;
    auto m0 = missile[0].output;
    auto m1 = missile[1].output;

    if(!playfield.priority && (pf || bl)) pixel = io.fgColor;
    if(p1 || m1)                          pixel = io.p1Color;
    if(p0 || m0)                          pixel = io.p0Color;
    if(playfield.priority && (pf || bl))  pixel = io.fgColor;

    // Update Collision
    if(m0 && p1) collision.M0P1 = 1;
    if(m0 && p0) collision.M0P0 = 1;
    if(m1 && p0) collision.M1P0 = 1;
    if(m1 && p1) collision.M1P1 = 1;
    if(p0 && pf) collision.P0PF = 1;
    if(p0 && bl) collision.P0BL = 1;
    if(p1 && pf) collision.P1PF = 1;
    if(p1 && bl) collision.P1BL = 1;
    if(m0 && pf) collision.M0PF = 1;
    if(m0 && bl) collision.M0BL = 1;
    if(m1 && pf) collision.M1PF = 1;
    if(m1 && bl) collision.M1BL = 1;
    if(bl && pf) collision.BLPF = 1;
    if(p0 && p1) collision.P0P1 = 1;
    if(m0 && m1) collision.M0M1 = 1;

    if(x >= 0 && y > 0 && y < displayHeight()) {
      if(io.vblank || hblank) pixel = 0;
      auto output = screen->pixels().data() + (y * 180) + 10;
      output[x] = pixel;
    }

    runAudio();
    step();
    if(io.hcounter == 0) cpu.io.rdyLine = 1;
  }

  // Prevent an infinite emulator hang when games miss vblank
  if(io.vcounter > vlines()) {
    scheduler.exit(Event::Frame);
    io.vcounter = 0;
  }
}

auto TIA::frame() -> void {
  screen->refreshRateHint(system.frequency(), 228, io.vcounter);
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto TIA::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto TIA::power(bool reset) -> void {
  Thread::create(system.frequency(), {&TIA::main, this});
  screen->power();
  io = {};
  playfield = {};
  for(auto& p : player)  {
    p.graphics[0] = p.graphics[1] = 0;
    p.reflect = 0;
    p.size = 0;
    p.offset = 0;
    p.delay = 0;
    p.counter = 0;
    p.startCounter = 0;
    p.pixelCounter = 0;
    p.widthCounter = 0;
    p.starting = 0;
    p.output = 0;
    p.copy = 0;
  };
  for(auto& m : missile) m = {};
  ball = {};
  collision = {};
  for(auto n : range(writeQueue.maxItems)) writeQueue.items[n] = {};
  audio[0] = {}; audio[1] = {};
  for(u32 level : range(15)) {
    volume[level] = pow(2, level * -2.0 / 6.0);
  }
  volume[15] = 0;
}

}
