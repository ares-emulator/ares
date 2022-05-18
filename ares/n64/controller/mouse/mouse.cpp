Mouse::Mouse(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Mouse");

  x           = node->append<Node::Input::Axis>  ("X");
  y           = node->append<Node::Input::Axis>  ("Y");
  left        = node->append<Node::Input::Button>("Left");
  right       = node->append<Node::Input::Button>("Right");
}

Mouse::~Mouse() {
}

auto Mouse::read() -> n32 {
  platform->input(x);
  platform->input(y);
  platform->input(left);
  platform->input(right);

  //TODO: Deal with more reasonable values for Mouse axis
  auto ax = x->value();
  auto ay = -y->value();

  n32 data;
  data.byte(0) = ay;
  data.byte(1) = ax;
  data.bit(30) = right->value();
  data.bit(31) = left->value();

  return data;
}

auto Mouse::readId() -> u16 {
  return 0x0002;
}
