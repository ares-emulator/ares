struct Screen : Object {
  DeclareClass(Screen, "Screen")
  using Object::Object;

  auto width() const -> uint { return _width; }
  auto height() const -> uint { return _height; }
  auto scaleX() const -> double { return _scaleX; }
  auto scaleY() const -> double { return _scaleY; }
  auto aspectX() const -> double { return _aspectX; }
  auto aspectY() const -> double { return _aspectY; }
  auto colors() const -> uint { return _colors; }

  auto saturation() const -> double { return _saturation; }
  auto gamma() const -> double { return _gamma; }
  auto luminance() const -> double { return _luminance; }

  auto colorBleed() const -> bool { return _colorBleed; }
  auto interframeBlending() const -> bool { return _interframeBlending; }
  auto rotation() const -> uint { return _rotation; }

  auto resetPalette() -> void;
  auto resetSprites() -> void;

  auto setSize(uint width, uint height) -> void { _width = width, _height = height; }
  auto setScale(double scaleX, double scaleY) -> void { _scaleX = scaleX, _scaleY = scaleY; }
  auto setAspect(double aspectX, double aspectY) -> void { _aspectX = aspectX, _aspectY = aspectY; }

  auto setSaturation(double saturation) -> void { _saturation = saturation; _palette.reset(); }
  auto setGamma(double gamma) -> void { _gamma = gamma; _palette.reset(); }
  auto setLuminance(double luminance) -> void { _luminance = luminance; _palette.reset(); }

  auto setColorBleed(bool colorBleed) -> void { _colorBleed = colorBleed; }
  auto setInterframeBlending(bool interframeBlending) -> void { _interframeBlending = interframeBlending; }
  auto setRotation(uint rotation) -> void { _rotation = rotation; }

  auto attach(Node::Sprite) -> void;
  auto detach(Node::Sprite) -> void;

  auto colors(uint colors, function<uint64 (uint32)> color) -> void;
  auto refresh(uint32* input, uint pitch, uint width, uint height) -> void;

  auto serialize(string& output, string depth) -> void override;
  auto unserialize(Markup::Node node) -> void override;

protected:
  uint _width = 0;
  uint _height = 0;
  double _scaleX = 1.0;
  double _scaleY = 1.0;
  double _aspectX = 1.0;
  double _aspectY = 1.0;
  uint _colors = 0;
  double _saturation = 1.0;
  double _gamma = 1.0;
  double _luminance = 1.0;
  bool _colorBleed = false;
  bool _interframeBlending = false;
  uint _rotation = 0;  //counter-clockwise (90 = left, 270 = right)

  function<uint64 (uint32)> _color;
  unique_pointer<uint32[]> _buffer;
  unique_pointer<uint32[]> _rotate;
  unique_pointer<uint32[]> _palette;
  vector<Node::Sprite> _sprites;

  uint _renderWidth = 0;
  uint _renderHeight = 0;
};
