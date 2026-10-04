auto POKEY::output() const -> f64 {
  // Altirra 4.40 fits these weights and the curve below to measurements of a
  // POKEY output path. The measured machine is not identified as an Atari 5200,
  // so this remains a provisional chip-level model until pin 37 is measured on
  // an identified console.
  // Sources: src/ATAudio/source/pokey.cpp, UpdateMixTable(), and
  // src/ATAudio/source/pokeyrenderer.cpp, kVolMixLookup.
  static constexpr u16 VolumeIndex[16] = {
     0,  1,  5,  6,
    25, 26, 30, 31,
    50, 51, 55, 56,
    75, 76, 80, 81,
  };

  struct MixTable {
    f64 output[325];

    MixTable() {
      static constexpr f64 Bit0 = 0.12 / 8.24;
      static constexpr f64 Bit1 = 0.26 / 8.24;
      static constexpr f64 Bit2 = 0.56 / 8.24;
      static constexpr f64 Threshold = 0.14;
      static constexpr f64 Slope = 2.1;
      static constexpr f64 Curve = 2.85;

      u32 index = 0;
      for(u32 bit23 = 0; bit23 < 13; bit23++) {
        for(u32 bit1 = 0; bit1 < 5; bit1++) {
          for(u32 bit0 = 0; bit0 < 5; bit0++) {
            f64 input = bit23 * Bit2 + bit1 * Bit1 + bit0 * Bit0;
            output[index++] = input < Threshold
              ? -Slope * input
              : -Slope * (Threshold + (1.0 - exp(-Curve * (input - Threshold))) / Curve);
          }
        }
      }
    }
  };
  static const MixTable table;

  auto levels = audio.levels();
  u32 index = 0;
  for(auto level : levels.channel) index += VolumeIndex[level];
  return table.output[index];
}
