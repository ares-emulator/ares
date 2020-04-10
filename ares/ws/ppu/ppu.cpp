#include <ws/ws.hpp>

namespace ares::WonderSwan {

PPU ppu;
#include "io.cpp"
#include "latch.cpp"
#include "render.cpp"
#include "color.cpp"
#include "serialization.cpp"

auto PPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("PPU");

  screen = node->append<Node::Screen>("Screen");
  screen->colors(1 << 12, {&PPU::color, this});
  screen->setSize(224 + (SoC::ASWAN() ? 0 : 13), 144 + (Model::WonderSwan() ? 13 : 0));
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);

  colorEmulation = screen->append<Node::Boolean>("Color Emulation", true, [&](auto value) {
    screen->resetPalette();
  });
  colorEmulation->setDynamic(true);

  interframeBlending = screen->append<Node::Boolean>("Interframe Blending", true, [&](auto value) {
    screen->setInterframeBlending(value);
  });
  interframeBlending->setDynamic(true);

  if(!Model::PocketChallengeV2()) {
    orientation = screen->append<Node::String>("Orientation", "Automatic", [&](auto value) {
      updateOrientation();
    });
    orientation->setDynamic(true);
    orientation->setAllowedValues({"Automatic", "Horizontal", "Vertical"});

    showIcons = screen->append<Node::Boolean>("Show Icons", true, [&](auto value) {
      updateIcons();
    });
    showIcons->setDynamic(true);
  }

  if(!Model::PocketChallengeV2()) {
    icon.auxiliary0   = screen->append<Node::Sprite>("Auxiliary - Small Icon");
    icon.auxiliary1   = screen->append<Node::Sprite>("Auxiliary - Medium Icon");
    icon.auxiliary2   = screen->append<Node::Sprite>("Auxiliary - Large Icon");
    icon.headphones   = screen->append<Node::Sprite>("Headphones");
    icon.initialized  = screen->append<Node::Sprite>("Initialized");
    icon.lowBattery   = screen->append<Node::Sprite>("Low Battery");
    icon.orientation0 = screen->append<Node::Sprite>("Orientation - Horizontal");
    icon.orientation1 = screen->append<Node::Sprite>("Orientation - Vertical");
    icon.poweredOn    = screen->append<Node::Sprite>("Powered On");
    icon.sleeping     = screen->append<Node::Sprite>("Sleeping");
  }
  if(Model::WonderSwan()) {
    icon.volumeA0     = screen->append<Node::Sprite>("Volume - 0%");
    icon.volumeA1     = screen->append<Node::Sprite>("Volume - 50%");
    icon.volumeA2     = screen->append<Node::Sprite>("Volume - 100%");
  }
  if(!SoC::ASWAN()) {
    icon.volumeB0     = screen->append<Node::Sprite>("Volume - 0%");
    icon.volumeB1     = screen->append<Node::Sprite>("Volume - 33%");
    icon.volumeB2     = screen->append<Node::Sprite>("Volume - 66%");
    icon.volumeB3     = screen->append<Node::Sprite>("Volume - 100%");
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
  node = {};
  screen = {};
  colorEmulation = {};
  interframeBlending = {};
  orientation = {};
  showIcons = {};
  icon = {};
}

