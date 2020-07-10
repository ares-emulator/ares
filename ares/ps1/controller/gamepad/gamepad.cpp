Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  up       = node->append<Node::Button>("Up");
  down     = node->append<Node::Button>("Down");
  left     = node->append<Node::Button>("Left");
  right    = node->append<Node::Button>("Right");
  cross    = node->append<Node::Button>("Cross");
  circle   = node->append<Node::Button>("Circle");
  square   = node->append<Node::Button>("Square");
  triangle = node->append<Node::Button>("Triangle");
  l1       = node->append<Node::Button>("L1");
  l2       = node->append<Node::Button>("L2");
  r1       = node->append<Node::Button>("R1");
  r2       = node->append<Node::Button>("R2");
  select   = node->append<Node::Button>("Select");
  start    = node->append<Node::Button>("Start");

  lx       = node->append<Node::Axis>  ("LX-axis");
  ly       = node->append<Node::Axis>  ("LY-axis");
  rx       = node->append<Node::Axis>  ("RX-axis");
  ry       = node->append<Node::Axis>  ("RY-axis");
  lt       = node->append<Node::Button>("L-thumb");
  rt       = node->append<Node::Button>("R-thumb");
}

auto Gamepad::read() -> u64 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(cross);
  platform->input(circle);
  platform->input(square);
  platform->input(triangle);
  platform->input(l1);
  platform->input(l2);
  platform->input(r1);
  platform->input(r2);
  platform->input(select);
  platform->input(start);

  platform->input(lx);
  platform->input(ly);
  platform->input(rx);
  platform->input(ry);
  platform->input(lt);
  platform->input(rt);

  uint64 data;
  //0x5a41 = digital; 0x5a73 = analog/pad; 0x5a53 = analog/stick
  data.bit( 0,15) = 0x415a;
  data.bit(16) = !select->value();
  data.bit(17) = !lt->value();
  data.bit(18) = !rt->value();
  data.bit(19) = !start->value();
  data.bit(20) = !(up->value() & !down->value());
  data.bit(21) = !(right->value() & !left->value());
  data.bit(22) = !(down->value() & !up->value());
  data.bit(23) = !(left->value() & !right->value());
  data.bit(24) = !l2->value();
  data.bit(25) = !r2->value();
  data.bit(26) = !l1->value();
  data.bit(27) = !r1->value();
  data.bit(28) = !triangle->value();
  data.bit(29) = !circle->value();
  data.bit(30) = !cross->value();
  data.bit(31) = !square->value();
  data.bit(32,39) = (rx->value() >> 8) + 128;
  data.bit(40,47) = (ry->value() >> 8) + 128;
  data.bit(48,55) = (lx->value() >> 8) + 128;
  data.bit(56,63) = (ly->value() >> 8) + 128;
  return data;
}
