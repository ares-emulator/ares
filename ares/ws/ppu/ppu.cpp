#include <ws/ws.hpp>

namespace ares::WonderSwan {

PPU ppu;
#include "io.cpp"
#include "memory.cpp"
#include "window.cpp"
#include "screen.cpp"
#include "sprite.cpp"
#include "dac.cpp"
#include "timer.cpp"
#include "color.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PPU");

  //LCD display icons are simulated as an extended part of the LCD screen using sprites
  //this isn't ideal as the display icons are vectors, but it's the best we can do for now
  const u32 width  = 224 + (SoC::ASWAN() ? 0 : 13);
  const u32 height = 144 + (Model::WonderSwan() ? 13 : 0);

  screen = node->append<Node::Video::Screen>("Screen", width, height);
  screen->colors(1 << 12, {&PPU::color, this});
  screen->setSize(width, height);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
  screen->setFillColor(SoC::ASWAN() ? 0xfff : 0);

  colorEmulation = screen->append<Node::Setting::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  interframeBlending = screen->append<Node::Setting::Boolean>("Interframe Blending", true, [&](auto value) {
    screen->setInterframeBlending(value);
  });
  interframeBlending->setDynamic(true);

  if(!Model::PocketChallengeV2()) {
    orientation = screen->append<Node::Setting::String>("Orientation", "Automatic", [&](auto value) {
      updateOrientation();
    });
    orientation->setDynamic(true);
    orientation->setAllowedValues({"Automatic", "Horizontal", "Vertical"});

    showIcons = screen->append<Node::Setting::Boolean>("Show Icons", true, [&](auto value) {
      updateIcons();
    });
    showIcons->setDynamic(true);
  }

  if(!Model::PocketChallengeV2()) {
    icon.auxiliary0   = screen->append<Node::Video::Sprite>("Auxiliary - Small Icon");
    icon.auxiliary1   = screen->append<Node::Video::Sprite>("Auxiliary - Medium Icon");
    icon.auxiliary2   = screen->append<Node::Video::Sprite>("Auxiliary - Large Icon");
    icon.headphones   = screen->append<Node::Video::Sprite>("Headphones");
    icon.initialized  = screen->append<Node::Video::Sprite>("Initialized");
    icon.lowBattery   = screen->append<Node::Video::Sprite>("Low Battery");
    icon.orientation0 = screen->append<Node::Video::Sprite>("Orientation - Horizontal");
    icon.orientation1 = screen->append<Node::Video::Sprite>("Orientation - Vertical");
    icon.poweredOn    = screen->append<Node::Video::Sprite>("Powered On");
    icon.sleeping     = screen->append<Node::Video::Sprite>("Sleeping");
  }
  if(Model::WonderSwan()) {
    icon.volumeA0     = screen->append<Node::Video::Sprite>("Volume - 0%");
    icon.volumeA1     = screen->append<Node::Video::Sprite>("Volume - 50%");
    icon.volumeA2     = screen->append<Node::Video::Sprite>("Volume - 100%");
  }
  if(!SoC::ASWAN()) {
    icon.volumeB0     = screen->append<Node::Video::Sprite>("Volume - 0%");
    icon.volumeB1     = screen->append<Node::Video::Sprite>("Volume - 33%");
    icon.volumeB2     = screen->append<Node::Video::Sprite>("Volume - 66%");
    icon.volumeB3     = screen->append<Node::Video::Sprite>("Volume - 100%");
  }

  bool invert = SoC::ASWAN();

