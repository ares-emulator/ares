auto KGE::Plane::render(n8 x, n8 y) -> maybe<Output&> {
  x += hscroll;
  y += vscroll;

  n11 address;
  address.bit(0,4) = x >> 3;
  address.bit(5,9) = y >> 3;
  address.bit(10)  = this == &self.plane2;

  x = (n3)x;
  y = (n3)y;

  auto& attribute = self.attributes[address];
  if(attribute.hflip) x ^= 7;
  if(attribute.vflip) y ^= 7;

  if(n2 index = self.characters[attribute.character][y][x]) {
    output.priority = self.io.planePriority ^ (this == &self.plane1);
    if(Model::NeoGeoPocket()) {
      if(index) output.color = palette[attribute.palette][index];
    }
    if(Model::NeoGeoPocketColor()) {
      switch(self.dac.colorMode) {
      case 0: output.color = attribute.code    * 4 + index; break;
      case 1: output.color = attribute.palette * 8 + palette[attribute.palette][index]; break;
      }
    }
    return output;
  }

  return {};
}

auto KGE::Plane::power() -> void {
  output = {};
  hscroll = 0;
  vscroll = 0;
  memory::assign(palette[0], 0, 0, 0, 0);
  memory::assign(palette[1], 0, 0, 0, 0);
}
