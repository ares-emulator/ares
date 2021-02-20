Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  port = node->append<Node::Port>("Pak");
  port->setFamily("Nintendo 64");
  port->setType("Pak");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setConnect([&] { return connect(); });
  port->setDisconnect([&] { return disconnect(); });
  port->setSupported({"Controller Pak", "Rumble Pak"});

  x           = node->append<Node::Input::Axis>  ("X-Axis");
  y           = node->append<Node::Input::Axis>  ("Y-Axis");
  up          = node->append<Node::Input::Button>("Up");
  down        = node->append<Node::Input::Button>("Down");
  left        = node->append<Node::Input::Button>("Left");
  right       = node->append<Node::Input::Button>("Right");
  b           = node->append<Node::Input::Button>("B");
  a           = node->append<Node::Input::Button>("A");
  cameraUp    = node->append<Node::Input::Button>("C-Up");
  cameraDown  = node->append<Node::Input::Button>("C-Down");
  cameraLeft  = node->append<Node::Input::Button>("C-Left");
  cameraRight = node->append<Node::Input::Button>("C-Right");
  l           = node->append<Node::Input::Button>("L");
  r           = node->append<Node::Input::Button>("R");
  z           = node->append<Node::Input::Button>("Z");
  start       = node->append<Node::Input::Button>("Start");
}

Gamepad::~Gamepad() {
  disconnect();
}

auto Gamepad::allocate(string name) -> Node::Peripheral {
  if(name == "Controller Pak") return pak = port->append<Node::Peripheral>("Controller Pak");
  if(name == "Rumble Pak"    ) return pak = port->append<Node::Peripheral>("Rumble Pak");
  return {};
}

auto Gamepad::connect() -> void {
  if(!pak) return;
  if(pak->name() == "Controller Pak") {
    ram.allocate(32_KiB);
    if(auto fp = platform->open(node, "save.pak", File::Read)) {
      ram.load(fp);
    }
  }
  if(pak->name() == "Rumble Pak") {
    motor = node->append<Node::Input::Rumble>("Rumble");
  }
}

auto Gamepad::disconnect() -> void {
  if(!pak) return;
  if(pak->name() == "Controller Pak") {
    if(auto fp = platform->open(node, "save.pak", File::Write)) {
      ram.save(fp);
    }
    ram.reset();
  }
  if(pak->name() == "Rumble Pak") {
    rumble(false);
    node->remove(motor);
    motor.reset();
  }
  port->remove(pak);
  pak.reset();
}

auto Gamepad::rumble(bool enable) -> void {
  if(!motor) return;
  motor->setEnable(enable);
  platform->input(motor);
}

auto Gamepad::read() -> n32 {
  platform->input(x);
  platform->input(y);
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(b);
  platform->input(a);
  platform->input(cameraUp);
  platform->input(cameraDown);
  platform->input(cameraLeft);
  platform->input(cameraRight);
  platform->input(l);
  platform->input(r);
  platform->input(z);
  platform->input(start);

  //16-bit signed -> 8-bit signed
  auto ay = sclamp<8>(-y->value() >> 8);
  auto ax = sclamp<8>(+x->value() >> 8);

  //dead-zone
  if(abs(ay) < 24) ay = 0;
  if(abs(ax) < 24) ax = 0;

  n32 data;
  data.byte(0) = ay;
  data.byte(1) = ax;
  data.bit(16) = cameraRight->value();
  data.bit(17) = cameraLeft->value();
  data.bit(18) = cameraDown->value();
  data.bit(19) = cameraUp->value();
  data.bit(20) = l->value();
  data.bit(21) = r->value();
  data.bit(22) = 0;
  data.bit(23) = 0;
  data.bit(24) = right->value() & !left->value();
  data.bit(25) = left->value() & !right->value();
  data.bit(26) = down->value() & !up->value();
  data.bit(27) = up->value() & !down->value();
  data.bit(28) = start->value();
  data.bit(29) = z->value();
  data.bit(30) = b->value();
  data.bit(31) = a->value();
  return data;
}

auto Gamepad::serialize(serializer& s) -> void {
  s(ram);
  rumble(false);
}
