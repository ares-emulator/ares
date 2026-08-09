auto GTIA::ColorRegisters::power() -> void {
  for(auto& color : playerColor) color = 0;
  for(auto& color : playfieldColor) color = 0;
  backgroundColor = 0;
}

auto GTIA::ColorRegisters::clock() -> void {
  for(u32 index = 0; index < 9; index++) {
    if(!colorDelay[index]) continue;
    colorDelay[index] = 0;
    if(index < 4) playerColor[index] = pendingColor[index];
    else if(index < 8) playfieldColor[index - 4] = pendingColor[index];
    else backgroundColor = pendingColor[index];
  }
}

auto GTIA::ColorRegisters::write(u8 index, n8 data) -> void {
  if(index >= 9) return;
  pendingColor[index] = data;
  colorDelay[index] = 1;
}

auto GTIA::ColorRegisters::player(u8 index) const -> n8 {
  return playerColor[index];
}

auto GTIA::ColorRegisters::playfield(u8 index) const -> n8 {
  return playfieldColor[index];
}

auto GTIA::ColorRegisters::background() const -> n8 {
  return backgroundColor;
}

auto GTIA::color(n32 color) -> n64 {
  static constexpr f64 LuminanceResistorsKOhm[4] = {39.0, 20.0, 10.0, 5.1};
  static constexpr f64 MaximumLuminanceConductance =
    1.0 / LuminanceResistorsKOhm[0] + 1.0 / LuminanceResistorsKOhm[1]
    + 1.0 / LuminanceResistorsKOhm[2] + 1.0 / LuminanceResistorsKOhm[3];
  static constexpr f64 BlackSetupIRE = 7.5;
  static constexpr f64 WhiteIRE = 100.0;
  static constexpr f64 ColorBurstPhaseDegrees = -57.0;
  static constexpr f64 HuePhaseStepDegrees = 24.0;
  static constexpr f64 ChromaAmplitude = 0.20;

  auto code = (u8)color;
  auto hue = code >> 4;
  auto clampColorChannel = [](f64 value) -> u64 {
    return (u64)(std::clamp(value, 0.0, 1.0) * 65535.0 + 0.5);
  };

  f64 conductance = 0.0;
  for(u32 bit = 0; bit < 4; bit++) {
    if(code >> bit & 1) conductance += 1.0 / LuminanceResistorsKOhm[bit];
  }

  // GTIA exposes separate hue and four-bit luminance signals. The CX5200
  // motherboard combines the luminance bits through this resistor family,
  // while luminance zero sits at blanking below the NTSC black setup.
  auto luminance = conductance / MaximumLuminanceConductance;
  auto y = (luminance * WhiteIRE - BlackSetupIRE) / (WhiteIRE - BlackSetupIRE);
  f64 i = 0.0;
  f64 q = 0.0;

  if(hue) {
    // Hue zero suppresses chroma. The remaining fifteen hues are successive
    // nominal GTIA delay stages measured from calibrated NTSC color burst.
    auto phase = (ColorBurstPhaseDegrees + HuePhaseStepDegrees * (hue - 1)) * Math::Pi / 180.0;
    i = ChromaAmplitude * cos(phase);
    q = ChromaAmplitude * sin(phase);
  }

  // Decode gamma-domain YIQ and clamp only after the matrix. This preserves
  // the component clipping that shifts very dark GTIA colors.
  auto red = clampColorChannel(y + 0.956 * i + 0.621 * q);
  auto green = clampColorChannel(y - 0.272 * i - 0.647 * q);
  auto blue = clampColorChannel(y - 1.106 * i + 1.703 * q);
  return red << 32 | green << 16 | blue;
}
