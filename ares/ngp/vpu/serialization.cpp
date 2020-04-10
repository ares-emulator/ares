auto VPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s.array(colors);

  s.integer(background.color);
  s.integer(background.unused);
  s.integer(background.mode);

  s.integer(window.hoffset);
  s.integer(window.voffset);
  s.integer(window.hlength);
  s.integer(window.vlength);
  s.integer(window.color);
  s.integer(window.output);

  for(auto& a : attributes) {
    s.integer(a.character);
    s.integer(a.code);
    s.integer(a.palette);
    s.integer(a.vflip);
    s.integer(a.hflip);
  }

  for(auto& c : characters) {
    for(auto& y : c) {
      for(auto& x : y) {
        s.integer(x);
      }
    }
  }

  s.integer(plane1.address);
  s.integer(plane1.colorNative);
  s.integer(plane1.colorCompatible);
  s.integer(plane1.hscroll);
  s.integer(plane1.vscroll);
  s.array(plane1.palette[0]);
  s.array(plane1.palette[1]);
  s.integer(plane1.output);
  s.integer(plane1.priority);

  s.integer(plane2.address);
  s.integer(plane2.colorNative);
  s.integer(plane2.colorCompatible);
  s.integer(plane2.hscroll);
  s.integer(plane2.vscroll);
  s.array(plane2.palette[0]);
  s.array(plane2.palette[1]);
  s.integer(plane2.output);
  s.integer(plane2.priority);

  s.integer(sprite.colorNative);
  s.integer(sprite.colorCompatible);
  s.integer(sprite.hscroll);
  s.integer(sprite.vscroll);
  s.array(sprite.palette[0]);
  s.array(sprite.palette[1]);
  s.integer(sprite.output);
  s.integer(sprite.priority);

  for(auto& o : sprites) {
    s.integer(o.character);
    s.integer(o.vchain);
    s.integer(o.hchain);
    s.integer(o.priority);
    s.integer(o.palette);
    s.integer(o.vflip);
    s.integer(o.hflip);
    s.integer(o.hoffset);
    s.integer(o.voffset);
    s.integer(o.code);
  }

  for(auto& o : tiles) {
    s.integer(o.x);
    s.integer(o.y);
    s.integer(o.character);
    s.integer(o.priority);
    s.integer(o.palette);
    s.integer(o.hflip);
    s.integer(o.code);
  }

  s.integer(tileCount);

  s.integer(led.control);
  s.integer(led.frequency);

  s.integer(dac.negate);
  s.integer(dac.colorMode);

  s.integer(io.vlines);
  s.integer(io.vcounter);
  s.integer(io.hcounter);

  s.integer(io.hblankEnableIRQ);
  s.integer(io.vblankEnableIRQ);
  s.integer(io.hblankActive);
  s.integer(io.vblankActive);
  s.integer(io.characterOver);
  s.integer(io.planePriority);
}
