auto VPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(colors);

  s(background.color);
  s(background.unused);
  s(background.mode);

  s(window.hoffset);
  s(window.voffset);
  s(window.hlength);
  s(window.vlength);
  s(window.color);
  s(window.output);

  for(auto& a : attributes) {
    s(a.character);
    s(a.code);
    s(a.palette);
    s(a.vflip);
    s(a.hflip);
  }

  for(auto& c : characters) {
    for(auto& y : c) {
      for(auto& x : y) {
        s(x);
      }
    }
  }

  s(plane1.address);
  s(plane1.colorNative);
  s(plane1.colorCompatible);
  s(plane1.hscroll);
  s(plane1.vscroll);
  s(plane1.palette[0]);
  s(plane1.palette[1]);
  s(plane1.output);
  s(plane1.priority);

  s(plane2.address);
  s(plane2.colorNative);
  s(plane2.colorCompatible);
  s(plane2.hscroll);
  s(plane2.vscroll);
  s(plane2.palette[0]);
  s(plane2.palette[1]);
  s(plane2.output);
  s(plane2.priority);

  s(sprite.colorNative);
  s(sprite.colorCompatible);
  s(sprite.hscroll);
  s(sprite.vscroll);
  s(sprite.palette[0]);
  s(sprite.palette[1]);
  s(sprite.output);
  s(sprite.priority);

  for(auto& o : sprites) {
    s(o.character);
    s(o.vchain);
    s(o.hchain);
    s(o.priority);
    s(o.palette);
    s(o.vflip);
    s(o.hflip);
    s(o.hoffset);
    s(o.voffset);
    s(o.code);
  }

  for(auto& o : tiles) {
    s(o.x);
    s(o.y);
    s(o.character);
    s(o.priority);
    s(o.palette);
    s(o.hflip);
    s(o.code);
  }

  s(tileCount);

  s(led.control);
  s(led.frequency);

  s(dac.negate);
  s(dac.colorMode);

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
