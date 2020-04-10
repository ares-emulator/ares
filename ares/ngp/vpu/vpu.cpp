#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

VPU vpu;
#include "memory.cpp"
#include "window.cpp"
#include "plane.cpp"
#include "sprite.cpp"
#include "color.cpp"
#include "serialization.cpp"

auto VPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("VPU");

  screen = node->append<Node::Screen>("Screen");
  if(Model::NeoGeoPocket()) screen->colors(1 << 3, {&VPU::colorNeoGeoPocket, this});
  if(Model::NeoGeoPocketColor()) screen->colors(1 << 12, {&VPU::colorNeoGeoPocketColor, this});
  screen->setSize(160, 152);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);

  interframeBlending = screen->append<Node::Boolean>("Interframe Blending", true, [&](auto value) {
    screen->setInterframeBlending(value);
  });
  interframeBlending->setDynamic(true);
}

auto VPU::unload() -> void {
  node = {};
  screen = {};
  interframeBlending = {};
}

auto VPU::main() -> void {
  if(io.vcounter < 152) {
    uint8 y = io.vcounter;
    auto output = buffer + y * 160;
    cacheSprites(y);
    for(uint8 x : range(160)) {
      bool validPlane1 = renderPlane(x, y, plane1);
      bool validPlane2 = renderPlane(x, y, plane2);
      bool validSprite = renderSprite(x);
      bool validWindow = renderWindow(x, y);

      uint12 color;
      //the dev manual says only background.mode = 0b10 enables background.color
      //Ogre Battle however sets 0b00 and expects the background color to be used
      //as such, it appears that only the lower bit of background.mode matters?
      if(Model::NeoGeoPocketColor() && !background.mode.bit(0)) {
        color = colors[0xf0 + background.color];
      }

      if(validSprite && sprite.priority == 1) color = sprite.output;
      if(validPlane1 && plane1.priority == 0) color = plane1.output;
      if(validPlane2 && plane2.priority == 0) color = plane2.output;
      if(validSprite && sprite.priority == 2) color = sprite.output;
      if(validPlane1 && plane1.priority == 1) color = plane1.output;
      if(validPlane2 && plane2.priority == 1) color = plane2.output;
      if(validSprite && sprite.priority == 3) color = sprite.output;
      if(validWindow) color = window.output;

      if(Model::NeoGeoPocket() && dac.negate) {
        color ^= 7;
      }

      output[x] = color;
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

auto VPU::step(uint clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

auto VPU::refresh() -> void {
  screen->refresh(buffer, 160 * sizeof(uint32), 160, 152);
}

auto VPU::power() -> void {
  Thread::create(system.frequency(), {&VPU::main, this});

  for(auto& p : colors) p = {};
  background = {};
  window = {};
  for(auto& a : attributes) a = {};
  for(auto& c : characters) for(auto& y : c) for(auto& x : y) x = 0;
  plane1 = {0x000, 0x40, 0xd0};
  plane2 = {0x800, 0x80, 0xe0};
  sprite = {0x00, 0xc0};
  for(auto& s : sprites) s = {};
  led = {};
  dac = {};
  io = {};
}

}
