#include <a26/a26.hpp>

namespace ares::Atari2600 {

TIA tia;

#include "audio.cpp"
#include "color.cpp"
#include "io.cpp"
#include "serialization.cpp"
#include "write-queue.cpp"

auto TIA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("TIA");

  screen = node->append<Node::Video::Screen>("Screen", 160, vlines());
  screen->colors(1 << 7, {&TIA::color, this});
  screen->setSize(160, displayHeight());
  screen->setScale(1.0, 1.0);
  screen->setAspect(12.0, 7.0);
  screen->setViewport(0, 0, 160, displayHeight());

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
  io.hmoveTriggered = 255;
}

auto TIA::scanline() -> void {
  cpu.io.scanlineCycles = 0;

  for(io.hcounter = 0; io.hcounter < 228; io.hcounter++) {
    writeQueue.step();
    auto x = io.hcounter - 68;
    if(x >= 0 && io.vcounter < vlines()) pixel(x);
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

auto TIA::pixel(n8 x) -> void {
  auto output = screen->pixels().data() + (io.vcounter * 160);

  // Output only black during vblank, or for the first 8px of a scanline where hmove was triggered
  if (io.vblank || (io.hmoveTriggered == io.vcounter && x < 8)) {
    output[x] = 0;
    return;
  }

  output[x] = io.bgColor;
  auto pf = runPlayfield(x);
  auto bl = runBall(x);
  auto p0 = runPlayer(x, 0);
  auto p1 = runPlayer(x, 1);
  auto m0 = runMissile(x, 0);
  auto m1 = runMissile(x, 1);

  if(!playfield.priority && (pf || bl)) output[x] = io.fgColor;
  if(p1 || m1)                          output[x] = io.p1Color;
  if(p0 || m0)                          output[x] = io.p0Color;
  if(playfield.priority && (pf || bl))  output[x] = io.fgColor;

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
}

auto TIA::runPlayfield(n8 x) -> n1 {
  if((x % 4) == 0) {
    auto pos = x >> 2;
    playfield.pixel = (!playfield.mirror || pos < 20) ? playfield.graphics.bit(pos % 20)
                                                      : playfield.graphics.bit(19 - (pos % 20));
  }

  return playfield.pixel;
}

auto TIA::runPlayer(n8 x, n1 index) -> n1 {
  auto& player = this->player[index];
  auto position = player.position;

  // Handle player stretch capability
  auto width = 8;
  if(player.size == 5) width = 16;
  if(player.size == 7) width = 32;

  // Handle repeat capability
  auto repeat = 1;
  if(player.size == 1 || player.size == 2 || player.size == 4) repeat = 2;
  if(player.size == 3 || player.size == 6)                     repeat = 3;

  auto spacing = 8;
  if(player.size == 2 || player.size == 6) spacing = 24;
  if(player.size == 4                    ) spacing = 56;

  for(int i = 0; i < repeat; i++) {
    if (x >= position && x < position + width) {
      auto bit = player.reflect ? (x - position) / (width / 8) : 7 - ((x - position) / (width / 8));
      return player.graphics[player.delay].bit(bit);
    }

    position = (position + (spacing + width)) % 160;
  }

  return 0;
}

auto TIA::runMissile(n8 x, n1 index) -> n1 {
  auto& missile = this->missile[index];
  auto& player = this->player[index];
  auto position = missile.position;

  if(missile.reset) {
    auto offset = 3;
    if(player.size == 5) offset = 6;
    if(player.size == 7) offset = 10;
    missile.position = player.position + offset;
    return 0;
  }

  if (!missile.enable) return 0;

  const int missileSizes[4] = {1, 2, 4, 8};

  // Handle player stretch capability
  auto width = missileSizes[missile.size];
  if(player.size == 5) width *= 2;
  if(player.size == 7) width *= 4;

  // Handle repeat capability
  auto repeat = 1;
  if(player.size == 1 || player.size == 2 || player.size == 4) repeat = 2;
  if(player.size == 3 || player.size == 6)                     repeat = 3;

  auto spacing = 8;
  if(player.size == 2 || player.size == 6) spacing = 24;
  if(player.size == 4                    ) spacing = 56;

  for(int i = 0; i < repeat; i++) {
    if (x >= position && x < position + width) {
      auto bit = player.reflect ? (x - position) / (width / 8) : 7 - ((x - position) / (width / 8));
      return 1;
    }

    position = (position + (spacing + width)) % 160;
  }

  return 0;
}

auto TIA::runBall(n8 x) -> n1 {
  if(!ball.enable[ball.delay]) return 0;

  const int ballSizes[4] = {1, 2, 4, 8};
  return x >= ball.position && x < ball.position + ballSizes[ball.size];
}

auto TIA::frame() -> void {
  auto height = io.vcounter;

  if(height <= 0) {
     debug(unusual, "invalid screen height");
     height = vlines();
  }

  screen->setViewport(0, 0, 160, height);
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
  for(auto& p : player)  p = {};
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