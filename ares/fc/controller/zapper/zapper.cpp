Zapper::Zapper(Node::Port port) {
  node = port->append<Node::Peripheral>("Zapper");

  x = node->append<Node::Input::Axis>("X");
  y = node->append<Node::Input::Axis>("Y");
  trigger = node->append<Node::Input::Button>("Trigger");

  lightGun.load(node, x, y, trigger);
}

Zapper::~Zapper() {
  lightGun.unload();
}

auto LightGun::load(Node::Object parent, Node::Input::Axis x, Node::Input::Axis y,
  Node::Input::Button trigger) -> void {
  unload();
  this->parent = parent;
  this->x = x;
  this->y = y;
  this->trigger = trigger;
  cx = 256 / 2;
  cy = 240 / 2;
  px = 0;
  py = 0;
  nx = 256 / 2;
  ny = 240 / 2;
  previous = 0;

  sprite = parent->append<Node::Video::Sprite>("Crosshair");
  sprite->setImage(Resource::Sprite::SuperFamicom::CrosshairGreen);
  ppu.screen->attach(sprite);
}

auto LightGun::unload() -> void {
  if(ppu.screen && sprite) ppu.screen->detach(sprite);
  if(parent && sprite) parent->remove(sprite);
  parent.reset();
  x.reset();
  y.reset();
  trigger.reset();
  sprite.reset();
}

auto LightGun::data() -> n3 {
  u32 next = ppu.io.ly * 283 + ppu.io.lx;
  n3 result = 0b000;

  platform->input(trigger);
  if(trigger->value()) result.bit(2) = 1;

  if(next < previous) {
    platform->input(x);  //-n = left, 0 = center, +n = right
    platform->input(y);  //-n = up,   0 = center, +n = down

    if(x->value() != px || y->value() != py) {
      px = x->value();
      py = y->value();

      cx = max(-8, min(256 + 8, px + cx));
      cy = max(-8, min(240 + 8, py + cy));

      sprite->setPosition(cx, cy);
      sprite->setVisible(true);

      nx = cx + 8;
      ny = cy + 8;
    }
  }
  previous = next;

  bool offscreen = nx < 0 || ny < 0 || nx >= 256 || ny >= 240;
  if(offscreen) {
    result.bit(1) = 1;
    return result;
  }

  u32* target = ppu.screen->pixels().data();
  u32 count = 0;
  for(int y = ny - 8; y <= ny + 8; y++) {
    if(y < 0) continue;
    if(y <= ppu.io.ly - 18) continue;
    if(y > ppu.io.ly) break;

    for(int x = nx - 8; x <= nx + 8; x++) {
      if(x < 0) continue;
      if(x >= 256) break;

      n32 offset = y * 283 + x;  //y * 283 + x + Region::PAL() ? 18 : 16;
      n64 color = ppu.color(target[offset]);
      f64 brightness = 0;

      brightness += ((color >> 32) & 0xffff) * 0.299;
      brightness += ((color >> 16) & 0xffff) * 0.587;
      brightness += ((color >>  0) & 0xffff) * 0.114;
      if(brightness > 0x80) count++;
    }
  }

  if(count < 0x40) result.bit(1) = 1;
  return result;
}

auto LightGun::serialize(serializer& s) -> void {
  s(cx);
  s(cy);
  s(px);
  s(py);
  s(nx);
  s(ny);
  s(previous);

  if(s.reading() && sprite) {
    sprite->setPosition(cx, cy);
    sprite->setVisible(true);
  }
}

auto Zapper::data() -> n3 {
  return lightGun.data();
}

auto Zapper::latch(n1 data) -> void {
}

auto Zapper::serialize(serializer& s) -> void {
}
