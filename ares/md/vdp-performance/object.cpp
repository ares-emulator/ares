inline auto VDP::Object::width() const -> u32 {
  return 1 + tileWidth << 3;
}

inline auto VDP::Object::height() const -> u32 {
  return 1 + tileHeight << 3 + (vdp.io.interlaceMode == 3);
}
