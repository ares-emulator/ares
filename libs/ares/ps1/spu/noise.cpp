auto SPU::Noise::update() -> void {
  static const u8 wave[64] = {
    1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,
  };
  static const u8 frequency[4] = {0, 84, 140, 180};

  count += 0x10000 + frequency[step];
  if(u16(count) >= 210) count = count + 0x10000 - frequency[step];
  u32 limit = (0x8000 >> shift) << 16;
  if(count < limit) return;
  count %= limit;
  level  = level << 1 | wave[level >> 10 & 63];
}