  if(!Model::PocketChallengeV2()) {
    icon.auxiliary0->setImage(Resource::Sprite::WonderSwan::Auxiliary0, invert);
    icon.auxiliary1->setImage(Resource::Sprite::WonderSwan::Auxiliary1, invert);
    icon.auxiliary2->setImage(Resource::Sprite::WonderSwan::Auxiliary2, invert);
    icon.headphones->setImage(Resource::Sprite::WonderSwan::Headphones, invert);
    icon.initialized->setImage(Resource::Sprite::WonderSwan::Initialized, invert);
    icon.lowBattery->setImage(Resource::Sprite::WonderSwan::LowBattery, invert);
    icon.orientation0->setImage(Resource::Sprite::WonderSwan::Orientation0, invert);
    icon.orientation1->setImage(Resource::Sprite::WonderSwan::Orientation1, invert);
    icon.poweredOn->setImage(Resource::Sprite::WonderSwan::PoweredOn, invert);
    icon.sleeping->setImage(Resource::Sprite::WonderSwan::Sleeping, invert);
  }
  if(Model::WonderSwan()) {
    icon.volumeA0->setImage(Resource::Sprite::WonderSwan::VolumeA0, invert);
    icon.volumeA1->setImage(Resource::Sprite::WonderSwan::VolumeA1, invert);
    icon.volumeA2->setImage(Resource::Sprite::WonderSwan::VolumeA2, invert);
  }
  if(!SoC::ASWAN()) {
    icon.volumeB0->setImage(Resource::Sprite::WonderSwan::VolumeB0, invert);
    icon.volumeB1->setImage(Resource::Sprite::WonderSwan::VolumeB1, invert);
    icon.volumeB2->setImage(Resource::Sprite::WonderSwan::VolumeB2, invert);
    icon.volumeB3->setImage(Resource::Sprite::WonderSwan::VolumeB3, invert);
  }

  if(Model::WonderSwan()) {
    icon.poweredOn->setPosition   (  0, 144);
    icon.initialized->setPosition ( 13, 144);
    icon.sleeping->setPosition    ( 26, 144);
    icon.lowBattery->setPosition  ( 39, 144);
    icon.volumeA2->setPosition    ( 52, 144);
    icon.volumeA1->setPosition    ( 52, 144);
    icon.volumeA0->setPosition    ( 52, 144);
    icon.headphones->setPosition  ( 65, 144);
    icon.orientation1->setPosition( 78, 144);
    icon.orientation0->setPosition( 91, 144);
    icon.auxiliary2->setPosition  (104, 144);
    icon.auxiliary1->setPosition  (117, 144);
    icon.auxiliary0->setPosition  (130, 144);
  }

  if(!SoC::ASWAN()) {
    icon.auxiliary0->setPosition  (224,   0);
    icon.auxiliary1->setPosition  (224,  13);
    icon.auxiliary2->setPosition  (224,  26);
    icon.orientation0->setPosition(224,  39);
    icon.orientation1->setPosition(224,  52);
    icon.headphones->setPosition  (224,  65);
    icon.volumeB0->setPosition    (224,  78);
    icon.volumeB1->setPosition    (224,  78);
    icon.volumeB2->setPosition    (224,  78);
    icon.volumeB3->setPosition    (224,  78);
    icon.lowBattery->setPosition  (224,  91);
    icon.sleeping->setPosition    (224, 104);
    icon.initialized->setPosition (224, 117);
    icon.poweredOn->setPosition   (224, 130);
  }

  if(!Model::PocketChallengeV2()) {
    screen->attach(icon.auxiliary0);
    screen->attach(icon.auxiliary1);
    screen->attach(icon.auxiliary2);
    screen->attach(icon.headphones);
    screen->attach(icon.initialized);
    screen->attach(icon.lowBattery);
    screen->attach(icon.orientation0);
    screen->attach(icon.orientation1);
    screen->attach(icon.poweredOn);
    screen->attach(icon.sleeping);
  }
  if(Model::WonderSwan()) {
    screen->attach(icon.volumeA0);
    screen->attach(icon.volumeA1);
    screen->attach(icon.volumeA2);
  }
  if(!SoC::ASWAN()) {
    screen->attach(icon.volumeB0);
    screen->attach(icon.volumeB1);
    screen->attach(icon.volumeB2);
    screen->attach(icon.volumeB3);
  }

  updateOrientation();
  updateIcons();
}

auto PPU::unload() -> void {
  icon = {};
  showIcons.reset();
  orientation.reset();
  interframeBlending.reset();
  colorEmulation.reset();
  screen->quit();
  node->remove(screen);
  screen.reset();
  node.reset();
}

