auto VDP::Object::width() const -> uint {
  return 1 + tileWidth << 3;
}

auto VDP::Object::height() const -> uint {
  return 1 + tileHeight << 3 + (vdp.io.interlaceMode == 3);
}