auto PPU::main() -> void {
  if(s.vtime == 142) {
    latchOAM();
  }

  if(s.vtime < 144) {
    uint y = s.vtime % (r.vtotal + 1);
    auto output = this->output + y * (224 + 13);
    latchRegisters();
    latchSprites(y);
    for(uint x : range(224)) {
      s.pixel = {Pixel::Source::Back, 0x000};
      if(r.lcdEnable) {
        renderBack();
        if(l.screenOneEnable) renderScreenOne(x, y);
        if(l.screenTwoEnable) renderScreenTwo(x, y);
        if(l.spriteEnable) renderSprite(x, y);
      }
      if(Model::WonderSwanColor() && r.lcdContrast == 1) {
        auto b = s.pixel.color.bit(0, 3);
        auto g = s.pixel.color.bit(4, 7);
        auto r = s.pixel.color.bit(8,11);
        //this is just a rough approximation, not accurate to the actual effect.
        //note that b,g,r are BitRange references, and modify the underlying s.pixel.color variable.
        b = min(15, b * 1.5);
        g = min(15, g * 1.5);
        r = min(15, r * 1.5);
      }
      *output++ = s.pixel.color;
      step(1);
    }
    step(32);
  } else {
    step(256);
  }
  scanline();
  if(r.htimerEnable && r.htimerCounter < r.htimerFrequency) {
    if(++r.htimerCounter == r.htimerFrequency) {
      if(r.htimerRepeat) {
        r.htimerCounter = 0;
      } else {
        r.htimerEnable = false;
      }
      cpu.raise(CPU::Interrupt::HblankTimer);
    }
  }
}

//vtotal+1 = scanlines per frame
//vtotal<143 inhibits vblank and repeats the screen image until vtime=144
//todo: unknown how votal<143 interferes with line compare interrupts
auto PPU::scanline() -> void {
  s.vtime++;
  if(s.vtime >= max(144, r.vtotal + 1)) return frame();
  if(s.vtime == r.lineCompare) cpu.raise(CPU::Interrupt::LineCompare);
  if(s.vtime == 144) {
    cpu.raise(CPU::Interrupt::Vblank);
    if(r.vtimerEnable && r.vtimerCounter < r.vtimerFrequency) {
      if(++r.vtimerCounter == r.vtimerFrequency) {
        if(r.vtimerRepeat) {
          r.vtimerCounter = 0;
        } else {
          r.vtimerEnable = false;
        }
        cpu.raise(CPU::Interrupt::VblankTimer);
      }
    }
  }
}

auto PPU::frame() -> void {
  s.field = !s.field;
  s.vtime = 0;
  scheduler.exit(Event::Frame);
}

auto PPU::refresh() -> void {
  uint width  = screen->width();
  uint height = screen->height();
  screen->refresh(output, (224 + 13) * sizeof(uint32), width, height);
}

auto PPU::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PPU::power() -> void {
  Thread::create(3'072'000, {&PPU::main, this});

  bus.map(this, 0x0000, 0x0017);
  bus.map(this, 0x001c, 0x003f);
  bus.map(this, 0x00a2);
  bus.map(this, 0x00a4, 0x00ab);

  auto color = SoC::ASWAN() ? 0xfff : 0x000;
  for(auto& pixel : output) pixel = color;
  s = {};
  l = {};
  r = {};
  updateIcons();
}

auto PPU::updateIcons() -> void {
  if(Model::PocketChallengeV2() || !showIcons) return;

  bool visible = showIcons->value();

  icon.poweredOn->setVisible(visible);

  icon.sleeping->setVisible(r.icon.sleeping & visible);
  icon.orientation1->setVisible(r.icon.orientation1 & visible);
  icon.orientation0->setVisible(r.icon.orientation0 & visible);
  icon.auxiliary0->setVisible(r.icon.auxiliary0 & visible);
  icon.auxiliary1->setVisible(r.icon.auxiliary1 & visible);
  icon.auxiliary2->setVisible(r.icon.auxiliary2 & visible);

  auto volume = apu.r.masterVolume;

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

  auto headphones = apu.r.headphonesConnected;

  icon.headphones->setVisible(headphones & visible);
}

auto PPU::updateOrientation() -> void {
  if(Model::PocketChallengeV2() || !orientation) return;

  if(r.icon.orientation1) l.orientation = 1;
  if(r.icon.orientation0) l.orientation = 0;

  auto orientation = this->orientation->value();
  if(orientation == "Horizontal" || (orientation == "Automatic" && l.orientation == 0)) {
    screen->setRotation(0);
  }
  if(orientation == "Vertical" || (orientation == "Automatic" && l.orientation == 1)) {
    screen->setRotation(90);
  }
}

}
