auto KGE::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(window);
  s(plane1);
  s(plane2);
  s(sprite);
  s(dac);

  for(auto& attribute : attributes) {
    s(attribute.character);
    s(attribute.code);
    s(attribute.palette);
    s(attribute.vflip);
    s(attribute.hflip);
  }

  for(auto& character : characters) {
    for(auto& y : character) {
      for(auto& x : y) {
        s(x);
      }
    }
  }

  s(background.color);
  s(background.unused);
  s(background.mode);

  s(led.control);
  s(led.frequency);

  s(io.vlines);
  s(io.vcounter);
  s(io.hcounter);

  s(io.hblankEnableIRQ);
  s(io.vblankEnableIRQ);
  s(io.hblankActive);
  s(io.vblankActive);
  s(io.characterOver);
  s(io.planePriority);
}

auto KGE::Window::serialize(serializer& s) -> void {
  s(output.color);

  s(hoffset);
  s(voffset);
  s(hlength);
  s(vlength);
  s(color);
}

auto KGE::Plane::serialize(serializer& s) -> void {
  s(output.color);
  s(output.priority);

  s(hscroll);
  s(vscroll);
  s(palette[0]);
  s(palette[1]);
}

auto KGE::Sprite::serialize(serializer& s) -> void {
  s(output.color);
  s(output.priority);

  for(auto& object : objects) {
    s(object.character);
    s(object.vchain);
    s(object.hchain);
    s(object.priority);
    s(object.palette);
    s(object.vflip);
    s(object.hflip);
    s(object.hoffset);
    s(object.voffset);
    s(object.code);
  }

  for(auto& tile : tiles) {
    s(tile.x);
    s(tile.y);
    s(tile.character);
    s(tile.priority);
    s(tile.palette);
    s(tile.hflip);
    s(tile.code);
  }

  s(hscroll);
  s(vscroll);
  s(palette);
}

auto KGE::DAC::serialize(serializer& s) -> void {
  s(colors);
  s(colorMode);
  s(negate);
}
