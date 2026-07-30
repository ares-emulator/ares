auto PPU::Model::Raster::evaluatesSprites(u32 y) const -> bool {
  return y < 240 || (y >= spriteEvaluationStart && y <= spriteEvaluationEnd);
}

auto PPU::Model::Raster::restrictsOAM(u32 y) const -> bool {
  return evaluatesSprites(y) || y == scanlines - 1;
}

auto PPU::Model::Raster::pixelVisible(u32 x, u32 y) const -> bool {
  return y >= visibleTop && x >= visibleLeft && x <= visibleRight;
}

auto PPU::Model::Raster::backdrop(n9 color) const -> n9 {
  return fixedBackdrop ? backdropColor : color;
}

PPU::Model::Model(const Raster& raster) : raster(raster) {
}

auto PPU::Model::color(n9 input) const -> n64 {
  f64 saturation = 1.5;
  f64 hue = 0.0;
  f64 contrast = 1.0;
  f64 brightness = 1.0;
  f64 gamma = 2.2;

  i32 color = (input & 0x0f), level = color < 0xe ? int(input >> 4 & 3) : 1;

  static constexpr f64 black = 0.518, white = 1.962, attenuation = 0.746;
  static constexpr f64 levels[8] = {
    0.350, 0.518, 0.962, 1.550,
    1.094, 1.506, 1.962, 1.962,
  };

  f64 loAndHi[2] = {
    levels[level + 4 * (color == 0x0)],
    levels[level + 4 * (color <  0xd)],
  };

  f64 y = 0.0, i = 0.0, q = 0.0;
  auto wave = [](i32 phase, i32 color) { return (color + phase + 8) % 12 < 6; };
  for(auto phase : range(12)) {
    f64 spot = loAndHi[wave(phase, color)];

    if(((input & 0x040) && wave(phase, 12))
    || ((input & 0x080) && wave(phase,  4))
    || ((input & 0x100) && wave(phase,  8))
    ) spot *= attenuation;

    f64 value = (spot - black) / (white - black);
    value = (value - 0.5) * contrast + 0.5;
    value *= brightness / 12.0;

    y += value;
    i += value * cos((Math::Pi / 6.0) * (phase + hue));
    q += value * sin((Math::Pi / 6.0) * (phase + hue));
  }

  i *= saturation;
  q *= saturation;

  auto gammaAdjust = [=](f64 value) { return value < 0.0 ? 0.0 : pow(value, 2.2 / gamma); };
  n64 red   = uclamp<16>(65535.0 * gammaAdjust(y +  0.946882 * i +  0.623557 * q));
  n64 green = uclamp<16>(65535.0 * gammaAdjust(y + -0.274788 * i + -0.635691 * q));
  n64 blue  = uclamp<16>(65535.0 * gammaAdjust(y + -1.108545 * i +  1.709007 * q));

  return red << 32 | green << 16 | blue;
}

auto PPU::Model::mapWriteRegister(n3 address) const -> n3 {
  return address;
}

auto PPU::Model::status(n8 data) const -> n8 {
  return data;
}

namespace {

auto rp2c02Raster() -> PPU::Model::Raster {
  return {
    4, 262, 241, 242,
    16, 0, 256, 240, 16,
    1, 0,
    0, 0, 255,
    8.0, 7.0,
    true, false, 0
  };
}

auto rp2c07Raster() -> PPU::Model::Raster {
  return {
    5, 312, 241, 288,
    18, 0, 256, 240, 18,
    264, 310,
    1, 2, 253,
    55.0, 43.0,
    false, true, 0x3f
  };
}

auto ua6538Raster() -> PPU::Model::Raster {
  return {
    5, 312, 291, 242,
    16, 0, 256, 240, 18,
    1, 0,
    0, 0, 255,
    55.0, 43.0,
    false, false, 0
  };
}

struct RP2C02 final : PPU::Model {
  RP2C02() : Model(rp2c02Raster()) {}
};

struct RP2C07 final : PPU::Model {
  RP2C07() : Model(rp2c07Raster()) {}
};

struct UA6538 final : PPU::Model {
  UA6538() : Model(ua6538Raster()) {}
};

struct RGBModel : PPU::Model {
  RGBModel(const Palette& palette) : Model(rp2c02Raster()), palette(palette) {}

  auto color(n9 color) const -> n64 override {
    u32 address = (color & 0x3f) * 3;
    n64 red   = color.bit(6) ? 7 : palette[address + 0];
    n64 green = color.bit(7) ? 7 : palette[address + 1];
    n64 blue  = color.bit(8) ? 7 : palette[address + 2];
    return red * 0xffff / 7 << 32 | green * 0xffff / 7 << 16 | blue * 0xffff / 7;
  }

  const Palette palette;
};

struct RP2C04 final : RGBModel {
  RP2C04(const Palette& palette) : RGBModel(palette) {}
};

struct RC2C05 : RGBModel {
  RC2C05(const Palette& palette, n6 securityCode) : RGBModel(palette), securityCode(securityCode) {}

  auto mapWriteRegister(n3 address) const -> n3 override {
    if(address == 0) return 1;
    if(address == 1) return 0;
    return address;
  }

  auto status(n8 data) const -> n8 override {
    return data & 0xc0 | securityCode;
  }

  const n6 securityCode;
};

struct RC2C05_01 final : RC2C05 {
  RC2C05_01(const Palette& palette) : RC2C05(palette, 0x1b) {}
};

struct RC2C05_02 final : RC2C05 {
  RC2C05_02(const Palette& palette) : RC2C05(palette, 0x3d) {}
};

struct RC2C05_03 final : RC2C05 {
  RC2C05_03(const Palette& palette) : RC2C05(palette, 0x1c) {}
};

struct RC2C05_04 final : RC2C05 {
  RC2C05_04(const Palette& palette) : RC2C05(palette, 0x1b) {}
};

}

auto PPU::createBaseModel() -> std::unique_ptr<Model> {
  switch(system.region()) {
  case System::Region::NTSCJ: return std::make_unique<RP2C02>();
  case System::Region::NTSCU: return std::make_unique<RP2C02>();
  case System::Region::PAL:   return std::make_unique<RP2C07>();
  case System::Region::Dendy: return std::make_unique<UA6538>();
  }
  return std::make_unique<RP2C02>();
}

auto PPU::createVsModel(string identifier, const Model::Palette& palette) -> std::unique_ptr<Model> {
  if(identifier == "ppu2c04")    return std::make_unique<RP2C04>(palette);
  if(identifier == "ppu2c05_01") return std::make_unique<RC2C05_01>(palette);
  if(identifier == "ppu2c05_02") return std::make_unique<RC2C05_02>(palette);
  if(identifier == "ppu2c05_03") return std::make_unique<RC2C05_03>(palette);
  if(identifier == "ppu2c05_04") return std::make_unique<RC2C05_04>(palette);
  return {};
}

auto PPU::createVsModel() -> std::unique_ptr<Model> {
  if(!cartridge.pak) return {};

  auto paletteFile = cartridge.pak->read("palette.rom");
  if(!paletteFile || paletteFile->size() != Model::PaletteSize) return {};

  Model::Palette palette;
  paletteFile->read(palette.data(), palette.size());
  for(auto channel : palette) {
    if(channel > 7) return {};
  }

  return createVsModel(cartridge.pak->attribute("ppu"), palette);
}