auto PPU::main() -> void {
  if(io.vcounter == 142) {
    sprite.frame();
  }

  if(io.vcounter < 144) {
    n8 y = io.vcounter % (io.vtotal + 1);
    screen1.scanline(y);
    screen2.scanline(y);
    sprite.scanline(y);
    dac.scanline(y);
    for(n8 x : range(224)) {
      screen1.pixel(x, y);
      screen2.pixel(x, y);
      sprite.pixel(x, y);
      dac.pixel(x, y);
      step(1);
    }
    step(32);
  } else {
    step(256);
  }
  scanline();
  if(htimer.step()) cpu.raise(CPU::Interrupt::HblankTimer);
}

//vtotal+1 = scanlines per frame
//vtotal<143 inhibits vblank and repeats the screen image until vcounter=144
//todo: unknown how votal<143 interferes with vcompare interrupts
auto PPU::scanline() -> void {
  io.vcounter++;
  if(io.vcounter >= max(144, io.vtotal + 1)) return frame();
  if(io.vcounter == io.vcompare) cpu.raise(CPU::Interrupt::LineCompare);
  if(io.vcounter == 144) {
    cpu.raise(CPU::Interrupt::Vblank);
    if(vtimer.step()) cpu.raise(CPU::Interrupt::VblankTimer);
  }
}

auto PPU::frame() -> void {
  io.vcounter = 0;
  io.field = !io.field;
  screen->setViewport(0, 0, screen->width(), screen->height());
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto PPU::step(u32 clocks) -> void {
  io.hcounter += clocks;
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PPU::power() -> void {
  Thread::create(3'072'000, {&PPU::main, this});
  screen->power();

  bus.map(this, 0x0000, 0x0017);
  bus.map(this, 0x001c, 0x003f);
  bus.map(this, 0x00a2);
  bus.map(this, 0x00a4, 0x00ab);

  screen1.power();
  screen2.power();
  sprite.power();
  dac.power();
  pram = {};
  lcd = {};
  htimer = {};
  vtimer = {};
  io = {};
  updateIcons();
}

auto PPU::updateIcons() -> void {
  if(Model::PocketChallengeV2() || !showIcons) return;

  bool visible = showIcons->value();

  icon.poweredOn->setVisible(visible);

  icon.sleeping->setVisible(lcd.icon.sleeping & visible);
  icon.orientation1->setVisible(lcd.icon.orientation1 & visible);
  icon.orientation0->setVisible(lcd.icon.orientation0 & visible);
  icon.auxiliary0->setVisible(lcd.icon.auxiliary0 & visible);
  icon.auxiliary1->setVisible(lcd.icon.auxiliary1 & visible);
  icon.auxiliary2->setVisible(lcd.icon.auxiliary2 & visible);

  auto volume = apu.io.masterVolume;

  if(Model::WonderSwan()) {
    icon.volumeA0->setVisible(volume == 0 & visible);
    icon.volumeA1->setVisible(volume == 1 & visible);
    icon.volumeA2->setVisible(volume == 2 & visible);
  }

  if(Model::WonderSwanColor() || Model::SwanCrystal()) {
    icon.volumeB0->setVisible(volume == 0 & visible);
    icon.volumeB1->setVisible(volume == 1 & visible);
    icon.volumeB2->setVisible(volume == 2 & visible);
    icon.volumeB3->setVisible(volume == 3 & visible);
  }

  auto headphones = apu.io.headphonesConnected;

  icon.headphones->setVisible(headphones & visible);
}

auto PPU::updateOrientation() -> void {
  if(Model::PocketChallengeV2() || !orientation) return;

  if(lcd.icon.orientation1) io.orientation = 1;
  if(lcd.icon.orientation0) io.orientation = 0;

  auto orientation = this->orientation->value();
  if(orientation == "Horizontal" || (orientation == "Automatic" && io.orientation == 0)) {
    screen->setRotation(0);
  }
  if(orientation == "Vertical" || (orientation == "Automatic" && io.orientation == 1)) {
    screen->setRotation(90);
  }
}

}
