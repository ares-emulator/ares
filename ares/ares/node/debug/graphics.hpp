struct Graphics : Object {
  DeclareClass(Graphics, "Graphics")

  Graphics(string name = {}) : Object(name) {
  }

  auto width() const -> uint { return _width; }
  auto height() const -> uint { return _height; }
  auto capture() const -> vector<uint32_t> { if(_capture) return _capture(); return {}; }

  auto setSize(uint width, uint height) -> void { _width = width, _height = height; }
  auto setCapture(function<vector<uint32_t> ()> capture) -> void { _capture = capture; }

  auto serialize(string& output, string depth) -> void override {
    Object::serialize(output, depth);
  }

  auto unserialize(Markup::Node node) -> void override {
    Object::unserialize(node);
  }

protected:
  uint _width = 0;
  uint _height = 0;
  function<vector<uint32_t> ()> _capture;
};
