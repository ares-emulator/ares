inline auto VDP::Object::width() const -> uint {
  return 1 + tileWidth << 3;
}

inline auto VDP::Object::height() const -> uint {
  return 1 + tileHeight << 3 + (vdp.io.interlaceMode == 3);
}
