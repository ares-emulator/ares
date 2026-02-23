auto KGE::DAC::begin(n8 y) -> void {
  output = self.screen->pixels().data() + y * 160;
}

auto KGE::DAC::run(n8 x, n8 y) -> void {
  n12 color;

  auto plane1 = self.plane1.render(x, y);
  auto plane2 = self.plane2.render(x, y);
  auto sprite = self.sprite.render(x, y);
  auto window = self.window.render(x, y);

  //the dev manual says only background.mode = 0b10 enables background.color
  //Ogre Battle however sets 0b00 and expects the background color to be used
  //as such, it appears that only the lower bit of background.mode matters?
  if(!self.background.mode.bit(0)) {
    color = self.background.color;
    if(Model::NeoGeoPocketColor()) color = colors[0xf0 + color];
  }

  if(sprite && sprite->priority == 1) {
    color = sprite->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x00 + color];  //native
      if(colorMode == 1) color = colors[0xc0 + color];  //compatible
    }
  }

  if(plane1 && plane1->priority == 0) {
    color = plane1->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x40 + color];  //native
      if(colorMode == 1) color = colors[0xd0 + color];  //compatible
    }
  }

  if(plane2 && plane2->priority == 0) {
    color = plane2->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x80 + color];  //native
      if(colorMode == 1) color = colors[0xe0 + color];  //compatible
    }
  }

  if(sprite && sprite->priority == 2) {
    color = sprite->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x00 + color];  //native
      if(colorMode == 1) color = colors[0xc0 + color];  //compatible
    }
  }

  if(plane1 && plane1->priority == 1) {
    color = plane1->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x40 + color];  //native
      if(colorMode == 1) color = colors[0xd0 + color];  //compatible
    }
  }

  if(plane2 && plane2->priority == 1) {
    color = plane2->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x80 + color];  //native
      if(colorMode == 1) color = colors[0xe0 + color];  //compatible
    }
  }

  if(sprite && sprite->priority == 3) {
    color = sprite->color;
    if(Model::NeoGeoPocketColor()) {
      if(colorMode == 0) color = colors[0x00 + color];  //native
      if(colorMode == 1) color = colors[0xc0 + color];  //compatible
    }
  }

  if(window) {
    color = window->color;
    if(Model::NeoGeoPocketColor()) color = colors[0xf8 + color];
  }

  if(Model::NeoGeoPocket() && negate) color ^= 7;

  output[x] = color;
}

auto KGE::DAC::power() -> void {
  for(auto& color : colors) color = 0;
  negate    = 0;
  colorMode = 0;
  output    = nullptr;
}
