Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  x           = node->append<Node::Axis>  ("X-axis");
  y           = node->append<Node::Axis>  ("Y-axis");
  up          = node->append<Node::Button>("Up");
  down        = node->append<Node::Button>("Down");
  left        = node->append<Node::Button>("Left");
  right       = node->append<Node::Button>("Right");
  b           = node->append<Node::Button>("B");
  a           = node->append<Node::Button>("A");
  cameraUp    = node->append<Node::Button>("C-Up");
  cameraDown  = node->append<Node::Button>("C-Down");
  cameraLeft  = node->append<Node::Button>("C-Left");
  cameraRight = node->append<Node::Button>("C-Right");
  l           = node->append<Node::Button>("L");
  r           = node->append<Node::Button>("R");
  z           = node->append<Node::Button>("Z");
  start       = node->append<Node::Button>("Start");
}

auto Gamepad::read() -> uint32 {
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

  uint32 data;
  data.byte(0) = -y->value() >> 8;
  data.byte(1) = +x->value() >> 8;
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
