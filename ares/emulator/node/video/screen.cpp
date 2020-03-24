auto Screen::resetPalette() -> void {
  _palette.reset();
}

auto Screen::resetSprites() -> void {
  _sprites.reset();
}

auto Screen::attach(Node::Sprite sprite) -> void {
  if(_sprites.find(sprite)) return;
  _sprites.append(sprite);
}

auto Screen::detach(Node::Sprite sprite) -> void {
  if(!_sprites.find(sprite)) return;
  _sprites.removeByValue(sprite);
}

auto Screen::colors(uint colors, function<uint64 (uint32)> color) -> void {
  _colors = colors;
  _color = color;
  _palette.reset();
}

auto Screen::refresh(uint32* input, uint pitch, uint width, uint height) -> void {
  if(runAhead()) return;

  //allocate the screen buffers (only when growing)
  if(_renderWidth != width || _renderHeight != height) {
    if(_renderWidth * _renderHeight < width * height) {
      _buffer = new uint32[width * height];
      _rotate = new uint32[width * height];
    }
  }

  //generate the color lookup palettes to convert native colors to ARGB8888
  if(!_palette) {
    _palette = new uint32[_colors];
    for(uint index : range(_colors)) {
      uint64 color = _color(index);
      uint16 b = color.bit( 0,15);
      uint16 g = color.bit(16,31);
      uint16 r = color.bit(32,47);
      uint16 a = 65535;

      if(_saturation != 1.0) {
        uint16 grayscale = uclamp<16>((r + g + b) / 3);
        double inverse = max(0.0, 1.0 - _saturation);
        r = uclamp<16>(r * _saturation + grayscale * inverse);
        g = uclamp<16>(g * _saturation + grayscale * inverse);
        b = uclamp<16>(b * _saturation + grayscale * inverse);
      }

      if(_gamma != 1.0) {
        double reciprocal = 1.0 / 32767.0;
        r = r > 32767 ? r : uint16(32767 * pow(r * reciprocal, _gamma));
        g = g > 32767 ? g : uint16(32767 * pow(g * reciprocal, _gamma));
        b = b > 32767 ? b : uint16(32767 * pow(b * reciprocal, _gamma));
      }

      if(_luminance != 1.0) {
        r = uclamp<16>(r * _luminance);
        g = uclamp<16>(g * _luminance);
        b = uclamp<16>(b * _luminance);
      }

      a >>= 8;
      r >>= 8;
      g >>= 8;
      b >>= 8;

      _palette[index] = a << 24 | r << 16 | g << 8 | b << 0;
    }
  }

  auto output = _buffer.data();
  pitch >>= 2;  //bytes to words

  for(uint y : range(height)) {
    auto source = input + y * pitch;
    auto target = output + y * width;

    //if not blending, or if previous frame resolution was different, render normally
    if(!_interframeBlending || width != _renderWidth || height != _renderHeight) {
      for(uint x : range(width)) {
        auto color = _palette[*source++];
        *target++ = color;
      }
    } else {
      uint32 mask = 1 << 24 | 1 << 16 | 1 << 8 | 1 << 0;
      for(uint x : range(width)) {
        auto a = *target;
        auto b = _palette[*source++];
        *target++ = (a + b - ((a ^ b) & mask)) >> 1;
      }
    }
  }

  if(_colorBleed) {
    uint32 mask = 1 << 24 | 1 << 16 | 1 << 8 | 1 << 9;
    for(uint y : range(height)) {
      auto target = output + y * width;
      for(uint x : range(width)) {
        auto a = target[x];
        auto b = target[x + (x != width - 1)];
        target[x] = (a + b - ((a ^ b) & mask)) >> 1;
      }
    }
  }

  for(auto& sprite : _sprites) {
    if(!sprite->visible()) continue;

    uint32 alpha = 255u << 24;
    for(int y : range(sprite->height())) {
      int pixelY = sprite->y() + y;
      if(pixelY < 0 || pixelY >= height) continue;

      auto source = sprite->image() + y * sprite->width();
      auto target = &output[pixelY * width];
      for(int x : range(sprite->width())) {
        int pixelX = sprite->x() + x;
        if(pixelX < 0 || pixelX >= width) continue;

        auto pixel = source[x];
        if(pixel >> 24) target[pixelX] = alpha | pixel;
      }
    }
  }

  if(_rotation == 90) {
    //rotate left
    for(uint y : range(height)) {
      auto source = output + y * width;
      for(uint x : range(width)) {
        auto target = _rotate.data() + (width - 1 - x) * height + y;
        *target = *source++;
      }
    }
    output = _rotate.data();
    swap(width, height);
  }

  if(_rotation == 180) {
    //rotate upside down
    for(uint y : range(height)) {
      auto source = output + y * width;
      for(uint x : range(width)) {
        auto target = _rotate.data() + (height - 1 - y) * width + (width - 1 - x);
        *target = *source++;
      }
    }
    output = _rotate.data();
  }

  if(_rotation == 270) {
    //rotate right
    for(uint y : range(height)) {
      auto source = output + y * width;
      for(uint x : range(width)) {
        auto target = _rotate.data() + x * height + (height - 1 - y);
        *target = *source++;
      }
    }
    output = _rotate.data();
    swap(width, height);
  }

  platform->video(shared(), (const uint32_t*)output, width * sizeof(uint32), width, height);

  _renderWidth = width;
  _renderHeight = height;
}

auto Screen::serialize(string& output, string depth) -> void {
  Object::serialize(output, depth);
  output.append(depth, "  width: ", _width, "\n");
  output.append(depth, "  height: ", _height, "\n");
  output.append(depth, "  scaleX: ", _scaleX, "\n");
  output.append(depth, "  scaleY: ", _scaleY, "\n");
  output.append(depth, "  aspectX: ", _aspectX, "\n");
  output.append(depth, "  aspectY: ", _aspectY, "\n");
  output.append(depth, "  colors: ", _colors, "\n");
  output.append(depth, "  saturation: ", _saturation, "\n");
  output.append(depth, "  gamma: ", _gamma, "\n");
  output.append(depth, "  luminance: ", _luminance, "\n");
  output.append(depth, "  colorBleed: ", _colorBleed, "\n");
  output.append(depth, "  interframeBlending: ", _interframeBlending, "\n");
  output.append(depth, "  rotation: ", _rotation, "\n");
}

auto Screen::unserialize(Markup::Node node) -> void {
  Object::unserialize(node);
  _width = node["width"].natural();
  _height = node["height"].natural();
  _scaleX = node["scaleX"].real();
  _scaleY = node["scaleY"].real();
  _aspectX = node["aspectX"].real();
  _aspectY = node["aspectY"].real();
  _colors = node["colors"].natural();
  _saturation = node["saturation"].real();
  _gamma = node["gamma"].real();
  _luminance = node["luminance"].real();
  _colorBleed = node["colorBleed"].boolean();
  _interframeBlending = node["interframeBlending"].boolean();
  _rotation = node["rotation"].natural();
  resetPalette();
  resetSprites();
}
