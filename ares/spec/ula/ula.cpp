#include <spec/spec.hpp>

namespace ares::ZXSpectrum {

ULA ula;
#include "color.cpp"
#include "serialization.cpp"

auto ULA::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("ULA");

  screen = node->append<Node::Video::Screen>("Screen", 352, 296);
  screen->colors(16, std::bind_front(&ULA::color, this));
  screen->setSize(352, 296);
  screen->setScale(1.0, 1.0);
  screen->setAspect(1.0, 1.0);
  screen->setViewport(0, 0, screen->width(), screen->height());
  screen->refreshRateHint(system.frequency(), 224, 312);

  stream = node->append<Node::Audio::Stream>("Audio");
  stream->setChannels(1);
  stream->setFrequency(system.frequency());
}

auto ULA::unload() -> void {
  node = {};
  screen = {};
  stream = {};
}

auto ULA::main() -> void {
  // Audio is sampled at system frequency, not pixel frequency
  // So we sample on every even pixel
  if ((hcounter & 1) == 0) {
    stream->frame((0.75 * io.ear) + (0.25 * io.mic));
  }

  if (vcounter >= border_top_start && hcounter >= border_left_start) {
    const auto pixel = ((vcounter - border_top_start) * 352) + hcounter - border_left_start;
    auto buffer = screen->pixels().data();

    if (vcounter < screentop_start || vcounter >= border_bottom_start) {
      buffer[pixel] = io.borderColor;
    } else if (hcounter < screenleft_start || hcounter >= border_right_start) {
      buffer[pixel] = io.borderColor;
    } else {
      const n16 y = vcounter - screentop_start;
      const n16 x = hcounter - screenleft_start;

      const n8 x_tile = x / 8;

      n16 pixel_addr;
      pixel_addr.bit( 0,  4) = x_tile;
      pixel_addr.bit( 5,  7) = y.bit(3, 5);
      pixel_addr.bit( 8, 10) = y.bit(0, 2);
      pixel_addr.bit(11, 12) = y.bit(6, 7);
      pixel_addr.bit(13, 15) = 0x02;
      const n8 pixels = fetch(pixel_addr);

      n16 attr_addr;
      attr_addr.bit( 0,  4) = x_tile;
      attr_addr.bit( 5,  7) = y.bit(3, 5);
      attr_addr.bit( 8,  9) = y.bit(6, 7);
      attr_addr.bit(10, 15) = 0x16;
      const n8 attr = fetch(attr_addr);

      const auto ink   = attr.bit(0, 2);
      const auto paper = attr.bit(3, 5);
      const auto bright = attr.bit(6);
      const auto flash = attr.bit(7);

      const u1 use_ink = (pixels.bit(7 - (x & 7))) ^ (flash & flashState);
      buffer[pixel] = (use_ink ? ink : paper) + (bright * 8);
    }
  }
  step(1);

  // Increment pixel counters
  hcounter++;
  if (hcounter == border_right_end) {
    hcounter = 0;
    vcounter++;
    if (vcounter == border_bottom_end) {
      vcounter = 0;
    }
  }

  // IRQ happens on the cycle as active display starts - 64 lines
  if (hcounter == screenleft_start && vcounter == 0) {
    if (++flashFrameCounter == 31) {
      flashState ^= 1;
    }

    cpu.setIrq(true);
    frame();
  }

  // IRQ ends 64 pixel cycles (32 cpu cycles) from IRQ start
  if (hcounter == screenleft_start + 64 && vcounter == 0) {
    cpu.setIrq(false);
  }
}

auto ULA::step(uint clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto ULA::frame() -> void {
  screen->frame();
  scheduler.exit(Event::Frame);
}

auto ULA::power() -> void {
  Thread::create(system.frequency() * 2, [&] { ULA::main(); });
  hcounter = 0;
  vcounter = 0;
  flashFrameCounter = 0;
  flashState = 0;

  if (Model::Spectrum128()) {
    // Spectrum 128 has 311 lines per frame, rather than 312
    // Spectrum 128 has 456 pixel cycles per scanline, rather than 448
    border_top_start = 16;
    screentop_start = border_top_start + 48;
    border_bottom_start = screentop_start + 192;
    border_bottom_end = border_bottom_start + 55;
    border_left_start = 104;
    screenleft_start = border_left_start + 48;
    border_right_start = screenleft_start + 256;
    border_right_end = border_right_start + 48;
  }
}

auto ULA::fetch(n16 address) -> n8 {
  if (Model::Spectrum128()) {
    busValue = cpu.readBanked(system.screenBank ? 7 : 5, address - 0x4000);
    return busValue;
  }

  busValue = cpu.ram.read(address - 0x4000);
  return busValue;
}

auto ULA::in(n16 port) -> n8 {
  u5 keys = 0x1f;

  for (uint n : range(8)) {
    if (!port.bit(n + 8)) {
      keys &= keyboard.read(n);
    }
  }

  n8 value;
  value.bit(0, 4) = keys;
  value.bit(5) = 1;

  if (tapeDeck.playing()) {
    value.bit(6) = tapeDeck.read();
  } else {
    value.bit(6) = io.mic | io.ear;
  }

  value.bit(7) = 1;
  return value;
}

auto ULA::out(n8 data) -> void {
  io.borderColor = data.bit(0, 2);
  io.mic = data.bit(3);
  io.ear = data.bit(4);
}

}
