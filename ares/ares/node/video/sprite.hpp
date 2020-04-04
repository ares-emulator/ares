struct Sprite : Object {
  DeclareClass(Sprite, "Sprite")
  using Object::Object;

  auto visible() const -> bool { return _visible; }
  auto x() const -> uint { return _x; }
  auto y() const -> uint { return _y; }
  auto width() const -> uint { return _width; }
  auto height() const -> uint { return _height; }
  auto image() const -> const uint32* { return _pixels.data(); }

  auto setVisible(bool visible) -> void;
  auto setPosition(uint x, uint y) -> void;
  auto setImage(nall::image, bool invert = false) -> void;

protected:
  bool _visible = false;
  uint _x = 0;
  uint _y = 0;
  uint _width = 0;
  uint _height = 0;
  unique_pointer<uint32[]> _pixels;
};
